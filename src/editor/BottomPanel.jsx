import React, { useEffect, useRef, useState } from 'react';
import { useStore } from '../store.jsx';
import AIAgent from './AIAgent.jsx';
import {
  Terminal, FileText, Bug, Volume2, Film, Code2, Sparkles, Filter, Trash2,
  XCircle, AlertTriangle, Info, Pin, Send, X,
} from 'lucide-react';

const TABS = [
  { id: 'console', label: 'Console', icon: Terminal },
  { id: 'saida', label: 'Saída', icon: FileText },
  { id: 'depurador', label: 'Depurador', icon: Bug },
  { id: 'audio', label: 'Áudio', icon: Volume2 },
  { id: 'animacao', label: 'Animação', icon: Film },
  { id: 'shader', label: 'Editor de Shader', icon: Code2 },
  { id: 'ia', label: 'Agente IA', icon: Sparkles },
];

export default function BottomPanel({ tab, setTab }) {
  const { logs, clearLogs, pushLog, togglePanel } = useStore();
  const [input, setInput] = useState('');
  const scrollRef = useRef(null);

  useEffect(() => {
    if (scrollRef.current) scrollRef.current.scrollTop = scrollRef.current.scrollHeight;
  }, [logs, tab]);

  const send = (e) => {
    e.preventDefault();
    if (!input.trim()) return;
    pushLog('INFO', `> ${input.trim()}`);
    setInput('');
  };

  const errors = logs.filter((l) => l.level === 'ERROR').length;
  const warns = logs.filter((l) => l.level === 'WARNING').length;
  const infos = logs.filter((l) => l.level === 'INFO').length;

  return (
    <div className="ed-bottom">
      <div className="ed-bottom-tabs">
        {TABS.map((t) => (
          <button
            key={t.id}
            className={`btab ${tab === t.id ? 'active' : ''}`}
            onClick={() => setTab(t.id)}
          >
            <t.icon size={14} />
            {t.label}
          </button>
        ))}
        <div className="btab-spacer" />
        {tab === 'console' && (
          <div className="btab-counts">
            <span className="cnt red"><XCircle size={14} /> {errors}</span>
            <span className="cnt yellow"><AlertTriangle size={14} /> {warns}</span>
            <span className="cnt blue"><Info size={14} /> {infos}</span>
            <button title="Filtrar"><Filter size={14} /></button>
            <button title="Limpar" onClick={clearLogs}><Trash2 size={14} /></button>
          </div>
        )}
        <button className="btab-pin" title="Fixar"><Pin size={13} /></button>
        <button className="btab-pin" title="Fechar" onClick={() => togglePanel('bottom')}><X size={13} /></button>
      </div>

      {tab === 'console' && (
        <>
          <div className="console-scroll" ref={scrollRef}>
            {logs.map((l, i) => (
              <div className="log-line" key={i}>
                <span className={`log-tag ${l.level.toLowerCase()}`}>[{l.level}]</span>
                <span className={`log-msg ${l.level.toLowerCase()}`}>{l.msg}</span>
              </div>
            ))}
            {!logs.length && <div className="empty-pane">Console limpo.</div>}
          </div>
          <form className="console-input" onSubmit={send}>
            <input
              placeholder="Digite uma mensagem..."
              value={input}
              onChange={(e) => setInput(e.target.value)}
            />
            <button type="submit" title="Enviar"><Send size={15} /></button>
          </form>
        </>
      )}

      {tab === 'ia' && <AIAgent />}

      {tab !== 'console' && tab !== 'ia' && (
        <div className="empty-pane big">
          {tab === 'saida' && 'Nada na saída — execute o jogo para ver os logs de compilação.'}
          {tab === 'depurador' && 'Depurador pronto. Inicie o jogo com depuração habilitada.'}
          {tab === 'audio' && 'Mixer de áudio — nenhum canal ativo.'}
          {tab === 'animacao' && 'Nenhuma animação aberta. Crie uma em Biblioteca → Animações.'}
          {tab === 'shader' && 'Editor de Shader GLSL — selecione um material para editar.'}
        </div>
      )}
    </div>
  );
}
