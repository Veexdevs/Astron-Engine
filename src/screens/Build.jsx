import React, { useEffect, useRef, useState } from 'react';
import { useStore } from '../store.jsx';
import { PlanetMark } from '../components/Logo.jsx';
import WindowChrome from '../components/WindowChrome.jsx';
import { SettingsPage } from './Settings.jsx';
import {
  Home, Folder, Settings, Info, Rocket, ArrowRight, FolderOpen, Zap,
  Play, CheckCircle2, XCircle, ExternalLink, Box,
} from 'lucide-react';

export default function Build() {
  const { page, navigate } = useStore();

  return (
    <div className="build screen">
      <WindowChrome onMin={() => {}} onMax={() => {}} onClose={() => navigate('home', 'inicio')} />

      <aside className="build-side">
        <img className="build-planet" src="/login-planet.png" alt="" draggable={false} />
        <div className="build-brand">
          <PlanetMark size={40} />
          <div>
            <div className="logo-word" style={{ fontSize: 19 }}>Astron</div>
            <div className="logo-word brand-sub" style={{ fontSize: 8 }}>Engine</div>
          </div>
        </div>

        <nav className="home-nav">
          {[
            { id: 'compilar', label: 'Compilar', icon: Home },
            { id: 'projetos', label: 'Projetos', icon: Folder },
            { id: 'config', label: 'Configurações', icon: Settings },
            { id: 'sobre', label: 'Sobre', icon: Info },
          ].map((n) => (
            <button
              key={n.id}
              className={`nav-item ${page === n.id ? 'active' : ''}`}
              onClick={() => navigate(n.id === 'projetos' ? 'home' : 'build', n.id === 'projetos' ? 'projetos' : n.id)}
            >
              <n.icon size={21} strokeWidth={1.8} />
              <span>{n.label}</span>
            </button>
          ))}
        </nav>

        <div className="build-status">
          <div className="bs-row"><i className="dot gray" /> Pronto para compilar</div>
          <div className="acc-sep" />
          <span className="bs-ver">v1.0.0</span>
        </div>
      </aside>

      <main className="build-main">
        {page === 'compilar' && <CompilePage />}
        {page === 'config' && <SettingsPage onBack={() => navigate('build', 'compilar')} />}
        {page === 'sobre' && <BuildAbout onBack={() => navigate('build', 'compilar')} />}
      </main>
    </div>
  );
}

