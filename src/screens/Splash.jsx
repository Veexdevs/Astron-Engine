import React, { useEffect, useRef, useState } from 'react';
import { useStore } from '../store.jsx';
import { LogoFull } from '../components/Logo.jsx';
import WindowChrome from '../components/WindowChrome.jsx';

// ============ Tela 1 — Splash / Carregando ============
export default function Splash() {
  const { setScreen } = useStore();
  const [progress, setProgress] = useState(0);
  const timer = useRef(null);

  useEffect(() => {
    let p = 0;
    timer.current = setInterval(() => {
      p += Math.random() * 9 + 4;
      if (p >= 100) {
        p = 100;
        clearInterval(timer.current);
        setTimeout(() => setScreen('login'), 420);
      }
      setProgress(p);
    }, 130);
    return () => clearInterval(timer.current);
  }, [setScreen]);

  return (
    <div className="splash screen">
      <div className="orb orb-tl" />
      <div className="orb orb-br" />
      <WindowChrome onMin={() => {}} onMax={() => {}} onClose={() => {}} />

      <div className="splash-center">
        <LogoFull size={168} wordSize={78} />
        <div className="splash-bar">
          <div className="splash-fill" style={{ width: `${Math.min(100, progress)}%` }} />
        </div>
        <div className="splash-text">Carregando...</div>
      </div>
    </div>
  );
}
