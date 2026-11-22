import { useCallback, useEffect, useMemo, useRef, useState } from 'react';
import { PencilRuler, Printer, Settings2, Plus, Upload, Download, Send, CircleHelp, Menu, X, Sparkles, FileCode2, Clock3, Waypoints, Check, Wifi, Leaf, Pencil, HardDrive } from 'lucide-react';
import { Logo, Button, Modal, Toast } from './components/ui';
import DrawingCanvas from './components/DrawingCanvas';
import EggPreview from './components/EggPreview';
import PrintPanel from './components/PrintPanel';
import SettingsPanel from './components/SettingsPanel';
import { DEFAULT_CONFIG, PATTERNS, pattern, prepareDrawing, estimate, formatTime, download } from './lib/drawing';
import { importSvg } from './lib/svg';
import { IS_DEMO, useMachine } from './hooks/useMachine';

const draftKey = 'egg-studio-draft-v1';
function loadDraft() {
  try {
    const draft = JSON.parse(localStorage.getItem(draftKey));
    if (draft && typeof draft.name === 'string' && Array.isArray(draft.strokes) && draft.strokes.every(s => Array.isArray(s) && s.every(p => Array.isArray(p) && p.length === 2 && p.every(Number.isFinite)))) return draft;
  } catch { /* O estúdio também funciona sem armazenamento local. */ }
  const flower = pattern('flora');
  return { name: 'Flora', strokes: [flower, flower.map(([r, a]) => [Math.round(800 + (r - 800) * .58), Math.round(a * .58)])] };
}
function PatternArt({ id }) {
  const points = pattern(id, { ...DEFAULT_CONFIG, arm_min: -400, arm_max: 400 });
  const min = Math.min(...points.map(p => p[0])), max = Math.max(...points.map(p => p[0]));
  const span = Math.max(max - min, 800);
  const path = points.map(([x, y], i) => `${i ? 'L' : 'M'}${10 + (x - (min + max) / 2 + span / 2) / span * 64},${40 - y / 400 * 28}`).join(' ');
  return <svg viewBox="0 0 84 80" aria-hidden="true"><path d={path} fill="none" stroke="currentColor" strokeWidth="1.5" strokeLinecap="round" strokeLinejoin="round" /></svg>;
}

