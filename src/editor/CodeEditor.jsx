import React, { useEffect, useMemo, useRef, useState } from 'react';
import { useStore } from '../store.jsx';
import { FileCode, Save } from 'lucide-react';

const KEYWORDS = new Set([
  'public', 'private', 'protected', 'internal', 'static', 'readonly', 'const', 'class', 'struct',
  'enum', 'interface', 'void', 'float', 'int', 'bool', 'string', 'double', 'long', 'byte', 'char',
  'new', 'return', 'if', 'else', 'for', 'foreach', 'while', 'do', 'switch', 'case', 'break',
  'continue', 'using', 'namespace', 'override', 'virtual', 'abstract', 'var', 'this', 'base',
  'true', 'false', 'null', 'get', 'set', 'property', 'try', 'catch', 'finally', 'throw',
  'in', 'out', 'ref', 'params', 'default', 'is', 'as', 'typeof', 'partial', 'async', 'await',
]);

function escapeHtml(s) {
  return s.replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;');
}

function highlightCs(code) {
  const esc = escapeHtml(code);
  const re =
    /(\/\/[^\n]*|\/\*[\s\S]*?\*\/)|("(?:[^"\\\n]|\\.)*")|('(?:[^'\\\n]|\\.)*')|\b(\d+\.?\d*[fLdMm]?)\b|([A-Za-z_][A-Za-z0-9_]*)/g;
  return esc.replace(re, (m, com, str, chr, num, ident) => {
    if (com) return `<span class="c-com">${com}</span>`;
    if (str) return `<span class="c-str">${str}</span>`;
    if (chr) return `<span class="c-str">${chr}</span>`;
    if (num) return `<span class="c-num">${num}</span>`;
    if (ident) {
      if (KEYWORDS.has(ident)) return `<span class="c-kw">${ident}</span>`;
      if (/^[A-Z]/.test(ident)) return `<span class="c-type">${ident}</span>`;
      if (ident.startsWith('res')) return `<span class="c-str">${ident}</span>`;
      return ident;
    }
    return m;
  });
}

export default function CodeEditor({ path }) {
  const { files, setFile, pushLog, pushToast } = useStore();
  const content = files[path] ?? '';
  const [text, setText] = useState(content);
  const taRef = useRef(null);
  const hlRef = useRef(null);
  const gutRef = useRef(null);

  useEffect(() => {
    setText(files[path] ?? '');
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [path]);

  const lines = useMemo(() => text.split('\n'), [text]);
  const highlighted = useMemo(() => `${highlightCs(text)}\n`, [text]);

  const onChange = (v) => {
    setText(v);
    setFile(path, v);
  };

  const syncScroll = () => {
    const ta = taRef.current;
    if (!ta) return;
    if (hlRef.current) {
      hlRef.current.scrollTop = ta.scrollTop;
      hlRef.current.scrollLeft = ta.scrollLeft;
    }
    if (gutRef.current) gutRef.current.scrollTop = ta.scrollTop;
  };

  const onKeyDown = (e) => {
    if (e.key === 'Tab') {
      e.preventDefault();
      const ta = e.target;
      const { selectionStart: s, selectionEnd: epos } = ta;
      const v = `${text.slice(0, s)}    ${text.slice(epos)}`;
      onChange(v);
      requestAnimationFrame(() => {
        ta.selectionStart = ta.selectionEnd = s + 4;
      });
    }
    if ((e.ctrlKey || e.metaKey) && e.key.toLowerCase() === 's') {
      e.preventDefault();
      pushLog('INFO', `Arquivo salvo: ${path}`);
      pushToast('Arquivo salvo ✓');
    }
  };

  const fileName = path.split('/').pop();

  return (
    <div className="code-editor">
      <div className="code-toolbar">
        <span className="code-file"><FileCode size={14} /> {path}</span>
        <span className="code-lang">C#</span>
        <button
          className="code-save"
          onClick={() => { pushLog('INFO', `Arquivo salvo: ${path}`); pushToast('Arquivo salvo ✓'); }}
        >
          <Save size={13} /> Salvar
        </button>
      </div>
      <div className="code-wrap">
        <div className="code-gutter" ref={gutRef}>
          {lines.map((_, i) => (
            <div key={i}>{i + 1}</div>
          ))}
        </div>
        <pre className="code-hl" ref={hlRef} dangerouslySetInnerHTML={{ __html: highlighted }} />
        <textarea
          ref={taRef}
          className="code-ta"
          value={text}
          spellCheck={false}
          onChange={(e) => onChange(e.target.value)}
          onScroll={syncScroll}
          onKeyDown={onKeyDown}
        />
      </div>
      <div className="code-status">
        <span>{fileName}</span>
        <span>UTF-8 · LF · C# · linhas: {lines.length}</span>
      </div>
    </div>
  );
}
