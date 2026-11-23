import React, { useState } from 'react';
import { useStore } from '../store.jsx';
import { LogoFull } from '../components/Logo.jsx';
import WindowChrome from '../components/WindowChrome.jsx';
import { User, Lock, Eye, EyeOff, Github, Settings } from 'lucide-react';

// ============ Tela 2 — Login ============
export default function Login() {
  const { setScreen, setSettingsOpen, account, setAccount, pushToast } = useStore();
  const [user, setUser] = useState(account.user || '');
  const [pass, setPass] = useState('');
  const [remember, setRemember] = useState(account.remember ?? false);
  const [showPass, setShowPass] = useState(false);
  const [error, setError] = useState('');

  const enter = (e) => {
    e?.preventDefault();
    if (!user.trim() || !pass.trim()) {
      setError('Preencha usuário e senha para continuar.');
      return;
    }
    setAccount({ user: user.trim(), remember });
    setScreen('app');
  };

  const github = () => {
    pushToast('Login com GitHub (demo) — conecte seu backend OAuth para produção.');
    setAccount({ user: 'github-user', remember: true });
    setScreen('app');
  };

  return (
    <div className="login screen">
      <div className="orb orb-login-tl" />
      <img className="login-planet" src="/login-planet.png" alt="" draggable={false} />
      <WindowChrome onMin={() => {}} onMax={() => {}} onClose={() => setScreen('splash')} />

      <div className="login-divider" />

      {/* ---- lado esquerdo: marca ---- */}
      <div className="login-brand">
        <LogoFull size={200} wordSize={96} stacked tagline="Crie. Desenvolva. Explore." />
      </div>

      {/* ---- lado direito: cartão ---- */}
      <div className="login-card">
        <h1>Bem-vindo(a)!</h1>
        <p className="login-sub">Faça login na sua conta para continuar.</p>

        <form onSubmit={enter}>
          <div className={`login-input ${error && !user.trim() ? 'err' : ''}`}>
            <User size={19} strokeWidth={1.7} />
            <input
              placeholder="Usuário ou e-mail"
              value={user}
              onChange={(e) => { setUser(e.target.value); setError(''); }}
              autoComplete="username"
            />
          </div>

          <div className={`login-input ${error && !pass.trim() ? 'err' : ''}`}>
            <Lock size={19} strokeWidth={1.7} />
            <input
              type={showPass ? 'text' : 'password'}
              placeholder="Senha"
              value={pass}
              onChange={(e) => { setPass(e.target.value); setError(''); }}
              autoComplete="current-password"
            />
            <button type="button" className="eye" onClick={() => setShowPass((v) => !v)} title="Mostrar senha">
              {showPass ? <EyeOff size={19} strokeWidth={1.7} /> : <Eye size={19} strokeWidth={1.7} />}
            </button>
          </div>

          {error && <div className="login-error">{error}</div>}

          <div className="login-row">
            <label className="login-check">
              <input type="checkbox" checked={remember} onChange={(e) => setRemember(e.target.checked)} />
              <span>Lembrar de mim</span>
            </label>
            <button type="button" className="login-link" onClick={() => pushToast('Enviamos um link de recuperação (demo).')}>
              Esqueceu sua senha?
            </button>
          </div>

          <button type="submit" className="btn-white login-submit">Entrar</button>
        </form>

        <div className="login-or"><span>ou</span></div>

        <button type="button" className="login-github" onClick={github}>
          <Github size={19} strokeWidth={1.7} />
          Entrar com GitHub
        </button>

        <div className="login-create">
          Não tem uma conta?{' '}
          <button type="button" className="login-link strong" onClick={() => pushToast('Cadastro disponível na versão com backend.')}>
            Criar conta
          </button>
        </div>
      </div>

      <button className="login-config" onClick={() => setSettingsOpen(true)}>
        <Settings size={17} strokeWidth={1.7} />
        Configurações
      </button>
      <div className="login-version">v1.0.0</div>
    </div>
  );
}
