//! Astron Engine — Empacotador binário nativo `.apak` em Rust (`astron_pak`).
//! Compacta scripts C#, cenas 3D `.scene`, shaders GLSL/HLSL e áudios `.wav`
//! em um único contêiner binário verificado por hash FNV-1a de 64 bits.

use std::ffi::CStr;
use std::fs::{self, File};
use std::io::{self, Read, Write};
use std::os::raw::c_char;
use std::path::{Path, PathBuf};

pub const APAK_MAGIC: &[u8; 8] = b"ASTRNPAK";
pub const APAK_VERSION: u32 = 1;
pub const FLAG_RLE_COMPRESSED: u32 = 1;

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct PakEntryMeta {
    pub relative_path: String,
    pub original_size: u64,
    pub stored_size: u64,
    pub hash_fnv64: u64,
    pub flags: u32,
}

/// Calcula hash FNV-1a de 64 bits para verificação de integridade de assets.
pub fn fnv1a_64(data: &[u8]) -> u64 {
    let mut hash: u64 = 0xcbf29ce484222325;
    for &byte in data {
        hash ^= u64::from(byte);
        hash = hash.wrapping_mul(0x00000100000001b3);
    }
    hash
}

/// Codificação PackBits/RLE rápida sem dependências externas.
pub fn rle_compress(input: &[u8]) -> Vec<u8> {
    let mut out = Vec::with_capacity(input.len());
    let n = input.len();
    let mut i = 0;

    while i < n {
        // Verifica repetição de bytes idênticos
        let mut run = 1usize;
        while i + run < n && run < 128 && input[i + run] == input[i] {
            run += 1;
        }

        if run >= 3 {
            // Marcador >= 128 indica repetição de (marker - 128 + 1) vezes
            out.push((128 + (run - 1)) as u8);
            out.push(input[i]);
            i += run;
        } else {
            // Bloco literal de até 128 bytes
            let start = i;
            let mut lit = 0usize;
            while i < n && lit < 128 {
                let mut look_run = 1usize;
                while i + look_run < n && look_run < 3 && input[i + look_run] == input[i] {
                    look_run += 1;
                }
                if look_run >= 3 {
                    break;
                }
                i += 1;
                lit += 1;
            }
            out.push((lit - 1) as u8);
            out.extend_from_slice(&input[start..start + lit]);
        }
    }
    out
}

/// Decodifica um buffer RLE de volta ao tamanho original.
pub fn rle_decompress(input: &[u8], expected_len: usize) -> io::Result<Vec<u8>> {
    let mut out = Vec::with_capacity(expected_len);
    let mut i = 0usize;
    while i < input.len() {
        let tag = input[i];
        i += 1;
        if tag >= 128 {
            let count = usize::from(tag - 128) + 1;
            if i >= input.len() {
                return Err(io::Error::new(io::ErrorKind::InvalidData, "RLE truncado"));
            }
            let val = input[i];
            i += 1;
            out.resize(out.len() + count, val);
        } else {
            let count = usize::from(tag) + 1;
            if i + count > input.len() {
                return Err(io::Error::new(io::ErrorKind::InvalidData, "Bloco literal RLE truncado"));
            }
            out.extend_from_slice(&input[i..i + count]);
            i += count;
        }
    }
    if out.len() != expected_len {
        return Err(io::Error::new(
            io::ErrorKind::InvalidData,
            "Tamanho descomprimido diverge do cabeçalho APAK",
        ));
    }
    Ok(out)
}

fn collect_files_recursive(base: &Path, dir: &Path, list: &mut Vec<PathBuf>) -> io::Result<()> {
    let mut entries: Vec<_> = fs::read_dir(dir)?.collect::<Result<Vec<_>, _>>()?;
    entries.sort_by_key(|e| e.file_name());
    for entry in entries {
        let p = entry.path();
        if p.is_dir() {
            collect_files_recursive(base, &p, list)?;
        } else if p.is_file() {
            if let Ok(rel) = p.strip_prefix(base) {
                list.push(rel.to_path_buf());
            }
        }
    }
    Ok(())
}

