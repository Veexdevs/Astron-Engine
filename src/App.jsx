import React from 'react';
import { useStore } from './store.jsx';
import Splash from './screens/Splash.jsx';
import Login from './screens/Login.jsx';
import Home from './screens/Home.jsx';
import Build from './screens/Build.jsx';
import Editor from './screens/Editor.jsx';
import { SettingsModal } from './screens/Settings.jsx';
import NewProjectModal from './components/NewProjectModal.jsx';
import './styles/screens.css';
import './styles/editor.css';

export default function App() {
  const { screen, area, toasts, settingsOpen, newProjectOpen } = useStore();

  return (
    <>
      {screen === 'splash' && <Splash />}
      {screen === 'login' && <Login />}
      {screen === 'app' && area === 'home' && <Home />}
      {screen === 'app' && area === 'build' && <Build />}
      {screen === 'app' && area === 'editor' && <Editor />}

      {settingsOpen && <SettingsModal />}
      {newProjectOpen && <NewProjectModal />}

      <div className="toasts">
        {toasts.map((t) => (
          <div className="toast" key={t.id}>{t.msg}</div>
        ))}
      </div>
    </>
  );
}