export default function App() {
  const [initial] = useState(loadDraft), [name, setName] = useState(initial.name), [strokes, setStrokes] = useState(initial.strokes), [future, setFuture] = useState([]);
  const [tab, setTab] = useState('create'), [view, setView] = useState(0), [menu, setMenu] = useState(false), [modal, setModal] = useState(null), [toast, setToast] = useState(null), [svgWidth, setSvgWidth] = useState(1600), [saved, setSaved] = useState('saved');
  const input = useRef(), machine = useMachine(), cfg = machine.status?.cfg || DEFAULT_CONFIG;
  const plan = useMemo(() => prepareDrawing(strokes, cfg, name), [strokes, cfg.arm_min, cfg.arm_max, name]);
  const seconds = useMemo(() => estimate(plan.strokes, cfg), [plan, cfg.speed_down, cfg.speed_up, cfg.pen_ms]);
  const notify = useCallback((message, error = false) => setToast({ message, error }), []), closeToast = useCallback(() => setToast(null), []);
  const act = useCallback(async (fn, message) => { try { const result = await fn(); if (message) notify(message); return result; } catch (error) { notify(error.message || 'Não foi possível concluir a ação.', true); return null; } }, [notify]);
  const confirm = (title, text, action) => setModal({ type: 'confirm', title, text, action });
  useEffect(() => {
    setSaved('saving');
    const timer = setTimeout(() => { try { localStorage.setItem(draftKey, JSON.stringify({ name, strokes })); setSaved('saved'); } catch { setSaved('unavailable'); } }, 450);
    return () => clearTimeout(timer);
  }, [name, strokes]);
  const add = newStrokes => { setStrokes(s => [...s, ...newStrokes]); setFuture([]); };
  const undo = () => { if (!strokes.length) return; setFuture(f => [...f, strokes.at(-1)]); setStrokes(s => s.slice(0, -1)); };
  const redo = () => { if (!future.length) return; setStrokes(s => [...s, future.at(-1)]); setFuture(f => f.slice(0, -1)); };
  const clear = () => confirm('Começar com uma tela limpa?', 'Os traços deste desenho serão removidos. Você pode baixar o arquivo antes de continuar.', () => { setStrokes([]); setFuture([]); setName('Sem título'); setView(0); });
  const navigate = page => { setTab(page); setMenu(false); };
  const machineBusy = machine.unavailable || machine.printing;
  const selectPattern = id => { add([pattern(id, cfg, view)]); notify(`${PATTERNS.find(p => p.id === id).name} adicionado ao desenho.`); };
  async function onImport(event) {
    const file = event.target.files?.[0]; event.target.value = '';
    if (!file) return;
    await act(async () => {
      const paths = importSvg(await file.text(), cfg, svgWidth, view); add(paths);
      if (!strokes.length) setName(file.name.replace(/\.svg$/i, '').slice(0, 40));
      notify(`${paths.length} traço${paths.length !== 1 ? 's' : ''} importado${paths.length !== 1 ? 's' : ''}.`);
    });
  }
  return <div className="app-shell">
    {menu && <button className="sidebar-scrim" aria-label="Fechar navegação" onClick={() => setMenu(false)} />}
    <aside className={`sidebar ${menu ? 'sidebar-open' : ''}`}>
      <div className="sidebar-brand"><Logo /><button className="mobile-close" aria-label="Fechar navegação" onClick={() => setMenu(false)}><X size={20} /></button></div>
      <div className="sidebar-caption">IDEIAS QUE GANHAM FORMA</div>
      <div className="sidebar-section-label">SEU ESPAÇO</div>
      <nav aria-label="Navegação principal">{[['create', PencilRuler, 'Criar desenho'], ['print', Printer, 'Imprimir'], ['settings', Settings2, 'Configuração']].map(([id, Icon, label]) => <button key={id} className={`nav-item ${tab === id ? 'selected' : ''}`} aria-current={tab === id ? 'page' : undefined} onClick={() => navigate(id)}><Icon size={20} strokeWidth={1.6} /><span>{label}</span>{id === 'print' && machine.status?.job.present && <span className="nav-count">1</span>}</button>)}</nav>
      <div className="sidebar-inspiration"><div className="inspiration-art"><svg viewBox="0 0 150 90" aria-hidden="true">{Array.from({ length: 8 }, (_, i) => <ellipse key={i} cx="75" cy="45" rx={15 + i * 6} ry="29" transform={`rotate(${i * 20} 75 45)`} fill="none" stroke="currentColor" strokeWidth=".8" />)}</svg></div><p>Uma superfície pequena.<br /><strong>Infinitas possibilidades.</strong></p></div>
      <div className="sidebar-bottom"><div className="machine-card"><div className="flex items-center gap-2"><span className={`connection-dot ${machine.online ? '' : 'offline'}`} /><strong>{IS_DEMO ? 'Modo demonstração' : machine.online ? 'EggBot conectado' : 'EggBot desconectado'}</strong></div><p>{IS_DEMO ? 'Explore. Nenhuma placa conectada.' : machine.status?.ssid || 'Verifique a rede do equipamento.'}</p><div className="machine-card-footer"><span>{IS_DEMO ? 'SIMULADOR LOCAL' : 'REDE LOCAL'}</span><Wifi size={14} /></div></div><button className="help-link" onClick={() => setModal({ type: 'help' })}><CircleHelp size={17} /> Como funciona<span>?</span></button><div className="sidebar-signature">FEITO PARA CRIAR. FEITO POR VOCÊ.</div></div>
    </aside>

    <div className="main-shell">
      <header className="topbar"><div className="flex min-w-0 items-center gap-3"><button className="menu-toggle" aria-label="Abrir navegação" onClick={() => setMenu(true)}><Menu size={22} /></button><span className="topbar-home">Workspace</span><span className="text-[#c2c9c3]">/</span><span className="font-semibold">{tab === 'create' ? 'Estúdio de criação' : tab === 'print' ? 'Impressão' : 'Configuração'}</span></div><div className="flex items-center gap-4"><span className="topbar-version">EGG STUDIO <span>01</span></span><div className="avatar" title="Espaço local de criação"><Leaf size={18} strokeWidth={1.5} /></div></div></header>
      <main className="main-content">
        <div className="page-heading"><div><div className="eyebrow text-muted">{tab === 'create' ? 'DA IMAGINAÇÃO PARA A SUPERFÍCIE' : tab === 'print' ? 'A HORA DE DAR VIDA' : 'CADA DETALHE FAZ DIFERENÇA'}</div><h1>{tab === 'create' ? <>Dê uma volta na <span>criatividade.</span></> : tab === 'print' ? <>Do traço à <span>casca.</span></> : <>No seu <span>ritmo.</span></>}</h1><p>{tab === 'create' ? 'Desenhe, experimente e transforme um ovo em algo só seu.' : tab === 'print' ? 'Acompanhe cada movimento da sua próxima criação.' : 'Um pouco de ajuste. Um mundo de possibilidades.'}</p></div>{tab === 'create' && <Button icon={Plus} onClick={clear}>Novo desenho</Button>}</div>

        {tab === 'create' && <div className="animate-in">
          <div className="workspace-title"><div className="flex min-w-0 items-center gap-2"><input aria-label="Nome do desenho" value={name} maxLength={40} onChange={e => setName(e.target.value)} className="project-name" /><Pencil size={13} className="shrink-0 text-muted" /><span className="draft-badge">RASCUNHO</span></div><span className="save-indicator">{saved === 'saved' ? <Check size={13} /> : <HardDrive size={13} />}{saved === 'saved' ? 'Salvo neste navegador' : saved === 'saving' ? 'Salvando rascunho…' : 'Rascunho não salvo'}</span></div>
          <div className="studio-layout"><div className="min-w-0 space-y-5"><DrawingCanvas strokes={strokes} onStroke={stroke => add([stroke])} config={cfg} view={view} setView={setView} undo={undo} redo={redo} canRedo={!!future.length} clear={clear} name={name} />
            <section className="patterns-section"><div className="mb-4 flex items-center justify-between"><div className="flex items-center gap-2"><Sparkles size={17} className="text-accent" /><h2 className="text-sm font-bold">Um ponto de partida</h2></div><span className="text-xs text-muted">Clique para adicionar</span></div><div className="patterns-grid">{PATTERNS.map(p => <button className="pattern-card" key={p.id} onClick={() => selectPattern(p.id)} aria-label={`Adicionar padrão ${p.name}`} title={p.caption}><PatternArt id={p.id} /><span>{p.name}</span><Plus size={12} className="pattern-plus" /></button>)}</div></section>
            <section className="import-strip"><div className="import-icon"><Upload size={21} strokeWidth={1.5} /></div><div className="min-w-0 flex-1"><h2>Já tem uma ideia no papel?</h2><p>Importe um SVG e continue a criação por aqui.</p></div><Button onClick={() => setModal({ type: 'import' })}>Importar SVG</Button></section>
          </div><aside className="studio-aside"><EggPreview file={plan.file} source="Projeção ilustrativa · desenho atual" /><section className="project-summary"><div className="flex items-center justify-between"><h2>Pronto para a próxima volta?</h2><span className="text-accent"><Send size={17} /></span></div><div className="summary-numbers"><div><Waypoints size={16} /><strong>{plan.points}</strong><span>pontos</span></div><div><Clock3 size={16} /><strong>{formatTime(seconds)}</strong><span>estimados</span></div></div>{plan.clipped > 0 && <p className="mb-3 text-xs text-amber-800">{plan.clipped} pontos ajustados aos limites da área.</p>}{plan.tooLarge && <p role="alert" className="mb-3 text-xs text-red-700">O desenho excede 64 KB ou 8000 linhas. Simplifique antes de enviar.</p>}<Button variant="primary" icon={Send} className="w-full" disabled={!plan.file || plan.tooLarge || machineBusy} onClick={() => act(async () => { await machine.upload(plan.file); notify(IS_DEMO ? 'Desenho enviado ao simulador. Pronto para imprimir.' : 'Desenho enviado ao EggBot. Pronto para imprimir.'); navigate('print'); })}>{machine.busy ? 'Enviando…' : IS_DEMO ? 'Enviar ao simulador' : 'Enviar para o EggBot'}</Button><div className="mt-3 flex items-center justify-center gap-4"><button className="text-action" disabled={!plan.file} onClick={() => download(plan.file, `${name.replace(/[^a-zA-Z0-9_-]/g, '_') || 'desenho'}.egg`)}><Download size={13} /> Baixar desenho</button><button className="text-action" disabled={!plan.file} onClick={() => setModal({ type: 'file' })}><FileCode2 size={13} /> Ver arquivo</button></div>{!machine.online && <p className="mt-3 text-xs text-red-700">Sem conexão. Seu desenho continua disponível para edição e download.</p>}</section></aside></div>
          <footer className="studio-footer"><span><span className="footer-dot" />{IS_DEMO ? 'Você está criando em modo demonstração' : machine.online ? 'Conectado à rede do EggBot' : 'Trabalhando offline'}</span><span>Desenhe livre. Pense fora da casca.</span></footer>
        </div>}
        {tab === 'print' && <PrintPanel machine={machine} act={act} confirm={confirm} goCreate={() => navigate('create')} />}
        {tab === 'settings' && <SettingsPanel machine={machine} act={act} notify={notify} />}
      </main>
    </div>

    <input ref={input} type="file" accept=".svg,image/svg+xml" className="hidden" aria-label="Arquivo SVG" onChange={onImport} />
    {modal?.type === 'confirm' && <Modal title={modal.title} onClose={() => setModal(null)}><p className="text-sm leading-6 text-muted">{modal.text}</p><div className="mt-6 flex justify-end gap-3"><Button onClick={() => setModal(null)}>Voltar</Button><Button variant="primary" onClick={() => { modal.action(); setModal(null); }}>Confirmar</Button></div></Modal>}
    {modal?.type === 'import' && <Modal title="Traga seu desenho para o estúdio" onClose={() => setModal(null)}><div className="import-drop"><Upload size={30} strokeWidth={1.3} /><h3>Seu SVG, uma nova superfície.</h3><p>Importamos os contornos. Para letras, converta o texto em caminhos no seu editor.</p><label className="mt-5 flex items-center justify-center gap-3 text-sm">Área do desenho<select value={svgWidth} onChange={e => setSvgWidth(+e.target.value)}><option value={1600}>Meia volta</option><option value={3150}>Volta inteira</option></select></label><Button variant="primary" icon={Upload} className="mt-5" onClick={() => { setModal(null); input.current.click(); }}>Escolher arquivo SVG</Button><span className="mt-3 block text-xs text-muted">Até 2 MB · somente contornos vetoriais</span></div></Modal>}
    {modal?.type === 'file' && <Modal title="Seu desenho, ponto a ponto" onClose={() => setModal(null)} wide><div className="mb-4 flex flex-wrap gap-4 text-xs text-muted"><span>{plan.lines} linhas</span><span>{(plan.bytes / 1024).toFixed(1)} KB</span><span>EGGJOB 1 · CRC-32</span></div><pre className="file-preview">{plan.file}</pre><Button icon={Download} className="mt-5" onClick={() => download(plan.file, 'desenho.egg')}>Baixar arquivo</Button></Modal>}
    {modal?.type === 'help' && <Modal title="Da primeira ideia ao último traço" onClose={() => setModal(null)}><div className="help-steps">{[[PencilRuler, 'Crie no seu ritmo', 'Desenhe com o mouse ou o dedo, explore os padrões ou importe um SVG. Seu rascunho fica salvo neste navegador.'], [Leaf, 'Veja de outro ângulo', 'Gire a prévia para explorar a superfície. A projeção é ilustrativa; o resultado depende da calibração e da forma do ovo.'], [Printer, 'Dê vida ao desenho', 'Envie o trabalho, confira a montagem e inicie na aba Imprimir. Você pode pausar ou cancelar.']].map(([Icon, title, text]) => <div key={title}><span><Icon size={23} /></span><div><h3>{title}</h3><p>{text}</p></div></div>)}</div><div className="notice mt-5">{IS_DEMO ? 'Modo demonstração: nenhuma placa recebe comandos. Os movimentos simulados são acelerados em 4×.' : 'Modo equipamento: os controles enviam comandos reais ao EggBot.'}</div></Modal>}
    {toast && <Toast {...toast} onClose={closeToast} />}
  </div>;
}
