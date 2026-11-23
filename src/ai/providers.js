// ============ Provedores de IA — qualquer API compatível com OpenAI + Anthropic ============
// Cobre: OpenRouter, OpenAI, Anthropic, Groq, Mistral, DeepSeek, Google Gemini
// e qualquer endpoint personalizado (compatível com OpenAI).

export const PROVIDERS = {
  openrouter: {
    label: 'OpenRouter',
    kind: 'openai',
    base: 'https://openrouter.ai/api/v1',
    models: [
      'openai/gpt-4o-mini',
      'openai/gpt-4o',
      'anthropic/claude-3.7-sonnet',
      'anthropic/claude-3.5-sonnet',
      'google/gemini-2.0-flash-001',
      'meta-llama/llama-3.3-70b-instruct',
      'qwen/qwen-2.5-coder-32b-instruct',
      'deepseek/deepseek-chat',
      'mistralai/mistral-large',
    ],
    hint: 'Acesse centenas de modelos em um só lugar — openrouter.ai/api/keys',
  },
  openai: {
    label: 'OpenAI',
    kind: 'openai',
    base: 'https://api.openai.com/v1',
    models: ['gpt-4o-mini', 'gpt-4o', 'o3-mini', 'gpt-4.1-mini'],
    hint: 'platform.openai.com/api-keys',
  },
  anthropic: {
    label: 'Anthropic (Claude)',
    kind: 'anthropic',
    base: 'https://api.anthropic.com',
    models: ['claude-sonnet-4-5', 'claude-opus-4-1', 'claude-3-5-haiku-latest'],
    hint: 'console.anthropic.com',
  },
  groq: {
    label: 'Groq',
    kind: 'openai',
    base: 'https://api.groq.com/openai/v1',
    models: ['llama-3.3-70b-versatile', 'llama-3.1-8b-instant', 'qwen-2.5-coder-32b'],
    hint: 'console.groq.com/keys',
  },
  mistral: {
    label: 'Mistral',
    kind: 'openai',
    base: 'https://api.mistral.ai/v1',
    models: ['mistral-large-latest', 'codestral-latest', 'mistral-small-latest'],
    hint: 'console.mistral.ai/api-keys',
  },
  deepseek: {
    label: 'DeepSeek',
    kind: 'openai',
    base: 'https://api.deepseek.com/v1',
    models: ['deepseek-chat', 'deepseek-reasoner'],
    hint: 'platform.deepseek.com/api_keys',
  },
  gemini: {
    label: 'Google Gemini',
    kind: 'openai',
    base: 'https://generativelanguage.googleapis.com/v1beta/openai',
    models: ['gemini-2.0-flash', 'gemini-2.5-flash', 'gemini-1.5-pro'],
    hint: 'aistudio.google.com/apikey (API compatível com OpenAI)',
  },
  custom: {
    label: 'Personalizada (compatível com OpenAI)',
    kind: 'openai',
    base: 'http://localhost:11434/v1',
    models: ['llama3.2', 'qwen2.5-coder'],
    hint: 'Qualquer endpoint que implemente /chat/completions (Ollama, LM Studio, vLLM...)',
  },
};

export function providerInfo(key) {
  return PROVIDERS[key] || PROVIDERS.custom;
}

// Monta a URL final do endpoint a partir da base configurada
function endpointUrl(baseUrl, kind) {
  const base = (baseUrl || '').replace(/\/+$/, '');
  if (kind === 'anthropic') return `${base}/v1/messages`;
  return `${base}/chat/completions`;
}

// Divide origem + caminho para uso com o proxy (/ai-proxy)
function splitOrigin(url) {
  try {
    const u = new URL(url);
    return { origin: u.origin, path: u.pathname + u.search };
  } catch {
    return { origin: 'https://openrouter.ai', path: url };
  }
}

async function doFetch(url, options, transport) {
  if (transport === 'direct') {
    return fetch(url, options);
  }
  const { origin, path } = splitOrigin(url);
  const headers = { ...(options.headers || {}), 'x-ai-origin': origin };
  const proxied = await fetch(`/ai-proxy${path}`, { ...options, headers });
  // Em deploy estático (ex.: Vercel / GitHub Pages) sem middleware /ai-proxy, faz fallback direto
  if (proxied.status === 404 || proxied.status === 405) {
    const ct = proxied.headers?.get?.('content-type') || '';
    if (ct.includes('text/html')) {
      return fetch(url, options);
    }
  }
  return proxied;
}

