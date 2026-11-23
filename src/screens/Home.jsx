import React, { useState } from 'react';
import { useStore } from '../store.jsx';
import { PlanetMark } from '../components/Logo.jsx';
import WindowChrome from '../components/WindowChrome.jsx';
import { SettingsPage } from './Settings.jsx';
import {
  Home as HomeIcon, Folder, Box, ShoppingBag, Settings, Info, Zap,
  Plus, Clock, ChevronRight, MoreHorizontal, User, LogOut, Play, Trash2, Pencil, Hammer,
} from 'lucide-react';

const NAV = [
  { id: 'inicio', label: 'Início', icon: HomeIcon },
  { id: 'projetos', label: 'Meus Projetos', icon: Folder },
  { id: 'biblioteca', label: 'Bibioteca', icon: Box },
  { id: 'loja', label: 'Loja', icon: ShoppingBag },
  { id: 'config', label: 'Configurações', icon: Settings },
  { id: 'sobre', label: 'Sobre', icon: Info },
];

export default function Home() {
  const store = useStore();
  const { page, navigate, setScreen } = store;

  return (
    <div className="home screen">
      <WindowChrome onMin={() => {}} onMax={() => {}} onClose={() => setScreen('login')} />

      {/* ---------- sidebar ---------- */}
      <aside className="home-side">
        <div className="home-brand">
          <PlanetMark size={48} />
          <div>
            <div className="logo-word" style={{ fontSize: 24 }}>Astron</div>
            <div className="logo-word brand-sub" style={{ fontSize: 10 }}>Engine</div>
          </div>
        </div>

        <nav className="home-nav">
          {NAV.map((n) => (
            <button
              key={n.id}
              className={`nav-item ${page === n.id ? 'active' : ''}`}
              onClick={() => navigate('home', n.id)}
            >
              <n.icon size={21} strokeWidth={1.8} />
              <span>{n.label}</span>
            </button>
          ))}
        </nav>

        <div className="home-account">
          <div className="acc-row">
            <span className="acc-zap"><Zap size={18} fill="currentColor" /></span>
            <div>
              <b>Astron Engine</b>
              <span>v1.0.0</span>
            </div>
          </div>
          <div className="acc-sep" />
          <div className="acc-online">
            <i className="dot green" /> Online
          </div>
        </div>
      </aside>

      {/* ---------- conteúdo ---------- */}
      <main className="home-main">
        <div className="home-topbar">
          <span />
          <div className="home-topbtns">
            <button className="circle-btn" title="Configurações" onClick={() => navigate('home', 'config')}>
              <Settings size={19} strokeWidth={1.7} />
            </button>
            <button className="circle-btn" title="Conta" onClick={() => navigate('home', 'config')}>
              <User size={19} strokeWidth={1.7} />
            </button>
          </div>
        </div>

        {page === 'inicio' && <InicioPage />}
        {page === 'projetos' && <ProjectsPage />}
        {page === 'biblioteca' && <LibraryPage />}
        {page === 'loja' && <StorePage />}
        {page === 'config' && <SettingsPage onBack={() => navigate('home', 'inicio')} />}
        {page === 'sobre' && <AboutPage />}
      </main>
    </div>
  );
}

// ---------------- Início ----------------
function InicioPage() {
  const { projects, navigate, setNewProjectOpen } = useStore();
  return (
    <>
      <section className="hero">
        <img className="hero-img" src="/hero-planet.jpg" alt="" draggable={false} />
        <div className="hero-fade" />
        <div className="hero-content">
          <h1>Crie. Desenvolva. Explore.</h1>
          <p>Sua imaginação, sem limites.</p>
          <button className="btn-white hero-btn" onClick={() => setNewProjectOpen(true)}>
            <Plus size={20} strokeWidth={2.6} /> Novo Projeto
          </button>
        </div>
      </section>

      <div className="section-head">
        <h2><Clock size={24} strokeWidth={2} /> Projetos Recentes</h2>
        <button className="btn-outline ver-todos" onClick={() => navigate('home', 'projetos')}>
          Ver Todos <ChevronRight size={16} />
        </button>
      </div>

      <div className="project-grid">
        {projects.slice(0, 3).map((p) => <ProjectCard key={p.id} project={p} />)}
        <button className="new-project-card" onClick={() => setNewProjectOpen(true)}>
          <Plus size={44} strokeWidth={1.6} />
          <b>Novo Projeto</b>
          <span>Comece do zero</span>
        </button>
      </div>
    </>
  );
}

