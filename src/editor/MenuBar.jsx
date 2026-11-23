import React, { useEffect, useRef, useState } from 'react';
import { useStore } from '../store.jsx';
import {
  ChevronDown, Save, FolderOpen, FilePlus2, Undo2, Redo2, Scissors, Copy, ClipboardPaste,
  Eye, Gamepad2, Bug, BookOpen, HelpCircle, Hammer, Play, Square, Settings as Cog,
  Sparkles, Code, Package, Film, Check, Maximize,
} from 'lucide-react';

const MENUS = [
  {
    id: 'arquivo', label: 'Arquivo',
    items: [
      { label: 'Novo Projeto…', action: 'novo' },
      { label: 'Abrir Projeto…', action: 'abrir' },
      { sep: true },
      { label: 'Salvar', shortcut: 'Ctrl+S', action: 'salvar', icon: Save },
      { label: 'Salvar Cena Como…', action: 'salvarcomo' },
      { sep: true },
      { label: 'Buildar Projeto…', shortcut: 'Ctrl+B', action: 'build', icon: Hammer },
      { sep: true },
      { label: 'Voltar ao Hub', action: 'hub' },
      { label: 'Sair', action: 'sair' },
    ],
  },
  {
    id: 'editar', label: 'Editar',
    items: [
      { label: 'Desfazer', shortcut: 'Ctrl+Z', action: 'log:Desfazer (demo)', icon: Undo2 },
      { label: 'Refazer', shortcut: 'Ctrl+Y', action: 'log:Refazer (demo)', icon: Redo2 },
      { sep: true },
      { label: 'Recortar', shortcut: 'Ctrl+X', action: 'log:Recortar', icon: Scissors },
      { label: 'Copiar', shortcut: 'Ctrl+C', action: 'log:Copiar', icon: Copy },
      { label: 'Colar', shortcut: 'Ctrl+V', action: 'log:Colar', icon: ClipboardPaste },
      { sep: true },
      { label: 'Buscar no Projeto', shortcut: 'Ctrl+Shift+F', action: 'log:Busca global (demo)' },
    ],
  },
  {
    id: 'visualizar', label: 'Visualizar',
    items: [
      { label: 'Explorer', panel: 'explorer' },
      { label: 'Arquivos', panel: 'files' },
      { label: 'Propriedades', panel: 'properties' },
      { label: 'Caixa de Ferramentas', panel: 'toolbox' },
      { label: 'Painel Inferior', panel: 'bottom' },
      { sep: true },
      { label: 'Tela Cheia', action: 'fullscreen', icon: Maximize },
    ],
  },
  {
    id: 'projeto', label: 'Projeto',
    items: [
      { label: 'Configurações do Projeto…', action: 'log:Configurações do projeto (demo)', icon: Cog },
      { label: 'Buildar…', shortcut: 'Ctrl+B', action: 'build', icon: Hammer },
      { label: 'Exportar ZIP', action: 'log:Exportação ZIP (demo)' },
    ],
  },
  {
    id: 'cena', label: 'Cena',
    items: [
      { label: 'Nova Cena', action: 'log:Nova cena criada', icon: FilePlus2 },
      { label: 'Salvar Cena', shortcut: 'Ctrl+S', action: 'salvar', icon: Save },
      { label: 'Renomear Cena…', action: 'log:Renomear cena (demo)' },
      { sep: true },
      { label: 'Adicionar Cubo', action: 'add:Cubo' },
      { label: 'Adicionar Câmera', action: 'add:Câmera' },
      { label: 'Adicionar Luz', action: 'add:Luz' },
    ],
  },
  {
    id: 'jogo', label: 'Jogo',
    items: [
      { label: 'Jogar', shortcut: 'F5', action: 'play', icon: Play },
      { label: 'Parar', shortcut: 'F8', action: 'stop', icon: Square },
      { sep: true },
      { label: 'Modo Depuração', action: 'log:Modo depuração (demo)', icon: Bug },
    ],
  },
  {
    id: 'script', label: 'Script',
    items: [
      { label: 'Editor de Código', action: 'editor', icon: Code },
      { label: 'Agente IA', shortcut: 'Ctrl+I', action: 'agent', icon: Sparkles },
      { sep: true },
      { label: 'Novo Script C#', action: 'novoscript', icon: FilePlus2 },
      { label: 'Compilar Scripts', action: 'log:Scripts compilados sem erros ✓' },
    ],
  },
  {
    id: 'biblioteca', label: 'Biblioteca de Recursos',
    items: [
      { label: 'Importar Asset…', action: 'log:Importador de assets (demo)', icon: Package },
      { label: 'Gerenciar Materiais', action: 'log:Gerenciador de materiais (demo)' },
      { label: 'Audios', action: 'log:Biblioteca de áudio (demo)' },
      { label: 'Animações', action: 'log:Editor de animação (demo)', icon: Film },
    ],
  },
  { id: 'config', label: 'Configurações', items: [{ label: 'Configurações Gerais…', action: 'settings', icon: Cog }, { label: 'Agente IA…', action: 'settings', icon: Sparkles }] },
  { id: 'ajuda', label: 'Ajuda', items: [{ label: 'Sobre o Astron Engine', action: 'log:Astron Engine v1.0.0 — Crie. Desenvolva. Explore.', icon: HelpCircle }, { label: 'Documentação', action: 'log:Documentação (demo)', icon: BookOpen }] },
];