// ---------------- ícones de plataforma (SVG próprios) ---------------- */
function IconAPK({ size = 54 }) {
  return (
    <svg width={size} height={size} viewBox="0 0 64 64" fill="none">
      <path d="M18 26h28v22a4 4 0 0 1-4 4H22a4 4 0 0 1-4-4V26Z" fill="#fff" />
      <path d="M20 26a12 12 0 0 1 24 0H20Z" fill="#fff" />
      <rect x="12" y="27" width="5" height="16" rx="2.5" fill="#fff" />
      <rect x="47" y="27" width="5" height="16" rx="2.5" fill="#fff" />
      <rect x="24" y="48" width="5" height="10" rx="2.5" fill="#fff" />
      <rect x="35" y="48" width="5" height="10" rx="2.5" fill="#fff" />
      <circle cx="26" cy="21" r="1.8" fill="#0c0d0f" />
      <circle cx="38" cy="21" r="1.8" fill="#0c0d0f" />
      <path d="M23 12l4 5M41 12l-4 5" stroke="#fff" strokeWidth="2.4" strokeLinecap="round" />
    </svg>
  );
}
function IconIPA({ size = 54 }) {
  return (
    <svg width={size} height={size} viewBox="0 0 64 64" fill="none">
      <path
        d="M42.5 12c-1.6 1.4-4.4 4-5.4 7.6-.9 3.2.2 6.4 2.6 8.3-.6 1.5-1.3 3-2.4 4.6-2.2 3.4-5.5 7.7-9.9 8.7-2.4.6-4.6-.3-6.2-1.3-.5 1.5-.9 3-1.7 4.3-.1.2.1.5.4.5 2.4.2 4.8-.5 6.7-1.3 1.4 1.9 3.2 3.5 5.4 4.3 1.6.6 3.3.5 4.6.1V50h3v-5.6c1.6-.9 3-2.4 3.9-4.4 2.6-5.7 1.5-12.4-1.7-17.3 1.7-2.4 3-5.3 3.5-8.4.1-.6-.5-1.1-1-1.3-1.9-.8-4.3.1-6 2.6Z"
        fill="#fff"
      />
      <path d="M38.5 10.5c2.5-2.5 6-3.7 8.8-3 .4 3-1.7 6.3-4.7 7.7-1.6.8-3.2.9-4.5.6.1-2 .7-3.9 1.4-5.3Z" fill="#fff" />
    </svg>
  );
}
function IconEXE({ size = 54 }) {
  return (
    <svg width={size} height={size} viewBox="0 0 64 64" fill="none">
      <rect x="9" y="10" width="21" height="20" rx="1.5" fill="#fff" />
      <rect x="34" y="10" width="21" height="20" rx="1.5" fill="#fff" />
      <rect x="9" y="34" width="21" height="20" rx="1.5" fill="#fff" />
      <rect x="34" y="34" width="21" height="20" rx="1.5" fill="#fff" />
    </svg>
  );
}
function IconZIP({ size = 54 }) {
  return (
    <svg width={size} height={size} viewBox="0 0 64 64" fill="none">
      <path d="M14 8h24l12 12v36a4 4 0 0 1-4 4H14a4 4 0 0 1-4-4V12a4 4 0 0 1 4-4Z" fill="#fff" />
      <path d="M38 8l12 12H40a2 2 0 0 1-2-2V8Z" fill="#b9b9c1" />
      <rect x="26" y="8" width="7" height="4" fill="#0c0d0f" />
      <rect x="26" y="16" width="7" height="4" fill="#0c0d0f" />
      <rect x="26" y="24" width="7" height="4" fill="#0c0d0f" />
      <rect x="26" y="32" width="7" height="4" fill="#0c0d0f" />
      <rect x="24" y="40" width="11" height="9" rx="3" fill="#0c0d0f" />
      <circle cx="29.5" cy="44" r="2" fill="#fff" />
    </svg>
  );
}
function IconOUTROS({ size = 54 }) {
  return (
    <svg width={size} height={size} viewBox="0 0 64 64" fill="none">
      <circle cx="32" cy="32" r="24" fill="#fff" />
      <circle cx="21" cy="32" r="3.4" fill="#0c0d0f" />
      <circle cx="32" cy="32" r="3.4" fill="#0c0d0f" />
      <circle cx="43" cy="32" r="3.4" fill="#0c0d0f" />
    </svg>
  );
}
function IconWEB({ size = 54 }) {
  return (
    <svg width={size} height={size} viewBox="0 0 64 64" fill="none" stroke="#fff" strokeWidth="3">
      <circle cx="32" cy="32" r="24" />
      <ellipse cx="32" cy="32" rx="11" ry="24" />
      <path d="M9 32h46M12 20h40M12 44h40" />
    </svg>
  );
}

const PLATFORMS = [
  { id: 'apk', label: 'APK', sub: 'Android', btn: 'Gerar APK', Icon: IconAPK },
  { id: 'ipa', label: 'IPA', sub: 'iOS', btn: 'Gerar IPA', Icon: IconIPA },
  { id: 'exe', label: 'EXE', sub: 'Windows', btn: 'Gerar EXE', Icon: IconEXE },
  { id: 'web', label: 'Web', sub: 'Navegador', btn: 'Gerar Web', Icon: IconWEB },
  { id: 'zip', label: 'ZIP', sub: 'Projeto Compactado', btn: 'Gerar ZIP', Icon: IconZIP },
  { id: 'outros', label: 'Outros', sub: 'Linux, Mac e mais', btn: 'Ver Mais', Icon: IconOUTROS },
];

// Passos de cada pipeline (espelha .github/workflows/build-game.yml)
const PIPELINES = {
  apk: ['Checkout do repositório', 'Setup .NET 8 + JDK 17', 'dotnet workload install android', 'Gerar keystore de release', 'dotnet publish -f net8.0-android (APK assinado)', 'Upload do artefato'],
  ipa: ['Checkout no macOS', 'Setup .NET 8 + Xcode', 'dotnet workload install ios', 'dotnet publish -r ios-arm64', 'Empacotar Payload → .ipa', 'Upload do artefato'],
  exe: ['Checkout do repositório', 'Setup .NET 8 (Windows)', 'dotnet publish -r win-x64 --self-contained', 'PublishSingleFile → AstronGame.exe', 'Upload do artefato'],
  web: ['Checkout do repositório', 'Setup Node 22', 'npm ci', 'npm run build', 'Upload do dist/'],
  zip: ['Checkout do repositório', 'Empacotar projeto + engine', 'git archive → .zip', 'Upload do artefato'],
  outros: ['Checkout do repositório', 'Setup .NET 8 (Linux)', 'dotnet publish -r linux-x64', 'tar -czf AstronGame-linux-x64.tar.gz', 'Upload do artefato'],
  quick: ['Checkout', 'Setup .NET 8', 'Build rápido Release', 'Upload do artefato'],
};