// ---------------- Cartão de projeto ----------------
function ProjectCard({ project }) {
  const { openProject, navigate, deleteProject, renameProject, pushToast } = useStore();
  const [menu, setMenu] = useState(false);

  return (
    <div className="project-card" onClick={() => openProject(project.id)}>
      <div className="pc-thumb">
        <img src={project.thumb} alt="" draggable={false} />
        <button
          className="pc-dots"
          onClick={(e) => { e.stopPropagation(); setMenu((v) => !v); }}
        >
          <MoreHorizontal size={18} />
        </button>
        {menu && (
          <div className="pc-menu" onClick={(e) => e.stopPropagation()}>
            <button onClick={() => { setMenu(false); openProject(project.id); }}>
              <Play size={14} /> Abrir
            </button>
            <button onClick={() => { setMenu(false); navigate('build', 'compilar'); }}>
              <Hammer size={14} /> Buildar
            </button>
            <button
              onClick={() => {
                setMenu(false);
                const name = prompt('Novo nome do projeto:', project.name);
                if (name) renameProject(project.id, name);
              }}
            >
              <Pencil size={14} /> Renomear
            </button>
            <button
              className="danger"
              onClick={() => {
                setMenu(false);
                if (confirm(`Excluir "${project.name}"?`)) deleteProject(project.id);
              }}
            >
              <Trash2 size={14} /> Excluir
            </button>
          </div>
        )}
      </div>
      <div className="pc-body">
        <b>{project.name}</b>
        <span><Clock size={14} /> Criado em {project.created}</span>
      </div>
    </div>
  );
}

// ---------------- Meus Projetos ----------------
function ProjectsPage() {
  const { projects, setNewProjectOpen } = useStore();
  return (
    <>
      <div className="section-head" style={{ marginTop: 4 }}>
        <h2><Folder size={24} /> Meus Projetos</h2>
        <button className="btn-white" style={{ height: 42, padding: '0 18px' }} onClick={() => setNewProjectOpen(true)}>
          <Plus size={17} style={{ marginRight: 7, verticalAlign: -3 }} /> Novo
        </button>
      </div>
      <div className="project-grid">
        {projects.map((p) => <ProjectCard key={p.id} project={p} />)}
        <button className="new-project-card" onClick={() => setNewProjectOpen(true)}>
          <Plus size={44} strokeWidth={1.6} />
          <b>Novo Projeto</b>
          <span>Comece do zero</span>
        </button>
      </div>
    </>
  );
}

// ---------------- Biblioteca ----------------
const LIB = [
  { name: 'Kit de Plataformas', img: '/thumb-obby.jpg', assets: 42 },
  { name: 'Pacote Urbano', img: '/thumb-city.jpg', assets: 128 },
  { name: 'Formas Básicas', img: '/thumb-cube.jpg', assets: 16 },
];

function LibraryPage() {
  const { pushToast } = useStore();
  return (
    <>
      <div className="section-head" style={{ marginTop: 4 }}>
        <h2><Box size={24} /> Bibioteca</h2>
      </div>
      <div className="lib-grid">
        {LIB.map((l) => (
          <div className="lib-card" key={l.name}>
            <img src={l.img} alt="" />
            <div className="lib-body">
              <b>{l.name}</b>
              <span className="muted">{l.assets} assets · C# / Astron</span>
              <button className="btn-outline" onClick={() => pushToast(`"${l.name}" importado para o projeto.`)}>
                Importar
              </button>
            </div>
          </div>
        ))}
      </div>
    </>
  );
}

// ---------------- Loja ----------------
const SHOP = [
  { name: 'Kit Sci-Fi Premium', price: 'R$ 29,90', img: '/thumb-city.jpg' },
  { name: 'Pacote Obby Pro', price: 'R$ 19,90', img: '/thumb-obby.jpg' },
  { name: 'Materiais Studio', price: 'R$ 12,90', img: '/thumb-cube.jpg' },
];

function StorePage() {
  const { pushToast } = useStore();
  return (
    <>
      <div className="section-head" style={{ marginTop: 4 }}>
        <h2><ShoppingBag size={24} /> Loja</h2>
      </div>
      <div className="lib-grid">
        {SHOP.map((s) => (
          <div className="lib-card" key={s.name}>
            <img src={s.img} alt="" />
            <div className="lib-body">
              <b>{s.name}</b>
              <span className="price">{s.price}</span>
              <button className="btn-white" style={{ height: 40 }} onClick={() => pushToast('Pagamento demo — em breve!')}>
                Comprar
              </button>
            </div>
          </div>
        ))}
      </div>
    </>
  );
}

// ---------------- Sobre ----------------
function AboutPage() {
  const { pushToast } = useStore();
  return (
    <div className="about-page">
      <PlanetMark size={130} />
      <h1 className="logo-word" style={{ fontSize: 54, marginTop: 18 }}>Astron</h1>
      <div className="logo-word" style={{ fontSize: 17, letterSpacing: '0.5em', paddingLeft: '0.5em', marginTop: 8 }}>Engine</div>
      <p className="muted" style={{ marginTop: 16, letterSpacing: '0.3em', paddingLeft: '0.3em', fontSize: 13 }}>
        Crie. Desenvolva. Explore.
      </p>
      <div className="about-badges">
        <span>v1.0.0</span><span>React + Three.js</span><span>C# / .NET 8</span><span>GitHub Actions</span><span>Agente IA</span>
      </div>
      <p className="muted" style={{ maxWidth: 460, textAlign: 'center', fontSize: 13.5, lineHeight: 1.7 }}>
        Engine de criação de jogos com editor 3D, Agente IA que escreve o código C# por você
        e builds automáticos de APK, IPA, EXE e Linux.
      </p>
      <button className="btn-outline" onClick={() => pushToast('Obrigado por usar a Astron Engine! 🪐')}>
        <LogOut size={15} style={{ marginRight: 8, verticalAlign: -2 }} /> Entrar em contato
      </button>
    </div>
  );
}
