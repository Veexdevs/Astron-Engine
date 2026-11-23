import { defineConfig } from 'vite';
import react from '@vitejs/plugin-react';

// Proxy de IA: permite chamar QUALQUER API de IA (OpenRouter, OpenAI, Anthropic,
// Groq, Mistral, Gemini, DeepSeek, personalizada...) sem problemas de CORS,
// roteando o alvo pelo header `x-ai-origin`.
export default defineConfig({
  plugins: [react()],
  server: {
    host: '0.0.0.0',
    port: 5173,
    allowedHosts: true,
    proxy: {
      '/ai-proxy': {
        changeOrigin: true,
        secure: true,
        rewrite: (path) => path.replace(/^\/ai-proxy/, ''),
        router: (req) => req.headers['x-ai-origin'] || 'https://openrouter.ai',
      },
    },
  },
  build: {
    outDir: 'dist',
    chunkSizeWarningLimit: 1600,
  },
});
