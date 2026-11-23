import React, { useState } from 'react';
import { useStore } from '../store.jsx';
import {
  Search, X, Pin, ChevronDown, ChevronRight, Settings as Cog, Pipette, PanelRight,
} from 'lucide-react';
import Toggle from '../components/Toggle.jsx';

function Section({ title, children, defaultOpen = true, icon = Cog }) {
  const [open, setOpen] = useState(defaultOpen);
  const Icon = icon;
  return (
    <div className="pr-section">
      <button className="pr-sec-head" onClick={() => setOpen((v) => !v)}>
        {open ? <ChevronDown size={14} /> : <ChevronRight size={14} />}
        <Icon size={14} className="pr-sec-ico" />
        <span>{title}</span>
      </button>
      {open && <div className="pr-sec-body">{children}</div>}
    </div>
  );
}

function Vec3({ label, values, onChange, step = 0.001 }) {
  const axes = ['X', 'Y', 'Z'];
  return (
    <div className="pr-vec">
      <span className="pr-vec-label">{label}</span>
      <div className="pr-vec-inputs">
        {axes.map((a, i) => (
          <label key={a} className="pr-num">
            <b>{a}</b>
            <input
              type="number"
              step={step}
              value={values[i]}
              onChange={(e) => {
                const next = [...values];
                next[i] = e.target.value;
                onChange(next);
              }}
            />
          </label>
        ))}
      </div>
    </div>
  );
}

export default function PropertiesPanel() {
  const { selection, pushLog, togglePanel } = useStore();
  const [pos, setPos] = useState(['0.000', '0.000', '0.000']);
  const [rot, setRot] = useState(['0.000', '0.000', '0.000']);
  const [scl, setScl] = useState(['1.000', '1.000', '1.000']);
  const [color, setColor] = useState('#FFFFFF');
  const [material, setMaterial] = useState('Plastic');
  const [alpha, setAlpha] = useState('0.000');
  const [visible, setVisible] = useState(true);
  const [mass, setMass] = useState('1.000');
  const [gravity, setGravity] = useState(true);
  const [collision, setCollision] = useState('Ativado');

  const title = selection?.props === 'model' ? 'Parte: Model' : selection?.label || 'Nenhum objeto';

  return (
    <div className="ed-panel pr-panel">
      <div className="panel-head">
        <span className="panel-title"><PanelRight size={14} /> Propriedades</span>
        <div className="panel-head-actions">
          <button title="Fixar"><Pin size={13} /></button>
          <button title="Fechar" onClick={() => togglePanel('properties')}><X size={13} /></button>
        </div>
      </div>

      <div className="panel-search">
        <Search size={14} />
        <input placeholder="Pesquisar propriedades..." />
      </div>

      <div className="pr-title">
        <span>{title}</span>
        <button title="Remover"><X size={14} /></button>
      </div>

      <div className="panel-scroll pr-scroll">
        <Section title="Transformação">
          <Vec3 label="Posição" values={pos} onChange={setPos} />
          <Vec3 label="Rotação" values={rot} onChange={setRot} />
          <Vec3 label="Escala" values={scl} onChange={setScl} step={0.01} />
        </Section>

        <Section title="Aparência">
          <div className="pr-row">
            <span className="pr-vec-label">Cor</span>
            <div className="pr-color">
              <label className="swatch" style={{ background: color }}>
                <input type="color" value={color} onChange={(e) => setColor(e.target.value.toUpperCase())} />
              </label>
              <input className="pr-hex" value={color} onChange={(e) => setColor(e.target.value)} />
              <button className="pr-eyedrop" title="Conta-gotas" onClick={() => pushLog('INFO', 'Conta-gotas ativo.')}>
                <Pipette size={15} />
              </button>
            </div>
          </div>
          <div className="pr-row">
            <span className="pr-vec-label">Material</span>
            <select className="pr-select" value={material} onChange={(e) => setMaterial(e.target.value)}>
              {['Plastic', 'Metal', 'Wood', 'Glass', 'Neon', 'Matte'].map((m) => (
                <option key={m}>{m}</option>
              ))}
            </select>
          </div>
          <div className="pr-row">
            <span className="pr-vec-label">Transparência</span>
            <div className="pr-slider">
              <input
                type="range"
                min="0"
                max="1"
                step="0.001"
                value={alpha}
                onChange={(e) => setAlpha(Number(e.target.value).toFixed(3))}
              />
              <span>{alpha}</span>
            </div>
          </div>
          <div className="pr-row">
            <span className="pr-vec-label">Visibilidade</span>
            <div className="pr-slider just-end">
              <Toggle checked={visible} onChange={(v) => { setVisible(v); pushLog('INFO', `Visibilidade: ${v ? 'ligada' : 'desligada'}`); }} />
            </div>
          </div>
        </Section>

        <Section title="Física">
          <div className="pr-row">
            <span className="pr-vec-label">Massa</span>
            <input className="pr-input" type="number" step="0.001" value={mass} onChange={(e) => setMass(e.target.value)} />
          </div>
          <div className="pr-row">
            <span className="pr-vec-label">Gravidade</span>
            <div className="pr-slider just-end">
              <Toggle checked={gravity} onChange={setGravity} />
            </div>
          </div>
          <div className="pr-row">
            <span className="pr-vec-label">Colisão</span>
            <select className="pr-select" value={collision} onChange={(e) => setCollision(e.target.value)}>
              <option>Ativado</option>
              <option>Desativado</option>
              <option>Trigger</option>
            </select>
          </div>
        </Section>

        <Section title="Renderização" defaultOpen={false}>
          <div className="pr-row">
            <span className="pr-vec-label">Sombra</span>
            <select className="pr-select" defaultValue="Suave"><option>Suave</option><option>Dura</option><option>Nenhuma</option></select>
          </div>
          <div className="pr-row">
            <span className="pr-vec-label">Layer</span>
            <input className="pr-input" defaultValue="Default" />
          </div>
        </Section>

        <Section title="Outras Configurações" defaultOpen={false}>
          <div className="pr-row">
            <span className="pr-vec-label">Tag</span>
            <input className="pr-input" defaultValue="Player" />
          </div>
          <div className="pr-row">
            <span className="pr-vec-label">Script</span>
            <input className="pr-input" defaultValue="res://scripts/Player.cs" />
          </div>
        </Section>
      </div>
    </div>
  );
}