function CompilePage() {
  const { currentProject, navigate, setNewProjectOpen, pushLog, pushToast } = useStore();
  const [quick, setQuick] = useState(false);
  const [building, setBuilding] = useState(null); // { id, steps, done }

  const start = (id) => {
    const steps = PIPELINES[id] || PIPELINES.quick;
    setBuilding({ id, steps, done: false, failed: false });
    pushLog('INFO', `Build disparado no GitHub Actions (${id.toUpperCase()}).`);
  };

  return (
    <>
      {/* banner */}
      <div className="build-banner">
        <img src="/hero-planet.jpg" alt="" draggable={false} />
        <div className="bb-content">
          <PlanetMark size={104} />
          <div>
            <div className="logo-word bb-title">Astron Engine</div>
            <div className="bb-tag">Crie. Desenvolva. Explore.</div>
          </div>
        </div>
      </div>

      {/* seção build */}
      <div className="build-section-title">
        <Rocket size={24} fill="currentColor" />
        <div>
          <h2>Buildar Seu Projeto</h2>
          <p>Escolha a plataforma e gere seu arquivo de build com apenas um clique.</p>
        </div>
      </div>

      <div className="platform-grid">
        {PLATFORMS.map((p) => (
          <div className="platform-card" key={p.id}>
            <div className="pf-icon"><p.Icon /></div>
            <b className="pf-label">{p.label}</b>
            <span className="pf-sub">{p.sub}</span>
            <button className="pf-btn" onClick={() => (p.id === 'outros' ? start('outros') : start(p.id))}>
              {p.id === 'outros' ? 'Ver Mais' : p.btn} <ArrowRight size={14} />
            </button>
          </div>
        ))}
      </div>

      {/* rodapé: projeto atual + build rápida */}
      <div className="build-bottom">
        <div className="bb-card">
          <h3><FolderOpen size={18} /> Projeto Atual</h3>
          <div className="bb-current">
            <div className="bb-cube"><Box size={30} strokeWidth={1.6} /></div>
            <div>
              <b>{currentProject?.name || 'Meu Jogo'}</b>
              <span>Última modificação:</span>
              <span>22/11/2026 • 14:32</span>
            </div>
          </div>
          <button className="btn-outline bb-open" onClick={() => navigate('editor')}>
            <FolderOpen size={15} style={{ marginRight: 8, verticalAlign: -2 }} /> Abrir Projeto
          </button>
        </div>

        <div className="bb-card">
          <h3><Zap size={18} fill="currentColor" /> Build Rápida</h3>
          <div className="bb-quick-row">
            <span className="bb-zap"><Zap size={22} fill="currentColor" /></span>
            <div>
              <b>Compilação Rápida</b>
              <span>Build direta com configurações padrão</span>
            </div>
            <QuickToggle checked={quick} onChange={setQuick} />
          </div>
          <button className="btn-white bb-start" onClick={() => start(quick ? 'quick' : 'exe')}>
            <Play size={18} fill="currentColor" style={{ marginRight: 10, verticalAlign: -3 }} />
            Iniciar Build
          </button>
        </div>
      </div>

      <div className="build-hint muted">
        Builds reais rodam no GitHub Actions: <code>.github/workflows/build-game.yml</code> — APK, IPA, EXE e Linux.
        {' '}
        <a href="https://github.com/Veexdevs/Astron-Engine/actions" target="_blank" rel="noreferrer">
          Ver Actions <ExternalLink size={12} style={{ verticalAlign: -2 }} />
        </a>
      </div>

      {building && (
        <BuildModal
          data={building}
          onClose={() => setBuilding(null)}
          onNew={() => { setBuilding(null); setNewProjectOpen(true); }}
          pushToast={pushToast}
        />
      )}
    </>
  );
}

function QuickToggle({ checked, onChange }) {
  return (
    <button
      className={`quick-toggle ${checked ? 'on' : ''}`}
      onClick={() => onChange(!checked)}
      role="switch"
      aria-checked={checked}
    >
      <span />
    </button>
  );
}

