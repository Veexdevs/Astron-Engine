import React from 'react';
import { Minus, Square, X } from 'lucide-react';

// Controles de janela falsos (— □ ×) no canto superior direito
export default function WindowChrome({ onMin, onMax, onClose, style }) {
  return (
    <div className="win-controls" style={{ position: 'absolute', top: 10, right: 14, ...style }}>
      <button title="Minimizar" onClick={onMin}>
        <Minus size={15} strokeWidth={1.8} />
      </button>
      <button title="Maximizar" onClick={onMax}>
        <Square size={11} strokeWidth={2.2} />
      </button>
      <button title="Fechar" className="close" onClick={onClose}>
        <X size={15} strokeWidth={2} />
      </button>
    </div>
  );
}
