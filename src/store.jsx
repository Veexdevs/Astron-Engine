import React, { createContext, useCallback, useContext, useEffect, useMemo, useRef, useState } from 'react';
import { SEED_PROJECTS, SEED_LOGS, defaultFiles, FOLDER_ORDER, ROOT_FILE_ORDER } from './data/seed.js';

const StoreCtx = createContext(null);
export const useStore = () => useContext(StoreCtx);

const LS = {
  ai: 'astron.ai.v1',
  projects: 'astron.projects.v1',
  files: 'astron.files.v1',
  account: 'astron.account.v1',
};

function load(key, fallback) {
  try {
    const raw = localStorage.getItem(key);
    return raw ? JSON.parse(raw) : fallback;
  } catch {
    return fallback;
  }
}
function save(key, value) {
  try {
    localStorage.setItem(key, JSON.stringify(value));
  } catch {
    /* quota */
  }
}

export function StoreProvider({ children }) {
  // ---- navegação ----
  const [screen, setScreen] = useState('splash'); // splash | login | app
  const [area, setArea] = useState('home'); // home | build | editor
  const [page, setPage] = useState('inicio'); // inicio | projetos | biblioteca | loja | config | sobre | compilar | build-config | build-sobre

  // ---- projetos / arquivos ----
  const [projects, setProjects] = useState(() => load(LS.projects, SEED_PROJECTS));
  const [currentId, setCurrentId] = useState('meu-jogo');
  const [filesByProject, setFilesByProject] = useState(() => {
    const stored = load(LS.files, null);
    if (stored && stored['meu-jogo']) return stored;
    const seed = {};
    for (const p of SEED_PROJECTS) seed[p.id] = defaultFiles(p.name);
    return seed;
  });

  // ---- editor ----
  const [logs, setLogs] = useState(SEED_LOGS);
  const [selection, setSelection] = useState({
    id: 'cena-principal',
    label: 'Cena Principal',
    kind: 'scene',
    props: 'model',
  });
  const [sceneObjects, setSceneObjects] = useState([]);
  const [tabs, setTabs] = useState([{ id: 'scene', type: 'scene', label: 'Cena Principal' }]);
  const [activeTab, setActiveTab] = useState('scene');
  const [playing, setPlaying] = useState(false);
  const [mode, setMode] = useState('3d');
  const [tool, setTool] = useState('select');
  const [panels, setPanels] = useState({
    explorer: true,
    files: true,
    properties: true,
    toolbox: true,
    bottom: true,
  });
  const [bottomTab, setBottomTab] = useState('console'); // console | saida | depurador | audio | animacao | shader | ia

  // ---- UI global ----
  const [settingsOpen, setSettingsOpen] = useState(false);
  const [newProjectOpen, setNewProjectOpen] = useState(false);
  const [toasts, setToasts] = useState([]);
  const toastId = useRef(0);

  // ---- IA ----
  const [ai, setAiState] = useState(() =>
    load(LS.ai, {
      provider: 'openrouter',
      apiKey: '',
      model: 'openai/gpt-4o-mini',
      baseUrl: 'https://openrouter.ai/api/v1',
      temperature: 0.4,
      transport: 'proxy', // proxy | direct
      maxSteps: 24,
    })
  );
  const setAi = useCallback((patch) => {
    setAiState((prev) => {
      const next = typeof patch === 'function' ? patch(prev) : { ...prev, ...patch };
      save(LS.ai, next);
      return next;
    });
  }, []);

  const [account, setAccount] = useState(() => load(LS.account, { user: '', remember: false }));
  const setAccountSave = useCallback((acc) => {
    setAccount(acc);
    save(LS.account, acc);
  }, []);

  // ---- persistências ----
  useEffect(() => save(LS.projects, projects), [projects]);
  useEffect(() => save(LS.files, filesByProject), [filesByProject]);

  const currentProject = useMemo(
    () => projects.find((p) => p.id === currentId) || projects[0] || null,
    [projects, currentId]
  );
  const files = useMemo(
    () => filesByProject[currentId] || {},
    [filesByProject, currentId]
  );

  const navigate = useCallback((nextArea, nextPage) => {
    setArea(nextArea);
    if (nextPage) setPage(nextPage);
    setScreen('app');
  }, []);

  const pushToast = useCallback((msg) => {
    const id = ++toastId.current;
    setToasts((t) => [...t, { id, msg }]);
    setTimeout(() => setToasts((t) => t.filter((x) => x.id !== id)), 3400);
  }, []);

  const pushLog = useCallback((level, msg) => {
    setLogs((l) => [...l.slice(-199), { level, msg, t: Date.now() }]);
  }, []);

  const clearLogs = useCallback(() => setLogs([]), []);

  const setFile = useCallback(
    (path, content) => {
      setFilesByProject((all) => {
        const cur = all[currentId] || {};
        return { ...all, [currentId]: { ...cur, [path]: content } };
      });
    },
    [currentId]
  );

  const removeFile = useCallback(
    (path) => {
      setFilesByProject((all) => {
        const cur = { ...(all[currentId] || {}) };
        delete cur[path];
        return { ...all, [currentId]: cur };
      });
    },
    [currentId]
  );

  const openProject = useCallback(
    (id) => {
      setCurrentId(id);
      setTabs([{ id: 'scene', type: 'scene', label: 'Cena Principal' }]);
      setActiveTab('scene');
      navigate('editor');
    },
    [navigate]
  );

  const createProject = useCallback(
    ({ name, template }) => {
      const id = name.toLowerCase().replace(/[^a-z0-9]+/g, '-').replace(/^-|-$/g, '') || `projeto-${Date.now()}`;
      const created = new Date().toLocaleDateString('pt-BR');
      const thumbMap = { blank: '/thumb-cube.jpg', platformer: '/thumb-city.jpg', obby: '/thumb-obby.jpg', topdown: '/thumb-cube.jpg' };
      setProjects((p) => [{ id, name, created, thumb: thumbMap[template] || '/thumb-cube.jpg' }, ...p]);
      setFilesByProject((all) => ({ ...all, [id]: defaultFiles(name) }));
      setCurrentId(id);
      setLogs([{ level: 'INFO', msg: `Novo projeto "${name}" criado (${template}).` }]);
      setTabs([{ id: 'scene', type: 'scene', label: 'Cena Principal' }]);
      setActiveTab('scene');
      setNewProjectOpen(false);
      navigate('editor');
      return id;
    },
    [navigate]
  );

  const deleteProject = useCallback(
    (id) => {
      setProjects((p) => p.filter((x) => x.id !== id));
      if (id === currentId) {
        const rest = projects.filter((x) => x.id !== id);
        if (rest[0]) setCurrentId(rest[0].id);
      }
      pushToast('Projeto excluído.');
    },
    [projects, currentId, pushToast]
  );

  const renameProject = useCallback(
    (id, name) => {
      setProjects((p) => p.map((x) => (x.id === id ? { ...x, name } : x)));
      pushToast('Projeto renomeado.');
    },
    [pushToast]
  );

  // ---- abas de código ----
  const openTab = useCallback((path) => {
    const label = path.split('/').pop();
    setTabs((t) => (t.some((x) => x.id === path) ? t : [...t, { id: path, type: 'file', label }]));
    setActiveTab(path);
  }, []);

  const closeTab = useCallback(
    (id) => {
      setTabs((t) => {
        const next = t.filter((x) => x.id !== id);
        if (!next.length) return [{ id: 'scene', type: 'scene', label: 'Cena Principal' }];
        setActiveTab((cur) => (cur === id ? next[next.length - 1].id : cur));
        return next;
      });
    },
    []
  );

  const togglePanel = useCallback((key) => {
    setPanels((p) => ({ ...p, [key]: !p[key] }));
  }, []);

  const addSceneObject = useCallback(
    (label) => {
      const id = `obj-${Date.now()}`;
      setSceneObjects((o) => [...o, { id, label }]);
      setSelection({ id, label, kind: 'object', props: 'model' });
      pushLog('INFO', `Objeto "${label}" adicionado à cena.`);
      return id;
    },
    [pushLog]
  );

  const buildTree = useCallback(
    (nodes) =>
      nodes.map((n) => {
        let node = { ...n };
        if (n.dynamic === 'objects') {
          node.children = sceneObjects.map((o) => ({ id: o.id, label: o.label, icon: 'objects', color: '#58a6ff' }));
        }
        if (node.children) node.children = buildTree(node.children);
        return node;
      }),
    [sceneObjects]
  );

  const value = {
    screen, setScreen,
    area, page, navigate,
    projects, currentId, currentProject,
    openProject, createProject, deleteProject, renameProject,
    files, setFile, removeFile,
    logs, pushLog, clearLogs,
    selection, setSelection,
    sceneObjects, addSceneObject, buildTree,
    tabs, activeTab, openTab, closeTab, setActiveTab,
    playing, setPlaying,
    mode, setMode,
    tool, setTool,
    panels, togglePanel,
    bottomTab, setBottomTab,
    settingsOpen, setSettingsOpen,
    newProjectOpen, setNewProjectOpen,
    toasts, pushToast,
    ai, setAi,
    account, setAccount: setAccountSave,
    // helpers de árvore de arquivos
    fileTree: useMemo(() => buildFileTree(files), [files]),
    folderOrder: FOLDER_ORDER,
    rootFileOrder: ROOT_FILE_ORDER,
  };

  return <StoreCtx.Provider value={value}>{children}</StoreCtx.Provider>;
}

// Constrói hierarquia res:// a partir dos caminhos planos
export function buildFileTree(files) {
  const root = { dirs: new Map(), files: [] };
  for (const path of Object.keys(files)) {
    const rel = path.replace(/^res:\/\//, '');
    const parts = rel.split('/');
    let node = root;
    for (let i = 0; i < parts.length - 1; i++) {
      if (!node.dirs.has(parts[i])) node.dirs.set(parts[i], { dirs: new Map(), files: [] });
      node = node.dirs.get(parts[i]);
    }
    node.files.push({ name: parts[parts.length - 1], path });
  }
  return root;
}
