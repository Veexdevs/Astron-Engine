import React from 'react';

// Toggle estilo iOS (azul quando ativo)
export default function Toggle({ checked, onChange, size = 1 }) {
  return (
    <button
      type="button"
      role="switch"
      aria-checked={checked}
      onClick={() => onChange(!checked)}
      style={{
        width: 46 * size,
        height: 27 * size,
        borderRadius: 999,
        background: checked ? '#0a84ff' : '#2a2b31',
        position: 'relative',
        transition: 'background 0.16s ease',
        flexShrink: 0,
      }}
    >
      <span
        style={{
          position: 'absolute',
          top: 3 * size,
          left: checked ? 46 * size - 24 * size : 3 * size,
          width: 21 * size,
          height: 21 * size,
          borderRadius: '50%',
          background: '#fff',
          transition: 'left 0.16s ease',
          boxShadow: '0 1px 3px rgba(0,0,0,0.4)',
        }}
      />
    </button>
  );
}