export default function MenuBar({
  onNovoScript, onAgent, onBuild, onHub, onSettings, onSair, panels, togglePanel,
}) {
  const [open, setOpen] = useState(null);
  const wrap = useRef(null);
  const { pushLog, pushToast, setNewProjectOpen, navigate, setPlaying, addSceneObject, openTab, clearLogs } = useStore();

  useEffect(() => {
    const close = (e) => {
      if (wrap.current && !wrap.current.contains(e.target)) setOpen(null);
    };
    document.addEventListener('mousedown', close);
    return () => document.removeEventListener('mousedown', close);
  }, []);

  const run = (item) => {
    setOpen(null);
    const a = item.action || '';
    if (item.panel) return togglePanel(item.panel);
    if (a === 'novo') return setNewProjectOpen(true);
    if (a === 'abrir') return navigate('home', 'projetos');
    if (a === 'salvar') { pushLog('INFO', 'Cena salva em res://default.scene'); return pushToast('Projeto salvo ✓'); }
    if (a === 'salvarcomo') return pushToast('Salvo como "Cena Principal.copy.scene" (demo)');
    if (a === 'build') return onBuild();
    if (a === 'hub') return onHub();
    if (a === 'sair') return onSair();
    if (a === 'settings') return onSettings();
    if (a === 'agent') return onAgent();
    if (a === 'novoscript') return onNovoScript();
    if (a === 'editor') return openTab('res://main.cs');
    if (a === 'play') { setPlaying(true); pushLog('INFO', '▶ Jogo iniciado — modo de execução.'); return; }
    if (a === 'stop') { setPlaying(false); pushLog('INFO', '■ Jogo parado.'); return; }
    if (a === 'fullscreen') {
      if (document.fullscreenElement) document.exitFullscreen();
      else document.documentElement.requestFullscreen?.();
      return;
    }
    if (a.startsWith('add:')) return addSceneObject(a.slice(4));
    if (a.startsWith('log:')) {
      if (a.includes('limpar')) clearLogs();
      return pushToast(a.slice(4));
    }
  };

  return (
    <div className="ed-menubar" ref={wrap}>
      <div className="ed-nlogo">N</div>
      <button className="ed-engname" onClick={() => setOpen(open === 'engine' ? null : 'engine')}>
        Nova Engine <ChevronDown size={13} />
      </button>
      {open === 'engine' && (
        <div className="ed-menu" style={{ left: 74 }}>
          <button onClick={() => { setOpen(null); onHub(); }}>Hub de Projetos</button>
          <button onClick={() => { setOpen(null); onSettings(); }}>Configurações</button>
          <button onClick={() => { setOpen(null); pushToast('Astron Engine v1.0.0'); }}>Sobre</button>
        </div>
      )}

      <div className="ed-menus">
        {MENUS.map((m) => (
          <div key={m.id} className="ed-menu-wrap">
            <button
              className={`ed-menu-btn ${open === m.id ? 'open' : ''}`}
              onClick={() => setOpen(open === m.id ? null : m.id)}
              onMouseEnter={() => open && setOpen(m.id)}
            >
              {m.label}
            </button>
            {open === m.id && (
              <div className="ed-menu">
                {m.items.map((it, i) =>
                  it.sep ? (
                    <div className="ed-menu-sep" key={i} />
                  ) : (
                    <button key={i} onClick={() => run(it)}>
                      <span className="ed-menu-left">
                        {it.icon ? <it.icon size={14} /> : <span className="ed-menu-ico-slot" />}
                        {it.label}
                        {it.panel && panels[it.panel] && <Check size={13} className="ed-menu-check" />}
                      </span>
                      {it.shortcut && <span className="ed-menu-key">{it.shortcut}</span>}
                    </button>
                  )
                )}
              </div>
            )}
          </div>
        ))}
      </div>

      <div className="ed-wincontrols">
        <button>—</button>
        <button>▢</button>
        <button onClick={onHub}>✕</button>
      </div>
    </div>
  );
}
