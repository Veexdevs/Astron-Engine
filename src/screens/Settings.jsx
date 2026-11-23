import React, { useState } from 'react';
import { useStore } from '../store.jsx';
import { PROVIDERS, providerInfo, chatComplete } from '../ai/providers.js';
import { ArrowLeft, Check, FlaskConical, ShieldCheck, User, Palette, Info, Sparkles } from 'lucide-react';

// ============ Formulário de IA compartilhado (página + modal + chat) ============
export function AIConfigForm({ compact = false }) {
  const { ai, setAi, pushToast } = useStore();
  const [testing, setTesting] = useState(false);
  const info = providerInfo(ai.provider);

  const changeProvider = (key) => {
    const p = providerInfo(key);
    setAi({ provider: key, baseUrl: p.base, model: p.models[0] || ai.model });
  };

  const test = async () => {
    if (!ai.apiKey) return pushToast('Cole sua chave de API primeiro.');
    setTesting(true);
    try {
      await chatComplete({
        config: ai,
        messages: [{ role: 'user', content: 'Responda apenas: OK' }],
        tools: [],
      });
      pushToast('✔ Conexão OK — a IA respondeu!');
    } catch (e) {
      pushToast(`Falha: ${e.message.slice(0, 140)}`);
    } finally {
      setTesting(false);
    }
  };

  return (
    <div className={compact ? 'ai-form compact' : 'ai-form'}>
      <div className="ai-form-head">
        <span className="ai-form-icon"><Sparkles size={16} /></span>
        <div>
          <h3>Agente IA</h3>
          <p>Conecte qualquer API — do OpenRouter a endpoints locais — e o Agente faz o jogo inteiro em C# escrevendo o código sozinho.</p>
        </div>
      </div>

      <div className="field-row">
        <div className="field">
          <label>Provedor</label>
          <select value={ai.provider} onChange={(e) => changeProvider(e.target.value)}>
            {Object.entries(PROVIDERS).map(([k, p]) => (
              <option key={k} value={k}>{p.label}</option>
            ))}
          </select>
        </div>
        <div className="field">
          <label>Modelo</label>
          <input
            list="astron-models"
            value={ai.model}
            onChange={(e) => setAi({ model: e.target.value })}
            placeholder="modelo-id"
          />
          <datalist id="astron-models">
            {info.models.map((m) => <option key={m} value={m} />)}
          </datalist>
        </div>
      </div>

      <div className="field">
        <label>Chave de API</label>
        <input
          type="password"
          value={ai.apiKey}
          onChange={(e) => setAi({ apiKey: e.target.value })}
          placeholder={ai.provider === 'openrouter' ? 'sk-or-v1-...' : 'sk-...'}
        />
        <span className="hint">{info.hint} — a chave fica só no seu navegador (localStorage).</span>
      </div>

      <div className="field">
        <label>URL base</label>
        <input value={ai.baseUrl} onChange={(e) => setAi({ baseUrl: e.target.value })} />
        <span className="hint">
          {info.kind === 'anthropic'
            ? 'API nativa da Anthropic (usa /v1/messages).'
            : 'Compatível com OpenAI (usa /chat/completions). Edite para qualquer servidor.'}
        </span>
      </div>

      <div className="field-row">
        <div className="field">
          <label>Temperatura: {Number(ai.temperature).toFixed(1)}</label>
          <input
            type="range"
            min="0"
            max="1"
            step="0.1"
            value={ai.temperature}
            onChange={(e) => setAi({ temperature: Number(e.target.value) })}
          />
        </div>
        <div className="field">
          <label>Máx. de passos do agente</label>
          <input
            type="number"
            min="4"
            max="60"
            value={ai.maxSteps}
            onChange={(e) => setAi({ maxSteps: Number(e.target.value) || 24 })}
          />
        </div>
      </div>

      <div className="field">
        <label>Transporte</label>
        <select value={ai.transport} onChange={(e) => setAi({ transport: e.target.value })}>
          <option value="proxy">Proxy do editor (recomendado — sem CORS)</option>
          <option value="direct">Direto do navegador (pode falhar por CORS)</option>
        </select>
      </div>

      <div className="ai-form-actions">
        <button className="btn-outline" onClick={test} disabled={testing}>
          <FlaskConical size={15} style={{ marginRight: 8, verticalAlign: -2 }} />
          {testing ? 'Testando…' : 'Testar conexão'}
        </button>
        <span className="ai-secure"><ShieldCheck size={13} /> Nada é enviado para servidores da Astron.</span>
      </div>
    </div>
  );
}

