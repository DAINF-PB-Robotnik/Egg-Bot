import { useEffect, useRef, useState } from 'react';
import { Pencil, Undo2, Redo2, Trash2, Grid2X2, Minus, Plus, ChevronLeft, ChevronRight, Maximize2 } from 'lucide-react';
import { IconButton } from './ui';

export default function DrawingCanvas({ strokes, onStroke, config, view, setView, undo, redo, canRedo, clear, name }) {
  const canvas = useRef(), active = useRef(null);
  const [grid, setGrid] = useState(true), [zoom, setZoom] = useState(100), [cursor, setCursor] = useState(null);
  const span = 1600 * 100 / zoom;
  const drawRef = useRef();
  drawRef.current = () => {
    const el = canvas.current;
    if (!el) return;
    const { width, height } = el.getBoundingClientRect(), d = window.devicePixelRatio || 1;
    el.width = Math.round(width * d); el.height = Math.round(height * d);
    const ctx = el.getContext('2d'); ctx.scale(d, d); ctx.clearRect(0, 0, width, height);
    const x = r => (r - view) / span * width, y = a => (config.arm_max - a) / (config.arm_max - config.arm_min) * height;
    if (grid) {
      ctx.fillStyle = '#d8e0da';
      for (let px = 16; px < width; px += 20) for (let py = 16; py < height; py += 20) { ctx.beginPath(); ctx.arc(px, py, .75, 0, Math.PI * 2); ctx.fill(); }
      ctx.strokeStyle = '#d7ded9'; ctx.lineWidth = 1; ctx.setLineDash([4, 6]); ctx.beginPath(); ctx.moveTo(0, y(0)); ctx.lineTo(width, y(0)); ctx.stroke(); ctx.setLineDash([]);
    }
    ctx.strokeStyle = '#346b53'; ctx.lineWidth = 2.1; ctx.lineCap = ctx.lineJoin = 'round';
    for (const stroke of [...strokes, ...(active.current ? [active.current] : [])]) {
      if (!stroke.length) continue;
      ctx.beginPath(); ctx.moveTo(x(stroke[0][0]), y(stroke[0][1]));
      for (const [r, a] of stroke.slice(1)) ctx.lineTo(x(r), y(a));
      if (stroke.length === 1) ctx.lineTo(x(stroke[0][0]) + .2, y(stroke[0][1]));
      ctx.stroke();
    }
  };
  useEffect(() => { drawRef.current(); }, [strokes, config.arm_min, config.arm_max, view, span, grid]);
  useEffect(() => { const ro = new ResizeObserver(() => drawRef.current()); ro.observe(canvas.current); return () => ro.disconnect(); }, []);
  function point(e) {
    const bounds = canvas.current.getBoundingClientRect();
    return [Math.round(view + Math.max(0, Math.min(bounds.width, e.clientX - bounds.left)) / bounds.width * span), Math.round(config.arm_max - Math.max(0, Math.min(bounds.height, e.clientY - bounds.top)) / bounds.height * (config.arm_max - config.arm_min))];
  }
  function finish() { if (!active.current) return; onStroke(active.current); active.current = null; drawRef.current(); }
  return <section className="drawing-panel" aria-label="Editor de desenho">
    <div className="canvas-toolbar">
      <div className="flex items-center gap-1"><span className="tool-selected" title="Desenho livre"><Pencil size={18} /><span>Desenhar</span></span><span className="toolbar-divider" /><IconButton icon={Undo2} label="Desfazer" onClick={undo} disabled={!strokes.length} /><IconButton icon={Redo2} label="Refazer" onClick={redo} disabled={!canRedo} /><IconButton icon={Trash2} label="Limpar desenho" onClick={clear} disabled={!strokes.length} /></div>
      <div className="flex items-center gap-1"><IconButton icon={Grid2X2} label="Mostrar grade" aria-pressed={grid} active={grid} onClick={() => setGrid(!grid)} /><span className="toolbar-divider" /><IconButton icon={Minus} label="Diminuir zoom" disabled={zoom <= 50} onClick={() => setZoom(z => z - 25)} /><span className="zoom-label">{zoom}%</span><IconButton icon={Plus} label="Aumentar zoom" disabled={zoom >= 200} onClick={() => setZoom(z => z + 25)} /><IconButton icon={Maximize2} label="Restaurar vista" onClick={() => { setZoom(100); setView(0); }} /></div>
    </div>
    <div className="canvas-paper">
      <div className="paper-label"><span className="inline-block h-1.5 w-1.5 rounded-full bg-accent" /> SUPERFÍCIE ABERTA <span>01</span></div>
      <canvas ref={canvas} aria-label="Área de desenho livre" style={{ aspectRatio: `${span} / ${config.arm_max - config.arm_min}` }}
        onPointerDown={e => { if (e.button !== 0 || active.current) return; e.currentTarget.setPointerCapture(e.pointerId); active.current = [point(e)]; drawRef.current(); }}
        onPointerMove={e => { const p = point(e); setCursor(p); if (active.current) { const last = active.current.at(-1); if (Math.hypot(p[0] - last[0], p[1] - last[1]) >= 3) { active.current.push(p); drawRef.current(); } } }}
        onPointerUp={finish} onPointerCancel={finish} onLostPointerCapture={finish} onPointerLeave={() => setCursor(null)} />
      {!strokes.length && <div className="canvas-empty"><span className="empty-pencil"><Pencil size={25} strokeWidth={1.4} /></span><strong>O espaço é seu.</strong><p>Desenhe com o dedo ou o mouse.<br />Ou comece com um dos padrões abaixo.</p></div>}
      <div className="paper-caption"><span>{name || 'Sem título'}</span><span>{cursor ? `${cursor[0]} / ${cursor[1]}` : 'FEITO POR VOCÊ'}</span></div>
    </div>
    <div className="canvas-bottom"><span><span className="hidden sm:inline">Área visível · </span>{view}–{Math.round(view + span)} passos</span><div className="flex items-center gap-2"><IconButton icon={ChevronLeft} label="Girar área para a esquerda" disabled={view <= 0} onClick={() => setView(v => Math.max(0, v - 400))} /><span>Rotação do ovo</span><IconButton icon={ChevronRight} label="Girar área para a direita" disabled={view >= 3200} onClick={() => setView(v => Math.min(3200, v + 400))} /></div></div>
  </section>;
}
