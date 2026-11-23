import React, { useState } from 'react';
import { useStore } from '../store.jsx';
import {
  ChevronDown, ChevronRight, Search, Pin, X, Folder, FolderOpen, Box, Package,
  Image, FileAudio, FileText, FileCode, FolderTree,
} from 'lucide-react';

function iconFor(name) {
  if (name.endsWith('.cs')) return { Icon: FileCode, color: '#7ee787' };
  if (name.endsWith('.scene')) return { Icon: Box, color: '#58a6ff' };
  if (name.endsWith('.model') || name.endsWith('.obj')) return { Icon: Package, color: '#d2a8ff' };
  if (name.endsWith('.tex') || name.endsWith('.png')) return { Icon: Image, color: '#79c0ff' };
  if (name.endsWith('.wav') || name.endsWith('.mp3')) return { Icon: FileAudio, color: '#ffa657' };
  if (name.endsWith('.ui') || name.endsWith('.json')) return { Icon: FileText, color: '#a5d6ff' };
  if (name.endsWith('.csproj')) return { Icon: FileText, color: '#e3b341' };
  return { Icon: FileText, color: '#9aa0aa' };
}

export default function FilesPanel() {
  const { fileTree, folderOrder, rootFileOrder, openTab, pushLog, activeTab, togglePanel } = useStore();
  const [expanded, setExpanded] = useState(() => new Set(['res://']));
  const [query, setQuery] = useState('');

  const toggle = (path) =>
    setExpanded((prev) => {
      const next = new Set(prev);
      if (next.has(path)) next.delete(path);
      else next.add(path);
      return next;
    });

  const sortDirs = (dirsMap) => {
    const names = [...dirsMap.keys()];
    names.sort((a, b) => {
      const ia = folderOrder.indexOf(a);
      const ib = folderOrder.indexOf(b);
      if (ia !== -1 || ib !== -1) return (ia === -1 ? 99 : ia) - (ib === -1 ? 99 : ib);
      return a.localeCompare(b);
    });
    return names;
  };

  const sortFiles = (list) => {
    return [...list].sort((a, b) => {
      const ia = rootFileOrder.indexOf(a.name);
      const ib = rootFileOrder.indexOf(b.name);
      if (ia !== -1 || ib !== -1) return (ia === -1 ? 99 : ia) - (ib === -1 ? 99 : ib);
      return a.name.localeCompare(b.name);
    });
  };

  const matches = (name) => !query || name.toLowerCase().includes(query.toLowerCase());

  const renderDir = (name, node, depth, basePath) => {
    const path = `${basePath}/${name}`;
    const isOpen = expanded.has(path) || !!query;
    const kids = sortDirs(node.dirs);
    const kidFiles = sortFiles(node.files);
    if (!matches(name) && !kids.some((k) => matches(k)) && !kidFiles.some((f) => matches(f.name))) return null;

    return (
      <div key={path}>
        <div className="ex-row" style={{ paddingLeft: 6 + depth * 16 }} onClick={() => toggle(path)}>
          <button className="ex-chev">{isOpen ? <ChevronDown size={13} /> : <ChevronRight size={13} />}</button>
          <span className="ex-ico" style={{ color: '#9aa0aa' }}>
            {isOpen ? <FolderOpen size={15} /> : <Folder size={15} />}
          </span>
          <span className="ex-label">{name}</span>
        </div>
        {isOpen && (
          <>
            {kids.map((k) => renderDir(k, node.dirs.get(k), depth + 1, path))}
            {sortFiles(node.files).map((f) => renderFile(f, depth + 1, path))}
          </>
        )}
      </div>
    );
  };

  const renderFile = (file, depth, basePath) => {
    if (!matches(file.name)) return null;
    const { Icon, color } = iconFor(file.name);
    const active = activeTab === file.path;
    return (
      <div
        key={file.path}
        className={`ex-row ${active ? 'selected' : ''}`}
        style={{ paddingLeft: 6 + depth * 16 }}
        onClick={() => openTab(file.path)}
        onDoubleClick={() => pushLog('INFO', `Editando ${file.path}`)}
      >
        <button className="ex-chev ghost" />
        <span className="ex-ico" style={{ color }}>
          <Icon size={15} strokeWidth={1.8} />
        </span>
        <span className="ex-label">{file.name}</span>
      </div>
    );
  };

  const rootDirs = sortDirs(fileTree.dirs);
  const rootFiles = sortFiles(fileTree.files);
  const rootOpen = expanded.has('res://') || !!query;

  return (
    <div className="ed-panel files-panel">
      <div className="panel-head">
        <span className="panel-title"><FolderTree size={14} /> Arquivos</span>
        <div className="panel-head-actions">
          <button title="Fixar"><Pin size={13} /></button>
          <button title="Fechar" onClick={() => togglePanel('files')}><X size={13} /></button>
        </div>
      </div>
      <div className="panel-search">
        <Search size={14} />
        <input placeholder="Pesquisar arquivos..." value={query} onChange={(e) => setQuery(e.target.value)} />
      </div>
      <div className="panel-scroll">
        <div className="ex-row" style={{ paddingLeft: 6 }} onClick={() => toggle('res://')}>
          <button className="ex-chev">{rootOpen ? <ChevronDown size={13} /> : <ChevronRight size={13} />}</button>
          <span className="ex-ico" style={{ color: '#e3b341' }}>
            <FolderOpen size={15} />
          </span>
          <span className="ex-label">res://</span>
        </div>
        {rootOpen && (
          <>
            {rootDirs.map((d) => renderDir(d, fileTree.dirs.get(d), 1, 'res://'))}
            {rootFiles.map((f) => renderFile(f, 1, 'res://'))}
          </>
        )}
      </div>
    </div>
  );
}
