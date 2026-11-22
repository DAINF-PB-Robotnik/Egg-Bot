import { useEffect, useMemo, useRef, useState } from 'react';
import { RotateCcw, MoveHorizontal, Scan } from 'lucide-react';
import { parseJob, ROTATION } from '../lib/drawing';

export default function EggPreview({ file, source = 'Seu desenho na superfície', large = false }) {
  const canvas = useRef(), drag = useRef(null), [rotation, setRotation] = useState(800), [travel, setTravel] = useState(false);
  const points = useMemo(() => parseJob(file), [file]);
  const center = () => {
    const ink = points.filter(p => p[2]);
    setRotation(ink.length ? ((Math.min(...ink.map(p => p[0])) + Math.max(...ink.map(p => p[0]))) / 2 % ROTATION + ROTATION) % ROTATION : 800);
  };
  useEffect(() => { center(); }, [file]);
  const draw = useRef();
  draw.current = () => {
    const el = canvas.current;
    if (!el) return;
    const width = el.clientWidth, height = el.clientHeight, d = window.devicePixelRatio || 1;
    el.width = width * d; el.height = height * d;
    const ctx = el.getContext('2d'); ctx.scale(d, d); ctx.clearRect(0, 0, width, height);
    const cx = width / 2, cy = height * .46, scale = Math.min(height * .36, width * .40);
    const profile = t => (.72 + .08 * Math.cos(t)) * Math.sin(t);
    const shadow = ctx.createRadialGradient(cx, cy + scale * 1.14, 1, cx, cy + scale * 1.14, scale * .85);
    shadow.addColorStop(0, '#00000065'); shadow.addColorStop(1, '#00000000');
    ctx.save(); ctx.translate(0, (cy + scale * 1.14) * .78); ctx.scale(1, .22); ctx.fillStyle = shadow; ctx.fillRect(0, 0, width, height * 5); ctx.restore();
    ctx.beginPath();
    for (let i = 0; i <= 100; i++) { const t = Math.PI * i / 100; ctx.lineTo(cx + profile(t) * scale, cy - Math.cos(t) * scale); }
    for (let i = 100; i >= 0; i--) { const t = Math.PI * i / 100; ctx.lineTo(cx - profile(t) * scale, cy - Math.cos(t) * scale); }
    ctx.closePath();
    const fill = ctx.createRadialGradient(cx - scale * .35, cy - scale * .5, scale * .05, cx + scale * .12, cy - scale * .15, scale * 1.4);
    fill.addColorStop(0, '#fffdf5'); fill.addColorStop(.4, '#f2ead9'); fill.addColorStop(.7, '#cec6b5'); fill.addColorStop(1, '#898a79');
    ctx.fillStyle = fill; ctx.fill();
    ctx.save(); ctx.clip();
    // Fine deterministic texture, rendered locally without image assets.
    for (let i = 0; i < 1800; i++) { const x = ((i * 113.7) % (scale * 2)) + cx - scale, y = ((i * 79.3) % (scale * 2)) + cy - scale; ctx.fillStyle = i % 2 ? '#867e7310' : '#ffffff19'; ctx.fillRect(x, y, .7, .7); }
    let previous;
    ctx.lineCap = ctx.lineJoin = 'round';
    for (const [r, a, pen] of points) {
      const t = Math.PI / 2 - a / ROTATION * 2 * Math.PI, theta = (r - rotation) / ROTATION * 2 * Math.PI;
      const p = { x: cx + profile(t) * Math.sin(theta) * scale, y: cy - Math.cos(t) * scale, z: Math.cos(theta) };
      if (previous && previous.z > 0 && p.z > 0 && (pen || travel)) {
        ctx.globalAlpha = .3 + .7 * p.z; ctx.strokeStyle = pen ? '#315844' : '#a67936'; ctx.lineWidth = pen ? 1.6 : .8; ctx.setLineDash(pen ? [] : [3, 4]);
        ctx.beginPath(); ctx.moveTo(previous.x, previous.y); ctx.lineTo(p.x, p.y); ctx.stroke();
      }
      previous = p;
    }
    ctx.restore(); ctx.globalAlpha = 1; ctx.setLineDash([]);
    ctx.strokeStyle = '#c7e2cc22'; ctx.lineWidth = 1; ctx.beginPath(); ctx.ellipse(cx, cy + scale * 1.3, scale * 1.03, scale * .13, 0, 0, Math.PI * 2); ctx.stroke();
  };
  useEffect(() => { draw.current(); }, [points, rotation, travel]);
  useEffect(() => { const observer = new ResizeObserver(() => draw.current()); observer.observe(canvas.current); return () => observer.disconnect(); }, []);
  return <section className={`egg-panel ${large ? 'egg-large' : ''}`}>
    <div className="flex items-start justify-between px-6 pt-6"><div><div className="eyebrow text-[#a7b7ac]">UMA NOVA PERSPECTIVA</div><h2 className="mt-2 text-lg font-semibold text-white">Prévia no ovo</h2></div><span className="preview-label"><Scan size={13} /> AO VIVO</span></div>
    <canvas ref={canvas} aria-label="Prévia do desenho no ovo; arraste para girar" onPointerDown={e => { e.currentTarget.setPointerCapture(e.pointerId); drag.current = { x: e.clientX, r: rotation }; }} onPointerMove={e => { if (drag.current) setRotation(((drag.current.r - (e.clientX - drag.current.x) * 6) % ROTATION + ROTATION) % ROTATION); }} onPointerUp={() => { drag.current = null; }} onPointerCancel={() => { drag.current = null; }} />
    <div className="preview-drag"><MoveHorizontal size={15} /> Arraste para explorar</div>
    <div className="px-6 pb-6"><div className="mb-2 flex items-center justify-between text-xs text-[#aebdb4]"><span>Rotação</span><span className="font-mono text-[#e2ecdf]">{Math.round(rotation / ROTATION * 360)}°</span></div><input aria-label="Rotação da prévia" className="egg-range" type="range" min="0" max="3199" value={Math.round(rotation)} onChange={e => setRotation(+e.target.value)} /><div className="mt-4 flex items-center justify-between gap-2 text-xs text-[#b1c0b7]"><label className="flex items-center gap-2"><input type="checkbox" checked={travel} onChange={e => setTravel(e.target.checked)} /> Deslocamentos</label><button className="flex items-center gap-1.5 hover:text-white" onClick={center}><RotateCcw size={13} /> Centralizar</button></div><div className="mt-5 border-t border-white/10 pt-4 text-xs text-[#a7b7ac]">{source}</div></div>
  </section>;
}