/// Empacota recursivamente um diretório de projeto em um arquivo `.apak`.
pub fn pack_directory(src_dir: &Path, out_apak: &Path) -> io::Result<Vec<PakEntryMeta>> {
    let mut rel_files = Vec::new();
    collect_files_recursive(src_dir, src_dir, &mut rel_files)?;

    if let Some(parent) = out_apak.parent() {
        if !parent.as_os_str().is_empty() {
            fs::create_dir_all(parent)?;
        }
    }

    let mut file = File::create(out_apak)?;
    file.write_all(APAK_MAGIC)?;
    file.write_all(&APAK_VERSION.to_le_bytes())?;
    file.write_all(&(rel_files.len() as u32).to_le_bytes())?;

    let mut metas = Vec::with_capacity(rel_files.len());

    for rel in rel_files {
        let full = src_dir.join(&rel);
        let raw = fs::read(&full)?;
        let hash = fnv1a_64(&raw);
        let compressed = rle_compress(&raw);

        let (payload, flags) = if !raw.is_empty() && compressed.len() + 8 < raw.len() {
            (compressed, FLAG_RLE_COMPRESSED)
        } else {
            (raw.clone(), 0u32)
        };

        let norm_rel = rel
            .components()
            .map(|c| c.as_os_str().to_string_lossy().into_owned())
            .collect::<Vec<_>>()
            .join("/");

        let path_bytes = norm_rel.as_bytes();
        file.write_all(&(path_bytes.len() as u16).to_le_bytes())?;
        file.write_all(path_bytes)?;
        file.write_all(&flags.to_le_bytes())?;
        file.write_all(&(raw.len() as u64).to_le_bytes())?;
        file.write_all(&(payload.len() as u64).to_le_bytes())?;
        file.write_all(&hash.to_le_bytes())?;
        file.write_all(&payload)?;

        metas.push(PakEntryMeta {
            relative_path: norm_rel,
            original_size: raw.len() as u64,
            stored_size: payload.len() as u64,
            hash_fnv64: hash,
            flags,
        });
    }

    file.flush()?;
    Ok(metas)
}

/// Lista e verifica o índice de um arquivo `.apak` (checando hashes FNV-1a).
pub fn inspect_and_verify(apak_path: &Path) -> io::Result<Vec<PakEntryMeta>> {
    let mut f = File::open(apak_path)?;
    let mut magic = [0u8; 8];
    f.read_exact(&mut magic)?;
    if &magic != APAK_MAGIC {
        return Err(io::Error::new(io::ErrorKind::InvalidData, "Assinatura APAK inválida"));
    }

    let mut u32_buf = [0u8; 4];
    f.read_exact(&mut u32_buf)?;
    let ver = u32::from_le_bytes(u32_buf);
    if ver != APAK_VERSION {
        return Err(io::Error::new(io::ErrorKind::InvalidData, "Versão APAK incompatível"));
    }

    f.read_exact(&mut u32_buf)?;
    let count = u32::from_le_bytes(u32_buf) as usize;
    let mut out = Vec::with_capacity(count);

    for _ in 0..count {
        let mut u16_buf = [0u8; 2];
        f.read_exact(&mut u16_buf)?;
        let path_len = u16::from_le_bytes(u16_buf) as usize;
        let mut path_bytes = vec![0u8; path_len];
        f.read_exact(&mut path_bytes)?;
        let rel_path = String::from_utf8(path_bytes)
            .map_err(|_| io::Error::new(io::ErrorKind::InvalidData, "Caminho UTF-8 inválido"))?;

        f.read_exact(&mut u32_buf)?;
        let flags = u32::from_le_bytes(u32_buf);

        let mut u64_buf = [0u8; 8];
        f.read_exact(&mut u64_buf)?;
        let orig_size = u64::from_le_bytes(u64_buf);
        f.read_exact(&mut u64_buf)?;
        let stored_size = u64::from_le_bytes(u64_buf);
        f.read_exact(&mut u64_buf)?;
        let expected_hash = u64::from_le_bytes(u64_buf);

        let mut payload = vec![0u8; stored_size as usize];
        f.read_exact(&mut payload)?;

        let decoded = if (flags & FLAG_RLE_COMPRESSED) != 0 {
            rle_decompress(&payload, orig_size as usize)?
        } else {
            payload
        };

        let actual_hash = fnv1a_64(&decoded);
        if actual_hash != expected_hash {
            return Err(io::Error::new(
                io::ErrorKind::InvalidData,
                format!("Falha de integridade FNV-1a em {}", rel_path),
            ));
        }

        out.push(PakEntryMeta {
            relative_path: rel_path,
            original_size: orig_size,
            stored_size,
            hash_fnv64: expected_hash,
            flags,
        });
    }

    Ok(out)
}

