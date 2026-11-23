import React, { useState } from 'react';
import { useStore } from '../store.jsx';
import { TEMPLATE_OPTIONS } from '../data/seed.js';
import { Box, Hammer, Gamepad2, Grid2x2 } from 'lucide-react';

const ICONS = { blank: Box, platformer: Hammer, obby: Gamepad2, topdown: Grid2x2 };

export default function NewProjectModal() {
  const { setNewProjectOpen, createProject } = useStore();
  const [name, setName] = useState('Meu Novo Jogo');
  const [template, setTemplate] = useState('platformer');

  const create = (e) => {
    e?.preventDefault();
    if (!name.trim()) return;
    createProject({ name: name.trim(), template });
  };

  return (
    <div className="modal-overlay" onClick={() => setNewProjectOpen(false)}>
      <div className="modal" onClick={(e) => e.stopPropagation()}>
        <div className="modal-title-row">
          <h2>Novo Projeto</h2>
          <button className="modal-x" onClick={() => setNewProjectOpen(false)}>✕</button>
        </div>
        <p className="muted" style={{ fontSize: 13.5, marginBottom: 18 }}>
          Escolha um nome e um template. Os jogos são programados em <b style={{ color: '#fff' }}>C#</b>.
        </p>

        <form onSubmit={create}>
          <div className="field">
            <label>Nome do projeto</label>
            <input value={name} onChange={(e) => setName(e.target.value)} autoFocus />
          </div>

          <div className="field">
            <label>Template</label>
            <div className="tpl-grid">
              {TEMPLATE_OPTIONS.map((t) => {
                const Icon = ICONS[t.id] || Box;
                return (
                  <button
                    type="button"
                    key={t.id}
                    className={`tpl ${template === t.id ? 'active' : ''}`}
                    onClick={() => setTemplate(t.id)}
                  >
                    <Icon size={20} />
                    <b>{t.label}</b>
                    <span>{t.desc}</span>
                  </button>
                );
              })}
            </div>
          </div>

          <div style={{ display: 'flex', gap: 10, justifyContent: 'flex-end', marginTop: 20 }}>
            <button type="button" className="btn-outline" onClick={() => setNewProjectOpen(false)}>
              Cancelar
            </button>
            <button type="submit" className="btn-white" style={{ padding: '0 26px', height: 44 }}>
              Criar Projeto
            </button>
          </div>
        </form>
      </div>
    </div>
  );
}
