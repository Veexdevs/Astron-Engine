import React, { useEffect, useRef, useState } from 'react';
import { useStore } from '../store.jsx';
import {
  MousePointer2, Move, RotateCw, Maximize2, Frame, Box, Camera, Lightbulb, Mountain,
  Puzzle, Play, Pause, Square, Monitor, Settings, ChevronDown, Plus, Circle, Grid2x2, Lightbulb as Bulb2,
} from 'lucide-react';

const TOOLS = [
  { id: 'select', label: 'Selecionar', icon: MousePointer2 },
  { id: 'move', label: 'Mover', icon: Move },
  { id: 'rotate', label: 'Rotacionar', icon: RotateCw },
  { id: 'scale', label: 'Escalar', icon: Maximize2 },
  { id: 'transform', label: 'Transformar', icon: Frame },
];

const CREATE_MENU = [
  { label: 'Cubo', icon: Box },
  { label: 'Esfera', icon: Circle },
  { label: 'Cilindro', icon: CylinderIcon },
  { label: 'Plano', icon: Grid2x2 },
  { label: 'Câmera', icon: Camera },
  { label: 'Luz', icon: Lightbulb },
];

function CylinderIcon({ size = 18 }) {
  return (
    <svg width={size} height={size} viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="1.8">
      <ellipse cx="12" cy="6" rx="7" ry="3" />
      <path d="M5 6v12c0 1.7 3.1 3 7 3s7-1.3 7-3V6" />
      <path d="M5 12c0 1.7 3.1 3 7 3s7-1.3 7-3" />
    </svg>
  );
}

export default function Toolbar({ onSettings }) {
  const {
    tool, setTool, mode, setMode, playing, setPlaying,
    addSceneObject, pushLog, pushToast, panels, togglePanel,
  } = useStore();
  const [createOpen, setCreateOpen] = useState(false);
  const [viewOpen, setViewOpen] = useState(false);
  const wrap = useRef(null);

  useEffect(() => {
    const close = (e) => {
      if (wrap.current && !wrap.current.contains(e.target)) {
        setCreateOpen(false);
        setViewOpen(false);
      }
    };
    document.addEventListener('mousedown', close);
    return () => document.removeEventListener('mousedown', close);
  }, []);

  const play = () => {
    setPlaying(true);
    pushLog('INFO', '▶ Jogo iniciado — executando Update/Render a 60 FPS.');
  };
  const pause = () => {
    if (!playing) return pushToast('O jogo não está rodando.');
    pushLog('INFO', '⏸ Jogo pausado.');
  };
  const stop = () => {
    setPlaying(false);
    pushLog('INFO', '■ Jogo parado.');
  };

  return (
    <div className="ed-toolbar" ref={wrap}>
      <div className="tb-group">
        {TOOLS.map((t) => (
          <button
            key={t.id}
            className={`tb-btn ${tool === t.id ? 'active' : ''}`}
            onClick={() => setTool(t.id)}
            title={t.label}
          >
            <t.icon size={19} strokeWidth={1.9} />
            <span>{t.label}</span>
          </button>
        ))}
      </div>

      <div className="tb-sep" />

      <div className="tb-group">
        <div className="tb-btn-wrap">
          <button className={`tb-btn ${createOpen ? 'active' : ''}`} onClick={() => { setCreateOpen((v) => !v); setViewOpen(false); }}>
            <Box size={19} strokeWidth={1.9} />
            <span>Criar <ChevronDown size={11} style={{ verticalAlign: -1 }} /></span>
          </button>
          {createOpen && (
            <div className="tb-dropdown">
              {CREATE_MENU.map((c) => (
                <button
                  key={c.label}
                  onClick={() => { addSceneObject(c.label); setCreateOpen(false); }}
                >
                  <c.icon size={15} /> {c.label}
                </button>
              ))}
            </div>
          )}
        </div>

        <button className="tb-btn" title="Câmera" onClick={() => { addSceneObject('Câmera'); }}>
          <Camera size={19} strokeWidth={1.9} />
          <span>Câmera</span>
        </button>
        <button className="tb-btn" title="Iluminação" onClick={() => { addSceneObject('Iluminação'); }}>
          <Lightbulb size={19} strokeWidth={1.9} />
          <span>Iluminação</span>
        </button>
        <button className="tb-btn" title="Terreno" onClick={() => { addSceneObject('Terreno'); }}>
          <Mountain size={19} strokeWidth={1.9} />
          <span>Terreno</span>
        </button>
        <button className="tb-btn" title="Componente" onClick={() => { addSceneObject('Componente'); }}>
          <Puzzle size={19} strokeWidth={1.9} />
          <span>Componente</span>
        </button>
      </div>

      <div className="tb-sep" />

      <div className="tb-group">
        <div className="tb-2d3d">
          <button className={mode === '2d' ? 'active' : ''} onClick={() => { setMode('2d'); pushLog('INFO', 'Viewport 2D (vista superior).'); }}>2D</button>
          <button className={mode === '3d' ? 'active' : ''} onClick={() => { setMode('3d'); pushLog('INFO', 'Viewport 3D (perspectiva).'); }}>3D</button>
        </div>
      </div>

      <div className="tb-sep" />

      <div className="tb-group">
        <button className={`tb-btn ${playing ? 'active' : ''}`} onClick={play} title="Jogar">
          <Play size={19} strokeWidth={1.9} />
          <span>Jogar</span>
        </button>
        <button className="tb-btn" onClick={pause} title="Pausar">
          <Pause size={19} strokeWidth={1.9} />
          <span>Pausar</span>
        </button>
        <button className="tb-btn" onClick={stop} title="Parar">
          <Square size={17} strokeWidth={2} fill="currentColor" />
          <span>Parar</span>
        </button>
      </div>

      <div className="tb-spacer" />

      <div className="tb-group">
        <div className="tb-btn-wrap">
          <button className={`tb-btn ${viewOpen ? 'active' : ''}`} onClick={() => { setViewOpen((v) => !v); setCreateOpen(false); }}>
            <Monitor size={19} strokeWidth={1.9} />
            <span>Visualização <ChevronDown size={11} style={{ verticalAlign: -1 }} /></span>
          </button>
          {viewOpen && (
            <div className="tb-dropdown right">
              {['Perspectiva', 'Ortográfica', 'Câmera Livre', 'Wireframe'].map((v) => (
                <button key={v} onClick={() => { pushLog('INFO', `Modo de visualização: ${v}`); setViewOpen(false); }}>
                  {v}
                </button>
              ))}
            </div>
          )}
        </div>
        <button className="tb-btn" onClick={onSettings} title="Configurações">
          <Settings size={19} strokeWidth={1.9} />
          <span>Configurações</span>
        </button>
      </div>
    </div>
  );
}
