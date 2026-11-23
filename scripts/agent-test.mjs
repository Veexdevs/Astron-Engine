// Teste do loop do Agente IA com API OpenAI-compatible mockada.
// Valida: tool calling → write_file aplicado no projeto → finish.
import { createServer } from 'vite';

const server = await createServer({
  configFile: false,
  root: process.cwd(),
  logLevel: 'error',
  server: { middlewareMode: true, hmr: false },
  plugins: [(await import('@vitejs/plugin-react')).default()],
});

const calls = [];
const NEW_FILE = `using Astron.Game;

public class Enemy : Component
{
    public float Speed = 120f;
    public override void Update(float dt) { Transform.Position.X += Speed * dt; }
}
`;

globalThis.fetch = async (url, options) => {
  const body = JSON.parse(options.body);
  calls.push({ url: String(url), model: body.model, tools: (body.tools || []).map((t) => t.function?.name || t.name) });

  const hasToolResult = body.messages.some((m) => m.role === 'tool');
  let message;
  if (!hasToolResult) {
    message = {
      role: 'assistant',
      content: null,
      tool_calls: [
        {
          id: 'call_1',
          type: 'function',
          function: { name: 'write_file', arguments: JSON.stringify({ path: 'res://scripts/Enemy.cs', content: NEW_FILE }) },
        },
      ],
    };
  } else {
    message = {
      role: 'assistant',
      content: null,
      tool_calls: [
        {
          id: 'call_2',
          type: 'function',
          function: { name: 'finish', arguments: JSON.stringify({ summary: 'Criei o inimigo Enemy.cs com movimento.' }) },
        },
      ],
    };
  }
  return new Response(JSON.stringify({ choices: [{ message }] }), {
    status: 200,
    headers: { 'content-type': 'application/json' },
  });
};

try {
  const { runAgentTurn } = await server.ssrLoadModule('/src/ai/agent.js');

  const written = [];
  const events = [];
  const files = { 'res://main.cs': '// entry' };

  const result = await runAgentTurn({
    config: {
      provider: 'openrouter',
      apiKey: 'sk-or-test',
      model: 'openai/gpt-4o-mini',
      baseUrl: 'https://mock.openrouter.ai/api/v1',
      transport: 'direct',
      temperature: 0.3,
      maxSteps: 6,
    },
    files,
    messages: [{ role: 'user', content: 'Crie um inimigo' }],
    writeFile: (path, content) => {
      written.push(path);
      files[path] = content;
    },
    onEvent: (e) => events.push(e.type + (e.detail ? `:${e.detail}` : '')),
  });

  const asserts = [
    ['fetch chamado', calls.length === 2],
    ['tools enviadas', calls[0].tools.includes('write_file') && calls[0].tools.includes('list_files')],
    ['write_file aplicado', written.includes('res://scripts/Enemy.cs')],
    ['conteúdo íntegro', files['res://scripts/Enemy.cs'] === NEW_FILE],
    ['finish retornou resumo', result.text.includes('Enemy.cs')],
    ['filesTouched', result.filesTouched?.length === 1],
    ['eventos tool_done', events.some((e) => e.startsWith('tool_done'))],
    ['eventos done', events.some((e) => e.startsWith('done'))],
    ['histórico com tool result', result.messages.some((m) => m.role === 'tool')],
  ];

  let failed = 0;
  for (const [name, ok] of asserts) {
    console.log(`${ok ? '✔' : '✘'} ${name}`);
    if (!ok) failed++;
  }
  console.log(failed ? `\n${failed} falha(s)` : '\nAgente IA: loop autônomo OK ✓');
  process.exit(failed ? 1 : 0);
} catch (e) {
  console.error('✘ erro:', e.message);
  console.error((e.stack || '').split('\n').slice(1, 5).join('\n'));
  process.exit(1);
} finally {
  await server.close();
}
