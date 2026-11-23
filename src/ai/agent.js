// ============ Agente IA do Astron Engine ============
// Loop autônomo: o modelo usa ferramentas (list_files / read_file / write_file)
// e vai ENFIANDO O CÓDIGO DENTRO DO PROJETO sozinho, sem ajuda do usuário.

import { chatComplete } from './providers.js';

export const AGENT_TOOLS = [
  {
    type: 'function',
    function: {
      name: 'list_files',
      description:
        'Lista todos os arquivos do projeto Astron Engine (caminhos res://). Use antes de começar.',
      parameters: { type: 'object', properties: {}, required: [] },
    },
  },
  {
    type: 'function',
    function: {
      name: 'read_file',
      description: 'Lê o conteúdo completo de um arquivo do projeto.',
      parameters: {
        type: 'object',
        properties: {
          path: { type: 'string', description: 'Caminho do arquivo, ex.: res://scripts/Player.cs' },
        },
        required: ['path'],
      },
    },
  },
  {
    type: 'function',
    function: {
      name: 'write_file',
      description:
        'Cria ou SUBSTITUI um arquivo inteiro do projeto com o conteúdo dado. É assim que você escreve o código C# do jogo. Sempre envie o conteúdo COMPLETO do arquivo.',
      parameters: {
        type: 'object',
        properties: {
          path: { type: 'string', description: 'Caminho do arquivo, ex.: res://scripts/Enemy.cs' },
          content: { type: 'string', description: 'Conteúdo completo do arquivo' },
        },
        required: ['path', 'content'],
      },
    },
  },
  {
    type: 'function',
    function: {
      name: 'finish',
      description:
        'Chame quando o pedido do usuário estiver totalmente implementado, com um resumo do que foi feito.',
      parameters: {
        type: 'object',
        properties: {
          summary: { type: 'string', description: 'Resumo em português do que foi criado/alterado' },
        },
        required: ['summary'],
      },
    },
  },
];

function buildFileTreeText(files) {
  const paths = Object.keys(files).sort();
  if (!paths.length) return '(projeto vazio)';
  return paths.map((p) => `- ${p} (${files[p].length} bytes)`).join('\n');
}

function buildSystemPrompt(files) {
  return `Você é o AGENTE IA OFICIAL do Astron Engine — uma engine de criação de jogos cujos jogos são programados em C# (.NET).

Sua missão: implementar COMPLETAMENTE o pedido do usuário escrevendo os arquivos do projeto, usando as ferramentas, SEM pedir ajuda e SEM perguntar o que deve fazer. Você faz o jogo inteiro.

REGRAS:
1. Fale sempre em português do Brasil.
2. Todo o código dos jogos é C# (targets .NET / Astron.Engine). Nada de Lua, JS ou GDScript.
3. Use a ferramenta write_file com o CONTEÚDO COMPLETO de cada arquivo (nunca trechos parciais nem "..." ).
4. Comece listando os arquivos (list_files) para entender a estrutura; leia arquivos que precisar alterar (read_file).
5. Estrutura padrão do projeto:
   - res://main.cs — ponto de entrada
   - res://default.scene — cena (JSON)
   - res://scripts/*.cs — componentes e regras do jogo (Component, Scene)
   - res://models, res://textures, res://sounds, res://ui — assets
   APIs disponíveis em Astron.Engine: GameWindow, Scene, Component, Transform, Input.GetAxis/JumpPressed, Physics.IsGrounded, Audio.Play, HUD.SetText, Spawn(), Pickup, Camera, Vector2.
6. Prefira arquivos novos em res://scripts/ ao modificar os existentes, a menos que o usuário peça alteração.
7. Se o pedido for vago ("faça um jogo"), escolha um gênero divertido e implemente o jogo COMPLETO: player, movimento, inimigos/obstáculos, pontuação, condição de vitória/derrota.
8. Termine chamando a ferramenta finish com um resumo do que foi criado.
9. Nunca invente URLs externas nem peça sua ajuda. Se não houver chave de API, avise uma única vez.

Arquivos atuais do projeto:
${buildFileTreeText(files)}

Responda de forma curta e objetiva; o código vai pelos arquivos, não pelo chat.`;
}

