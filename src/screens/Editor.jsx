import React, { useEffect } from 'react';
import { useStore } from '../store.jsx';
import MenuBar from '../editor/MenuBar.jsx';
import Toolbar from '../editor/Toolbar.jsx';
import ExplorerPanel from '../editor/ExplorerPanel.jsx';
import FilesPanel from '../editor/FilesPanel.jsx';
import Viewport from '../editor/Viewport.jsx';
import PropertiesPanel from '../editor/PropertiesPanel.jsx';
import ToolboxPanel from '../editor/ToolboxPanel.jsx';
import BottomPanel from '../editor/BottomPanel.jsx';
import CodeEditor from '../editor/CodeEditor.jsx';
import StatusBar from '../editor/StatusBar.jsx';

// ============ Tela 5 — Editor (Nova Engine) ============
export default function Editor() {
  const {
    tabs, activeTab, setActiveTab, closeTab, panels, bottomTab, setBottomTab,
    togglePanel, navigate, pushLog, openTab, setFile, files, pushToast,
    setPlaying, setSettingsOpen, setScreen,
  } = useStore();

  // atalhos do editor
  useEffect(() => {
    const onKey = (e) => {
      if ((e.ctrlKey || e.metaKey) && e.key.toLowerCase() === 's') {
        e.preventDefault();
        pushLog('INFO', 'Cena salva em res://default.scene');
        pushToast('Projeto salvo ✓');
      }
      if ((e.ctrlKey || e.metaKey) && e.key.toLowerCase() === 'b') {
        e.preventDefault();
        navigate('build', 'compilar');
      }
      if ((e.ctrlKey || e.metaKey) && e.key.toLowerCase() === 'i') {
        e.preventDefault();
        setBottomTab('ia');
        if (!panels.bottom) togglePanel('bottom');
      }
    };
    window.addEventListener('keydown', onKey);
    return () => window.removeEventListener('keydown', onKey);
  }, [navigate, pushLog, pushToast, setBottomTab, panels.bottom, togglePanel]);

  const novoScript = () => {
    const path = 'res://scripts/NovoScript.cs';
    if (!files[path]) {
      setFile(
        path,
        `using Astron.Engine;\n\n// Script criado pelo editor\npublic class NovoScript : Component\n{\n    public override void Update(float dt)\n    {\n        // lógica aqui\n    }\n}\n`
      );
    }
    openTab(path);
    pushLog('INFO', `Novo script C# criado: ${path}`);
  };

  const active = tabs.find((t) => t.id === activeTab) || tabs[0];

  return (
    <div className="ed-root screen">
      <MenuBar
        onNovoScript={novoScript}
        onAgent={() => { setBottomTab('ia'); if (!panels.bottom) togglePanel('bottom'); }}
        onBuild={() => navigate('build', 'compilar')}
        onHub={() => navigate('home', 'inicio')}
        onSettings={() => setSettingsOpen(true)}
        onSair={() => setScreen('login')}
        panels={panels}
        togglePanel={togglePanel}
      />
      <Toolbar onSettings={() => setSettingsOpen(true)} />

      <div className="ed-body">
        {/* ---- coluna esquerda ---- */}
        {(panels.explorer || panels.files) && (
          <div className="ed-left">
            {panels.explorer && <ExplorerPanel />}
            {panels.files && <FilesPanel />}
          </div>
        )}

        {/* ---- centro ---- */}
        <div className="ed-center">
          <div className="ed-tabs">
            {tabs.map((t) => (
              <div
                key={t.id}
                className={`ed-tab ${t.id === active?.id ? 'active' : ''}`}
                onMouseDown={() => setActiveTab(t.id)}
              >
                <span className="ed-tab-label">{t.label}</span>
                <button
                  className="ed-tab-x"
                  onMouseDown={(e) => { e.stopPropagation(); closeTab(t.id); }}
                >
                  ✕
                </button>
              </div>
            ))}
            <button className="ed-tab-plus" title="Nova cena" onClick={() => pushLog('INFO', 'Aba de cena adicional (demo)')}>+</button>
          </div>

          <div className="ed-viewport-wrap">
            {active?.type === 'scene' ? <Viewport /> : <CodeEditor path={active.id} />}
          </div>

          {panels.bottom && (
            <BottomPanel tab={bottomTab} setTab={setBottomTab} />
          )}
        </div>

        {/* ---- coluna direita ---- */}
        {(panels.properties || panels.toolbox) && (
          <div className="ed-right">
            {panels.properties && <PropertiesPanel />}
            {panels.toolbox && <ToolboxPanel />}
          </div>
        )}
      </div>

      <StatusBar />
    </div>
  );
}