/// Desempacota um arquivo `.apak` para um diretório de destino.
pub fn unpack_archive(apak_path: &Path, dst_dir: &Path) -> io::Result<usize> {
    let mut f = File::open(apak_path)?;
    let mut magic = [0u8; 8];
    f.read_exact(&mut magic)?;
    if &magic != APAK_MAGIC {
        return Err(io::Error::new(io::ErrorKind::InvalidData, "Assinatura APAK inválida"));
    }

    let mut u32_buf = [0u8; 4];
    f.read_exact(&mut u32_buf)?;
    let _ver = u32::from_le_bytes(u32_buf);
    f.read_exact(&mut u32_buf)?;
    let count = u32::from_le_bytes(u32_buf) as usize;

    for _ in 0..count {
        let mut u16_buf = [0u8; 2];
        f.read_exact(&mut u16_buf)?;
        let path_len = u16::from_le_bytes(u16_buf) as usize;
        let mut path_bytes = vec![0u8; path_len];
        f.read_exact(&mut path_bytes)?;
        let rel_path = String::from_utf8(path_bytes)
            .map_err(|_| io::Error::new(io::ErrorKind::InvalidData, "UTF-8 inválido"))?;

        if rel_path.contains("..") {
            return Err(io::Error::new(io::ErrorKind::PermissionDenied, "Path traversal bloqueado"));
        }

        f.read_exact(&mut u32_buf)?;
        let flags = u32::from_le_bytes(u32_buf);

        let mut u64_buf = [0u8; 8];
        f.read_exact(&mut u64_buf)?;
        let orig_size = u64::from_le_bytes(u64_buf);
        f.read_exact(&mut u64_buf)?;
        let stored_size = u64::from_le_bytes(u64_buf);
        f.read_exact(&mut u64_buf)?;
        let expected_hash = u64::from_le_bytes(u64_buf);

        let mut payload = vec![0u8; stored_size as usize];
        f.read_exact(&mut payload)?;

        let decoded = if (flags & FLAG_RLE_COMPRESSED) != 0 {
            rle_decompress(&payload, orig_size as usize)?
        } else {
            payload
        };

        if fnv1a_64(&decoded) != expected_hash {
            return Err(io::Error::new(io::ErrorKind::InvalidData, "Hash FNV-1a inválido"));
        }

        let target = dst_dir.join(&rel_path);
        if let Some(p) = target.parent() {
            fs::create_dir_all(p)?;
        }
        fs::write(target, &decoded)?;
    }

    Ok(count)
}

// ==================== Interface FFI C / C++ / C# ====================

#[no_mangle]
pub unsafe extern "C" fn astron_pak_fnv64(ptr: *const u8, len: usize) -> u64 {
    if ptr.is_null() || len == 0 {
        return fnv1a_64(&[]);
    }
    let slice = std::slice::from_raw_parts(ptr, len);
    fnv1a_64(slice)
}

#[no_mangle]
pub unsafe extern "C" fn astron_pak_pack_dir_c(
    src_dir_utf8: *const c_char,
    out_apak_utf8: *const c_char,
) -> i32 {
    if src_dir_utf8.is_null() || out_apak_utf8.is_null() {
        return -1;
    }
    let Ok(src) = CStr::from_ptr(src_dir_utf8).to_str() else { return -2 };
    let Ok(dst) = CStr::from_ptr(out_apak_utf8).to_str() else { return -2 };
    match pack_directory(Path::new(src), Path::new(dst)) {
        Ok(entries) => entries.len() as i32,
        Err(_) => -3,
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn rle_roundtrip_works() {
        let mut sample = Vec::new();
        sample.extend_from_slice(&[0xAA; 64]);
        sample.extend_from_slice(b"Astron Engine Native C++ C# C GLSL Rust");
        sample.extend_from_slice(&[0x00; 96]);

        let comp = rle_compress(&sample);
        assert!(comp.len() < sample.len());
        let decomp = rle_decompress(&comp, sample.len()).expect("decompress");
        assert_eq!(decomp, sample);
    }

    #[test]
    fn apak_pack_verify_and_unpack() {
        let base = std::env::temp_dir().join("astron_pak_rs_unit_test");
        let _ = fs::remove_all(&base);
        let src = base.join("src");
        let dst = base.join("unpacked");
        let pak = base.join("bundle.apak");

        fs::create_dir_all(src.join("scripts")).unwrap();
        fs::write(
            src.join("scripts/Player.cs"),
            b"namespace Astron.Scripts { public sealed class Player {} }",
        )
        .unwrap();
        fs::write(src.join("default.scene"), vec![b' '; 256]).unwrap();

        let packed = pack_directory(&src, &pak).expect("pack");
        assert_eq!(packed.len(), 2);

        let verified = inspect_and_verify(&pak).expect("verify");
        assert_eq!(verified.len(), 2);

        let count = unpack_archive(&pak, &dst).expect("unpack");
        assert_eq!(count, 2);
        assert!(dst.join("scripts/Player.cs").exists());

        let _ = fs::remove_all(&base);
    }
}
