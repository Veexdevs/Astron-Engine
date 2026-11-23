import React, { useState } from 'react';
import { useStore } from '../store.jsx';
import { EXPLORER_TREE } from '../data/seed.js';
import {
  ChevronDown, ChevronRight, Search, Pin, X, Box, Folder, Mountain, Lightbulb,
  Camera, User, Activity, Music, Monitor, Code, Circle, File, FileCode, Boxes, Files,
} from 'lucide-react';

const ICONS = {
  box: Box,
  folder: Folder,
  scene: Box,
  scene2: File,
  objects: Boxes,
  mountain: Mountain,
  bulb: Lightbulb,
  camera: Camera,
  user: User,
  activity: Activity,
  music: Music,
  monitor: Monitor,
  code: Code,
  circle: Circle,
  filecode: FileCode,
  file: File,
  files: Files,
};

export default function ExplorerPanel() {
  const { buildTree, selection, setSelection, pushLog, openTab, togglePanel } = useStore();
  const [expanded, setExpanded] = useState(() => new Set(['projeto', 'cenas', 'mundo-3d']));
  const [query, setQuery] = useState('');

  const tree = buildTree(EXPLORER_TREE);

  const toggle = (id) => {
    setExpanded((prev) => {
      const next = new Set(prev);
      if (next.has(id)) next.delete(id);
      else next.add(id);
      return next;
    });
  };

  const hit = (n) => !query || n.label.toLowerCase().includes(query.toLowerCase());

  const renderNode = (node, depth) => {
    const hasKids = Array.isArray(node.children);
    const isOpen = expanded.has(node.id);
    const selected = selection?.id === node.id;
    const Icon = ICONS[node.icon] || File;
    const visible = hit(node) || (hasKids && node.children.some(hit));
    if (!visible) return null;

    return (
      <div key={node.id}>
        <div
          className={`ex-row ${selected ? 'selected' : ''}`}
          style={{ paddingLeft: 6 + depth * 16 }}
          onClick={() =>
            setSelection({
              id: node.id,
              label: node.label,
              kind: node.kind || (node.path ? 'file' : 'node'),
              props: node.path ? 'file' : node.kind === 'scene' ? 'model' : 'node',
              path: node.path,
            })
          }
          onDoubleClick={() => {
            if (node.path) openTab(node.path);
            else if (hasKids) toggle(node.id);
            else pushLog('INFO', `Abrindo ${node.label}…`);
          }}
        >
          <button
            className={`ex-chev ${hasKids ? '' : 'ghost'}`}
            onClick={(e) => { e.stopPropagation(); if (hasKids) toggle(node.id); }}
          >
            {hasKids && (isOpen ? <ChevronDown size={13} /> : <ChevronRight size={13} />)}
          </button>
          <span className="ex-ico" style={{ color: node.color || '#9aa0aa' }}>
            <Icon size={15} strokeWidth={1.8} />
          </span>
          <span className="ex-label">{node.label}</span>
        </div>
        {hasKids && isOpen && node.children.map((c) => renderNode(c, depth + 1))}
      </div>
    );
  };

  return (
    <div className="ed-panel ex-panel">
      <div className="panel-head">
        <span className="panel-title"><Files size={14} /> Explorer</span>
        <div className="panel-head-actions">
          <button title="Fixar"><Pin size={13} /></button>
          <button title="Fechar" onClick={() => togglePanel('explorer')}><X size={13} /></button>
        </div>
      </div>
      <div className="panel-search">
        <Search size={14} />
        <input placeholder="Pesquisar no Explorer..." value={query} onChange={(e) => setQuery(e.target.value)} />
      </div>
      <div className="panel-scroll ex-tree">{tree.map((n) => renderNode(n, 0))}</div>
    </div>
  );
}
