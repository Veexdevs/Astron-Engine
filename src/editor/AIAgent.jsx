import React, { useEffect, useRef, useState } from 'react';
import { useStore } from '../store.jsx';
import { runAgentTurn } from '../ai/agent.js';
import {
  Sparkles, Send, Settings, Trash2, FileCode, CheckCircle2, CircleDashed,
  AlertTriangle, Loader2, Terminal,
} from 'lucide-react';

const SUGGESTIONS = [
  'Faça um jogo de plataforma completo em C#',
  'Crie um player com pulo duplo e coletáveis',
  'Adicione inimigos, colisão e pontuação',
  'Faça um jogo onde você desvia de obstáculos',
];

export default function AIAgent() {
  const { ai, files, setFile, openTab, pushLog, setSettingsOpen, pushToast } = useStore();
  const [msgs, setMsgs] = useState([]);
  const [history, setHistory] = useState([]);
  const [input, setInput] = useState('');
  const [running, setRunning] = useState(false);
  const scrollRef = useRef(null);

  useEffect(() => {
    if (scrollRef.current) scrollRef.current.scrollTop = scrollRef.current.scrollHeight;
  }, [msgs, running]);

  const updateLast = (fn) =>
    setMsgs((m) => {
      if (!m.length) return m;
      const c = [...m];
      c[c.length - 1] = fn(c[c.length - 1]);
      return c;
    });

  const run = async (text) => {
    const question = (text || input).trim();
    if (!question || running) return;
    setInput('');

    if (!ai.apiKey) {
      setMsgs((m) => [
        ...m,
        { role: 'user', content: question },
        {
          role: 'assistant',
          error:
            'Nenhuma chave de API configurada. Clique em ⚙ Configurações e cole sua chave (ex.: OpenRouter — openrouter.ai/api/keys). O Agente funciona com OpenRouter, OpenAI, Anthropic, Groq, Mistral, DeepSeek, Gemini ou qualquer API compatível.',
          pending: false,
        },
      ]);
      return;
    }

    const hist = [...history, { role: 'user', content: question }];
    setMsgs((m) => [
      ...m,
      { role: 'user', content: question },
      { role: 'assistant', text: '', tools: [], status: 'Conectando…', pending: true },
    ]);
    setRunning(true);

    try {
      const res = await runAgentTurn({
        config: ai,
        files: { ...files },
        messages: hist,
        writeFile: (path, content) => {
          setFile(path, content);
          if (path.endsWith('.cs') || path.endsWith('.scene') || path.endsWith('.csproj')) {
            openTab(path);
          }
          pushLog('INFO', `Agente IA gravou ${path}`);
        },
        onEvent: (evt) => {
          if (evt.type === 'status') updateLast((m) => ({ ...m, status: evt.text }));
          else if (evt.type === 'text')
            updateLast((m) => ({ ...m, text: m.text ? `${m.text}\n\n${evt.text}` : evt.text, status: '' }));
          else if (evt.type === 'tool_start')
            updateLast((m) => ({ ...m, tools: [...(m.tools || []), { ...evt, done: false }], status: `⚙ ${evt.label}${evt.detail ? ' · ' + evt.detail : ''}` }));
          else if (evt.type === 'tool_done')
            updateLast((m) => ({
              ...m,
              tools: (m.tools || []).map((t, i) =>
                i === m.tools.length - 1 && !t.done ? { ...t, ...evt, done: true } : t
              ),
              status: '',
            }));
          else if (evt.type === 'done')
            updateLast((m) => ({ ...m, pending: false, status: '', text: evt.text && !m.text ? evt.text : m.text }));
        },
      });
      setHistory(res.messages.filter((m) => m.role !== 'system'));
      if (res.filesTouched?.length) {
        pushToast(`Agente IA gravou ${res.filesTouched.length} arquivo(s) ✓`);
      }
    } catch (e) {
      updateLast((m) => ({
        ...m,
        pending: false,
        status: '',
        error: e.message,
      }));
      pushLog('ERROR', `Agente IA: ${e.message}`);
    } finally {
      setRunning(false);
    }
  };

  const clear = () => {
    setMsgs([]);
    setHistory([]);
  };

  const configured = !!ai.apiKey;

  return (
    <div className="ia-root">
      {!configured && (
        <div className="ia-banner">
          <AlertTriangle size={15} />
          Configure a API para usar o Agente — ele faz o jogo inteiro escrevendo o código C# sozinho.
          <button onClick={() => setSettingsOpen(true)}>Configurar agora</button>
        </div>
      )}

      <div className="ia-msgs" ref={scrollRef}>
        {!msgs.length && (
          <div className="ia-empty">
            <div className="ia-avatar big"><Sparkles size={26} /></div>
            <h4>Agente IA do Astron</h4>
            <p>
              Peça um jogo inteiro — o Agente planeja, cria os arquivos <code>.cs</code> e coloca o código
              dentro do projeto <b>sem sua ajuda</b>.
            </p>
            <div className="ia-chips">
              {SUGGESTIONS.map((s) => (
                <button key={s} onClick={() => run(s)} disabled={running}>
                  {s}
                </button>
              ))}
            </div>
          </div>
        )}

        {msgs.map((m, i) =>
          m.role === 'user' ? (
            <div className="ia-user" key={i}>{m.content}</div>
          ) : (
            <div className={`ia-assistant ${m.error ? 'err' : ''}`} key={i}>
              <div className="ia-avatar"><Sparkles size={14} /></div>
              <div className="ia-bubble">
                {m.error && (
                  <div className="ia-error">
                    <AlertTriangle size={14} /> {m.error}
                  </div>
                )}
                {m.text && <div className="ia-text">{m.text}</div>}

                {!!m.tools?.length && (
                  <div className="ia-tools">
                    {m.tools.map((t, j) => (
                      <div className={`ia-tool ${t.done ? 'done' : ''}`} key={j}>
                        {t.done ? <CheckCircle2 size={13} /> : <CircleDashed size={13} className="spin" />}
                        <b>{t.label}</b>
                        {t.detail && <span>{t.detail}</span>}
                      </div>
                    ))}
                  </div>
                )}

                {m.status && (
                  <div className="ia-status">
                    <Loader2 size={13} className="spin" /> {m.status}
                  </div>
                )}
                {m.pending && !m.status && !m.tools?.length && (
                  <div className="ia-status">
                    <Loader2 size={13} className="spin" /> Pensando…
                  </div>
                )}
              </div>
            </div>
          )
        )}
      </div>

      <div className="ia-inputrow">
        <textarea
          rows={1}
          placeholder="Peça um jogo… ex.: “faça um platformer com 3 fases em C#”"
          value={input}
          onChange={(e) => setInput(e.target.value)}
          onKeyDown={(e) => {
            if (e.key === 'Enter' && !e.shiftKey) {
              e.preventDefault();
              run();
            }
          }}
        />
        <div className="ia-inputbtns">
          <button title="Limpar conversa" onClick={clear} disabled={running}><Trash2 size={16} /></button>
          <button title="Configurar IA" onClick={() => setSettingsOpen(true)}><Settings size={16} /></button>
          <button className="ia-send" title="Enviar" onClick={() => run()} disabled={running || !input.trim()}>
            {running ? <Loader2 size={16} className="spin" /> : <Send size={16} />}
          </button>
        </div>
      </div>
      <div className="ia-foot">
        <Terminal size={11} /> Modelo: <b>{ai.model}</b> · {ai.provider} · o agente edita os arquivos automaticamente
      </div>
    </div>
  );
}
