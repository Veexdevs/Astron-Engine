// Smoke test: renderiza todas as telas em SSR para capturar erros de runtime.
import { createServer } from 'vite';

const server = await createServer({
  configFile: false,
  root: process.cwd(),
  logLevel: 'error',
  server: { middlewareMode: true, hmr: false },
  plugins: [(await import('@vitejs/plugin-react')).default()],
});

try {
  const React = (await import('react')).default;
  const { renderToString } = await import('react-dom/server');
  const { StoreProvider } = await server.ssrLoadModule('/src/store.jsx');

  const screens = [
    ['Splash', '/src/screens/Splash.jsx'],
    ['Login', '/src/screens/Login.jsx'],
    ['Home', '/src/screens/Home.jsx'],
    ['Build', '/src/screens/Build.jsx'],
    ['Editor', '/src/screens/Editor.jsx'],
    ['Settings', '/src/screens/Settings.jsx'],
    ['NewProjectModal', '/src/components/NewProjectModal.jsx'],
  ];

  let failures = 0;
  for (const [name, path] of screens) {
    try {
      const mod = await server.ssrLoadModule(path);
      const Comp = mod.default || mod.SettingsPage;
      const html = renderToString(React.createElement(StoreProvider, null, React.createElement(Comp)));
      const len = html.length;
      if (len < 50) throw new Error(`renderizou apenas ${len} bytes`);
      console.log(`✔ ${name} (${len} bytes)`);
    } catch (e) {
      failures++;
      console.log(`✘ ${name}: ${e.message}`);
      console.log((e.stack || '').split('\n').slice(1, 4).join('\n'));
    }
  }

  // App raiz (estado inicial = splash)
  try {
    const App = (await server.ssrLoadModule('/src/App.jsx')).default;
    const html = renderToString(React.createElement(StoreProvider, null, React.createElement(App)));
    console.log(`✔ App root (${html.length} bytes)`);
  } catch (e) {
    failures++;
    console.log(`✘ App root: ${e.message}`);
  }

  console.log(failures ? `\n${failures} falha(s)` : '\nTodas as telas renderizaram OK ✓');
  process.exit(failures ? 1 : 0);
} finally {
  await server.close();
}
