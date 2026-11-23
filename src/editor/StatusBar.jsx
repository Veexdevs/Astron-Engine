import React, { useEffect, useState } from 'react';
import { useStore } from '../store.jsx';

export default function StatusBar() {
  const { playing, currentProject } = useStore();
  const [fps, setFps] = useState(144);
  const [mem, setMem] = useState(842);

  useEffect(() => {
    const t = setInterval(() => {
      setFps(140 + Math.floor(Math.random() * 9)); // ~140-148
      setMem(836 + Math.floor(Math.random() * 14));
    }, 700);
    return () => clearInterval(t);
  }, []);

  return (
    <div className="ed-status">
      <div className="st-left">
        <i className={`dot ${playing ? 'yellow' : 'green'}`} />
        <span>{playing ? 'Executando…' : 'Pronto'}</span>
        {currentProject && <span className="st-proj">· {currentProject.name}</span>}
      </div>
      <div className="st-right">
        <span>FPS: {playing ? Math.min(fps, 60) : fps}</span>
        <span className="st-sep">|</span>
        <span>Memória: {mem} MB</span>
        <span className="st-sep">|</span>
        <span>Versão: 0.1.0</span>
      </div>
    </div>
  );
}
