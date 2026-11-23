use astron_pak::{inspect_and_verify, pack_directory, unpack_archive};
use std::env;
use std::path::Path;
use std::process;

fn print_usage() {
    eprintln!("Astron Engine Asset Packager (Rust v1.0.0)");
    eprintln!("Uso:");
    eprintln!("  astron-pak pack   <diretorio_origem> <arquivo.apak>");
    eprintln!("  astron-pak list   <arquivo.apak>");
    eprintln!("  astron-pak verify <arquivo.apak>");
    eprintln!("  astron-pak unpack <arquivo.apak> <diretorio_destino>");
}

fn main() {
    let args: Vec<String> = env::args().collect();
    if args.len() < 3 {
        print_usage();
        process::exit(1);
    }

    match args[1].as_str() {
        "pack" => {
            if args.len() < 4 {
                print_usage();
                process::exit(1);
            }
            let src = Path::new(&args[2]);
            let dst = Path::new(&args[3]);
            match pack_directory(src, dst) {
                Ok(entries) => {
                    let orig: u64 = entries.iter().map(|e| e.original_size).sum();
                    let stored: u64 = entries.iter().map(|e| e.stored_size).sum();
                    println!(
                        "OK: {} arquivos empacotados em '{}' ({} -> {} bytes)",
                        entries.len(),
                        dst.display(),
                        orig,
                        stored
                    );
                }
                Err(e) => {
                    eprintln!("Erro ao empacotar: {}", e);
                    process::exit(2);
                }
            }
        }
        "list" | "verify" => {
            let pak = Path::new(&args[2]);
            match inspect_and_verify(pak) {
                Ok(entries) => {
                    for e in &entries {
                        println!(
                            "{:016x}  {:8} -> {:8} B  {}{}",
                            e.hash_fnv64,
                            e.original_size,
                            e.stored_size,
                            e.relative_path,
                            if e.flags != 0 { " [RLE]" } else { "" }
                        );
                    }
                    println!("Verificado com sucesso: {} entradas íntegras.", entries.len());
                }
                Err(e) => {
                    eprintln!("Falha na verificação APAK: {}", e);
                    process::exit(2);
                }
            }
        }
        "unpack" => {
            if args.len() < 4 {
                print_usage();
                process::exit(1);
            }
            let pak = Path::new(&args[2]);
            let dst = Path::new(&args[3]);
            match unpack_archive(pak, dst) {
                Ok(n) => println!("Extraídos {} arquivos em '{}'.", n, dst.display()),
                Err(e) => {
                    eprintln!("Erro ao desempacotar: {}", e);
                    process::exit(2);
                }
            }
        }
        _ => {
            print_usage();
            process::exit(1);
        }
    }
}