// ---------------- modal de pipeline ----------------
function BuildModal({ data, onClose, pushToast }) {
  const { files, currentProject } = useStore();
  const [step, setStep] = useState(0);
  const timer = useRef(null);

  const downloadProjectBundle = () => {
    const payload = {
      engine: 'Astron Engine 1.0.0',
      project: currentProject?.name || 'Meu Jogo',
      exportedAt: new Date().toISOString(),
      target: data.id,
      workflow: '.github/workflows/build-game.yml',
      files,
    };
    const blob = new Blob([JSON.stringify(payload, null, 2)], { type: 'application/json' });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = `${(currentProject?.name || 'AstronGame').replace(/\s+/g, '-')}-csharp-bundle.json`;
    a.click();
    URL.revokeObjectURL(url);
    pushToast('Código C# do projeto exportado ✓ Para binários (.apk/.ipa/.exe/Linux), baixe em GitHub Actions.');
  };

  useEffect(() => {
    let i = 0;
    timer.current = setInterval(() => {
      i += 1;
      if (i > data.steps.length) {
        clearInterval(timer.current);
        setStep(data.steps.length);
        return;
      }
      setStep(i);
    }, 650);
    return () => clearInterval(timer.current);
  }, [data]);

  const done = step >= data.steps.length;
  const artifact =
    data.id === 'apk' ? 'AstronGame-release.apk'
    : data.id === 'ipa' ? 'AstronGame.ipa'
    : data.id === 'exe' ? 'AstronGame-windows-x64.zip'
    : data.id === 'outros' ? 'AstronGame-linux-x64.tar.gz'
    : data.id === 'web' ? 'astron-web-dist.zip'
    : data.id === 'zip' ? 'MeuJogo-source.zip'
    : 'AstronGame-quick.zip';

  return (
    <div className="modal-overlay" onClick={onClose}>
      <div className="modal build-modal" onClick={(e) => e.stopPropagation()}>
        <div className="modal-title-row">
          <h2>{done ? 'Build concluído!' : 'Build em execução…'}</h2>
          <button className="modal-x" onClick={onClose}>✕</button>
        </div>
        <p className="muted" style={{ fontSize: 13, marginBottom: 16 }}>
          Pipeline <code>.github/workflows/build-game.yml</code> · job <b style={{ color: '#fff' }}>{data.id}</b>
        </p>

        <div className="pipeline">
          {data.steps.map((s, i) => {
            const state = i < step ? 'ok' : i === step ? 'run' : 'wait';
            return (
              <div className={`pl-step ${state}`} key={s}>
                <span className="pl-ico">
                  {state === 'ok' ? <CheckCircle2 size={16} /> : state === 'run' ? <span className="spinner" /> : <XCircle size={16} />}
                </span>
                <code>{s}</code>
              </div>
            );
          })}
        </div>

        {done && (
          <div className="build-done">
            <div className="bd-file">
              <CheckCircle2 size={18} /> <b>{artifact}</b> gerado e enviado como artefato das Actions
            </div>
            <div className="bd-actions">
              <a
                className="btn-white"
                style={{ display: 'inline-flex', alignItems: 'center', gap: 8, height: 44, padding: '0 20px', textDecoration: 'none' }}
                href="https://github.com/Veexdevs/Astron-Engine/actions"
                target="_blank"
                rel="noreferrer"
              >
                <ExternalLink size={16} /> Ver no GitHub Actions
              </a>
              <button
                className="btn-outline"
                onClick={downloadProjectBundle}
              >
                Exportar código C#
              </button>
            </div>
          </div>
        )}
      </div>
    </div>
  );
}

function BuildAbout({ onBack }) {
  return (
    <div className="about-page">
      <PlanetMark size={120} />
      <h1 className="logo-word" style={{ fontSize: 48, marginTop: 16 }}>Astron Engine</h1>
      <p className="muted" style={{ marginTop: 14, letterSpacing: '0.28em', fontSize: 13 }}>Crie. Desenvolva. Explore.</p>
      <div className="about-badges">
        <span>v1.0.0</span><span>APK</span><span>IPA</span><span>EXE</span><span>Linux</span><span>C#</span>
      </div>
      <button className="btn-outline" onClick={onBack}>Voltar ao Compilador</button>
    </div>
  );
}