// ============ Página de configurações (Home / Build) ============
export function SettingsPage({ onBack, variant = 'home' }) {
  const { account, pushToast, navigate } = useStore();
  const [tab, setTab] = useState('ia');

  const tabs = [
    { id: 'ia', label: 'Agente IA', icon: Sparkles },
    { id: 'conta', label: 'Conta', icon: User },
    { id: 'aparencia', label: 'Aparência', icon: Palette },
    { id: 'sobre', label: 'Sobre', icon: Info },
  ];

  return (
    <div className="settings-page">
      <div className="settings-page-head">
        {onBack ? (
          <button className="btn-outline back" onClick={onBack}>
            <ArrowLeft size={16} /> Voltar
          </button>
        ) : (
          <span />
        )}
        <h1>Configurações</h1>
        <span />
      </div>

      <div className="settings-body">
        <nav className="settings-tabs">
          {tabs.map((t) => (
            <button
              key={t.id}
              className={tab === t.id ? 'active' : ''}
              onClick={() => setTab(t.id)}
            >
              <t.icon size={16} /> {t.label}
            </button>
          ))}
        </nav>

        <div className="settings-content">
          {tab === 'ia' && <AIConfigForm />}

          {tab === 'conta' && (
            <div className="settings-card">
              <h3><User size={16} /> Conta</h3>
              <div className="field">
                <label>Usuário</label>
                <input defaultValue={account.user || 'jogador'} readOnly />
              </div>
              <div className="field">
                <label>E-mail</label>
                <input defaultValue="dev@astron.engine" readOnly />
              </div>
              <button className="btn-outline" onClick={() => pushToast('Perfil atualizado (demo).')}>
                <Check size={15} style={{ marginRight: 8, verticalAlign: -2 }} /> Salvar
              </button>
            </div>
          )}

          {tab === 'aparencia' && (
            <div className="settings-card">
              <h3><Palette size={16} /> Aparência</h3>
              <div className="field">
                <label>Tema</label>
                <select defaultValue="dark" onChange={() => pushToast('O Astron Engine é dark-only por enquanto.')}>
                  <option value="dark">Escuro (padrão)</option>
                </select>
              </div>
              <div className="field">
                <label>Escala da interface</label>
                <select defaultValue="100" onChange={() => pushToast('Escala aplicada (demo).')}>
                  <option value="100">100%</option>
                  <option value="125">125%</option>
                  <option value="150">150%</option>
                </select>
              </div>
            </div>
          )}

          {tab === 'sobre' && (
            <div className="settings-card about">
              <div className="about-logo">🪐</div>
              <h3>Astron Engine v1.0.0</h3>
              <p className="muted">Crie. Desenvolva. Explore.</p>
              <ul className="about-stack">
                <li>Editor web (React + Three.js)</li>
                <li>Jogos em C# / .NET 8</li>
                <li>Builds: APK · IPA · EXE · Linux via GitHub Actions</li>
                <li>Agente IA com OpenRouter e APIs compatíveis</li>
              </ul>
              <button className="btn-outline" onClick={() => navigate('home', 'sobre')}>
                Ver página Sobre
              </button>
            </div>
          )}
        </div>
      </div>
    </div>
  );
}

// ============ Modal de configurações (usado no login/editor) ============
export function SettingsModal() {
  const { setSettingsOpen } = useStore();
  return (
    <div className="modal-overlay" onClick={() => setSettingsOpen(false)}>
      <div className="modal" onClick={(e) => e.stopPropagation()}>
        <div className="modal-title-row">
          <h2>Configurações</h2>
          <button className="modal-x" onClick={() => setSettingsOpen(false)}>✕</button>
        </div>
        <AIConfigForm compact />
      </div>
    </div>
  );
}

export default SettingsPage;
