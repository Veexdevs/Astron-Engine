import React from 'react';

// Marca do Astron Engine: planeta com anel (SVG fiel ao logo)
export function PlanetMark({ size = 64, className = '' }) {
  const uid = React.useId();
  return (
    <svg
      className={`logo-mark ${className}`}
      width={size}
      height={size}
      viewBox="0 0 100 100"
      fill="none"
      aria-hidden="true"
    >
      <defs>
        {/* metade inferior (anel passando na frente) */}
        <clipPath id={`${uid}-front`}>
          <polygon points="0,73.3 100,26.7 100,220 0,220" />
        </clipPath>
      </defs>
      {/* anel atrás */}
      <g>
        <ellipse cx="50" cy="50" rx="45" ry="16" transform="rotate(-25 50 50)" stroke="#000" strokeWidth="11" />
        <ellipse cx="50" cy="50" rx="45" ry="16" transform="rotate(-25 50 50)" stroke="#fff" strokeWidth="6" />
      </g>
      {/* planeta */}
      <circle cx="50" cy="50" r="25.5" fill="#fff" stroke="#000" strokeWidth="3" />
      {/* anel na frente (parte inferior) */}
      <g clipPath={`url(#${uid}-front)`}>
        <ellipse cx="50" cy="50" rx="45" ry="16" transform="rotate(-25 50 50)" stroke="#000" strokeWidth="11" />
        <ellipse cx="50" cy="50" rx="45" ry="16" transform="rotate(-25 50 50)" stroke="#fff" strokeWidth="6" />
      </g>
    </svg>
  );
}

// Logo horizontal completo (marca + wordmark)
export function LogoFull({ size = 64, wordSize, subSize, gap = 18, stacked = false, tagline }) {
  const w = wordSize || size * 1.15;
  return (
    <div
      style={{
        display: 'flex',
        flexDirection: stacked ? 'column' : 'row',
        alignItems: 'center',
        gap: stacked ? 10 : gap,
      }}
    >
      <PlanetMark size={size} />
      <div style={{ textAlign: 'center' }}>
        <div className="logo-word" style={{ fontSize: w }}>Astron</div>
        <div
          className="logo-word"
          style={{
            fontSize: Math.max(11, w * 0.34),
            letterSpacing: '0.55em',
            marginTop: Math.max(4, w * 0.1),
            paddingLeft: '0.55em',
            fontWeight: 400,
          }}
        >
          Engine
        </div>
        {tagline && (
          <div
            style={{
              marginTop: 14,
              fontSize: Math.max(11, w * 0.16),
              letterSpacing: '0.42em',
              paddingLeft: '0.42em',
              color: '#e8e8ea',
              fontWeight: 400,
            }}
          >
            {tagline}
          </div>
        )}
      </div>
    </div>
  );
}

export default LogoFull;