function raiseFor(res, bodyText) {
  let detail = bodyText.slice(0, 400);
  try {
    const j = JSON.parse(bodyText);
    detail = j?.error?.message || j?.error?.type || j?.message || detail;
  } catch { /* mantém texto cru */ }
  if (res.status === 401 || res.status === 403) {
    throw new Error(`Chave de API recusada (${res.status}). Verifique a chave em Configurações → Agente IA. ${detail}`);
  }
  if (res.status === 429) {
    throw new Error(`Limite de requisições atingido (429). ${detail}`);
  }
  if (!res.ok) {
    throw new Error(`Erro ${res.status} na API de IA: ${detail}`);
  }
}

// ---------- Chamada: OpenAI-compatible (OpenRouter, OpenAI, Groq, Gemini...) ----------
async function callOpenAICompat({ config, messages, tools }) {
  const url = endpointUrl(config.baseUrl, 'openai');
  const isOpenRouter = (config.baseUrl || '').includes('openrouter.ai') || config.provider === 'openrouter';
  const extraHeaders = isOpenRouter
    ? { 'X-Title': 'Astron Engine' }
    : {};

  const res = await doFetch(
    url,
    {
      method: 'POST',
      headers: {
        'content-type': 'application/json',
        authorization: `Bearer ${config.apiKey}`,
        ...extraHeaders,
      },
      body: JSON.stringify({
        model: config.model,
        messages,
        tools: tools?.length ? tools : undefined,
        temperature: Number(config.temperature ?? 0.4),
        max_tokens: 4096,
      }),
    },
    config.transport
  );
  const text = await res.text();
  raiseFor(res, text);
  const data = JSON.parse(text);
  const msg = data.choices?.[0]?.message || {};
  return {
    content: msg.content || '',
    toolCalls: (msg.tool_calls || []).map((tc) => ({
      id: tc.id || `call_${Math.random().toString(36).slice(2)}`,
      name: tc.function?.name,
      args: safeParse(tc.function?.arguments),
    })),
  };
}

// ---------- Chamada: Anthropic (Messages API) ----------
async function callAnthropic({ config, messages, tools }) {
  const url = endpointUrl(config.baseUrl, 'anthropic');
  const system = messages.find((m) => m.role === 'system')?.content || '';
  const rest = [];

  for (const m of messages) {
    if (m.role === 'system') continue;
    if (m.role === 'tool') {
      const block = {
        type: 'tool_result',
        tool_use_id: m.tool_call_id,
        content: String(m.content),
      };
      const prev = rest[rest.length - 1];
      if (prev && prev.role === 'user' && Array.isArray(prev.content)) {
        prev.content.push(block);
      } else {
        rest.push({ role: 'user', content: [block] });
      }
      continue;
    }
    if (m.role === 'assistant' && m.tool_calls) {
      rest.push({
        role: 'assistant',
        content: [
          ...(m.content ? [{ type: 'text', text: m.content }] : []),
          ...m.tool_calls.map((tc) => ({
            type: 'tool_use',
            id: tc.id,
            name: tc.name || tc.function?.name,
            input: tc.args || safeParse(tc.function?.arguments),
          })),
        ],
      });
      continue;
    }
    rest.push({ role: m.role, content: m.content });
  }

  const res = await doFetch(
    url,
    {
      method: 'POST',
      headers: {
        'content-type': 'application/json',
        'x-api-key': config.apiKey,
        'anthropic-version': '2023-06-01',
      },
      body: JSON.stringify({
        model: config.model,
        max_tokens: 4096,
        temperature: Number(config.temperature ?? 0.4),
        system,
        messages: rest,
        tools: tools?.map((t) => ({
          name: t.function.name,
          description: t.function.description,
          input_schema: t.function.parameters,
        })) || undefined,
      }),
    },
    config.transport
  );
  const text = await res.text();
  raiseFor(res, text);
  const data = JSON.parse(text);
  const content = Array.isArray(data.content) ? data.content : [];
  const toolCalls = content
    .filter((b) => b.type === 'tool_use')
    .map((b) => ({ id: b.id, name: b.name, args: b.input || {} }));
  const textPart = content
    .filter((b) => b.type === 'text')
    .map((b) => b.text)
    .join('\n');
  return { content: textPart, toolCalls };
}

function safeParse(s) {
  if (!s) return {};
  if (typeof s === 'object') return s;
  try {
    return JSON.parse(s);
  } catch {
    return {};
  }
}

// Ponto de entrada único — escolhe o adaptador certo
export async function chatComplete(args) {
  const info = providerInfo(args.config.provider);
  if (!args.config.apiKey) {
    throw new Error(
      'Nenhuma chave de API configurada. Abra Configurações → Agente IA e cole sua chave (ex.: OpenRouter).'
    );
  }
  if (info.kind === 'anthropic') return callAnthropic(args);
  return callOpenAICompat(args);
}