/**
 * Executa um turno do agente.
 * @param {object} opts
 * @param {object} opts.config   config de IA (providers.js)
 * @param {object} opts.files    mapa path -> conteúdo (snapshot atual)
 * @param {Array}  opts.messages histórico OpenAI-style
 * @param {(evt)=>void} opts.onEvent   eventos: status | text | tool_start | tool_done | done
 * @param {(path,content)=>void} opts.writeFile
 * @returns {Promise<{messages:Array, text:string, filesTouched:string[]}>}
 */
export async function runAgentTurn({ config, files, messages, onEvent, writeFile }) {
  let msgs = [...messages];
  const filesTouched = [];
  let finalText = '';
  const maxSteps = Number(config.maxSteps || 24);

  for (let step = 0; step < maxSteps; step++) {
    onEvent?.({ type: 'status', text: step === 0 ? 'Pensando…' : `Executando passo ${step + 1}…` });

    const reply = await chatComplete({
      config,
      messages: [
        { role: 'system', content: buildSystemPrompt(files) },
        ...msgs.filter((m) => m.role !== 'system'),
      ],
      tools: AGENT_TOOLS,
    });

    if (reply.content) {
      finalText = reply.content;
      msgs.push({ role: 'assistant', content: reply.content });
      onEvent?.({ type: 'text', text: reply.content });
    }

    if (!reply.toolCalls.length) {
      onEvent?.({ type: 'done', text: finalText });
      return { messages: msgs, text: finalText, filesTouched };
    }

    msgs.push({
      role: 'assistant',
      content: reply.content || null,
      tool_calls: reply.toolCalls.map((tc) => ({
        id: tc.id,
        type: 'function',
        function: {
          name: tc.name,
          arguments: JSON.stringify(tc.args || {}),
        },
      })),
    });

    for (const call of reply.toolCalls) {
      const name = call.name;
      const args = call.args || {};
      let result = '';

      if (name === 'list_files') {
        onEvent?.({ type: 'tool_start', name, label: 'list_files' });
        result = Object.keys(files)
          .sort()
          .join('\n');
        onEvent?.({ type: 'tool_done', name, label: 'list_files', detail: `${Object.keys(files).length} arquivos` });
      } else if (name === 'read_file') {
        onEvent?.({ type: 'tool_start', name, label: 'read_file', detail: args.path });
        const content = files[args.path];
        result = content !== undefined ? content : `ERRO: arquivo não encontrado: ${args.path}`;
        onEvent?.({ type: 'tool_done', name, label: 'read_file', detail: args.path });
      } else if (name === 'write_file') {
        const path = String(args.path || '').trim();
        if (!path || typeof args.content !== 'string') {
          result = 'ERRO: write_file exige "path" e "content" (string).';
        } else {
          const norm = path.startsWith('res://') ? path : `res://${path.replace(/^\/+/, '')}`;
          onEvent?.({ type: 'tool_start', name, label: 'write_file', detail: norm });
          files[norm] = args.content;
          filesTouched.push(norm);
          writeFile(norm, args.content);
          result = `OK: ${norm} gravado (${args.content.length} bytes).`;
          onEvent?.({ type: 'tool_done', name, label: 'write_file', detail: norm, ok: true });
        }
      } else if (name === 'finish') {
        onEvent?.({ type: 'tool_start', name, label: 'finish' });
        result = 'Concluído.';
        onEvent?.({ type: 'tool_done', name, label: 'finish', detail: args.summary || '' });
        msgs.push({ role: 'tool', tool_call_id: call.id, content: result });
        onEvent?.({ type: 'done', text: args.summary || finalText, filesTouched });
        return { messages: msgs, text: args.summary || finalText, filesTouched };
      } else {
        result = `ERRO: ferramenta desconhecida "${name}".`;
      }

      msgs.push({ role: 'tool', tool_call_id: call.id, content: result });
    }
  }

  onEvent?.({ type: 'done', text: finalText, filesTouched });
  return { messages: msgs, text: finalText, filesTouched };
}
