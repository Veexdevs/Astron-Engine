import React, { useState } from 'react';
import { useStore } from '../store.jsx';
import { Wrench, X, Pin } from 'lucide-react';

const SHAPES = [
  { id: 'Part', svg: CubeSVG },
  { id: 'Sphere', svg: SphereSVG },
  { id: 'Cylinder', svg: CylinderSVG },
  { id: 'Block', svg: BlockSVG },
];

export default function ToolboxPanel() {
  const { addSceneObject, togglePanel } = useStore();
  const [tab, setTab] = useState('Modelos');

  return (
    <div className="ed-panel tb-panel">
      <div className="panel-head">
        <span className="panel-title"><Wrench size={14} /> Caixa de Ferramentas</span>
        <div className="panel-head-actions">
          <button title="Fixar"><Pin size={13} /></button>
          <button title="Fechar" onClick={() => togglePanel('toolbox')}><X size={13} /></button>
        </div>
      </div>

      <div className="tb-tabs">
        {['Modelos', 'Recursos', 'Materiais'].map((t) => (
          <button key={t} className={tab === t ? 'active' : ''} onClick={() => setTab(t)}>
            {t}
          </button>
        ))}
      </div>

      <div className="tb-items">
        {tab === 'Modelos' &&
          SHAPES.map((s) => (
            <button key={s.id} className="tb-item" onClick={() => addSceneObject(s.id)} title={`Adicionar ${s.id}`}>
              <s.svg />
              <span>{s.id}</span>
            </button>
          ))}
        {tab === 'Recursos' &&
          ['Sprite', 'Audio', 'Fonte', 'Material'].map((s) => (
            <button key={s} className="tb-item" onClick={() => addSceneObject(s)}>
              <div className="ph-icon">{s[0]}</div>
              <span>{s}</span>
            </button>
          ))}
        {tab === 'Materiais' &&
          ['Plastic', 'Metal', 'Glass', 'Neon'].map((s) => (
            <button key={s} className="tb-item" onClick={() => addSceneObject(s)}>
              <div className={`mat-icon ${s.toLowerCase()}`} />
              <span>{s}</span>
            </button>
          ))}
      </div>
    </div>
  );
}

function CubeSVG() {
  return (
    <svg width="56" height="56" viewBox="0 0 64 64">
      <path d="M32 6l24 13v26L32 58 8 45V19L32 6Z" fill="#d9dade" />
      <path d="M32 6l24 13-24 13L8 19 32 6Z" fill="#f1f1f4" />
      <path d="M32 32v26L8 45V19l24 13Z" fill="#b9bac1" />
      <path d="M32 32v26l24-13V19L32 32Z" fill="#cfd0d6" />
    </svg>
  );
}
function SphereSVG() {
  return (
    <svg width="56" height="56" viewBox="0 0 64 64">
      <defs>
        <radialGradient id="sph" cx="35%" cy="30%" r="75%">
          <stop offset="0%" stopColor="#ffffff" />
          <stop offset="55%" stopColor="#d4d5da" />
          <stop offset="100%" stopColor="#8f9097" />
        </radialGradient>
      </defs>
      <circle cx="32" cy="32" r="26" fill="url(#sph)" />
    </svg>
  );
}
function CylinderSVG() {
  return (
    <svg width="56" height="56" viewBox="0 0 64 64">
      <defs>
        <linearGradient id="cyl" x1="0" y1="0" x2="1" y2="0">
          <stop offset="0%" stopColor="#9c9da4" />
          <stop offset="35%" stopColor="#e8e8ec" />
          <stop offset="100%" stopColor="#a9aab1" />
        </linearGradient>
      </defs>
      <path d="M16 16h32v32H16z" fill="url(#cyl)" />
      <ellipse cx="32" cy="16" rx="16" ry="6" fill="#f2f2f5" />
      <ellipse cx="32" cy="48" rx="16" ry="6" fill="#b7b8bf" />
    </svg>
  );
}
function BlockSVG() {
  return (
    <svg width="56" height="56" viewBox="0 0 64 64">
      <path d="M8 26l24-12 24 12v14L32 52 8 40V26Z" fill="#c7c8ce" />
      <path d="M8 26l24 12V52L8 40V26Z" fill="#a6a7ae" />
      <path d="M56 26L32 38v14l24-12V26Z" fill="#b7b8bf" />
      <path d="M8 26l24-12 24 12-24 12L8 26Z" fill="#eceded" />
    </svg>
  );
}
