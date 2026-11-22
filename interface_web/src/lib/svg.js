import { simplify } from './drawing';

// Recria apenas geometria conhecida: arquivos importados não entram como HTML ativo.
const tags = new Set(['svg', 'g', 'path', 'line', 'polyline', 'polygon', 'rect', 'circle', 'ellipse']);
const attrs = new Set(['viewBox', 'preserveAspectRatio', 'transform', 'd', 'points', 'x', 'y', 'x1', 'x2', 'y1', 'y2', 'width', 'height', 'rx', 'ry', 'cx', 'cy', 'r']);
function safeGeometry(node) {
  if (!tags.has(node.localName)) return null;
  const clean = document.createElementNS('http://www.w3.org/2000/svg', node.localName);
  for (const attr of node.attributes) if (attrs.has(attr.name)) clean.setAttribute(attr.name, attr.value);
  for (const child of node.children) { const result = safeGeometry(child); if (result) clean.appendChild(result); }
  return clean;
}
export function importSvg(text, config, width = 1600, view = 0) {
  if (text.length > 2_000_000) throw new Error('Use um SVG de até 2 MB.');
  const doc = new DOMParser().parseFromString(text, 'image/svg+xml');
  if (doc.querySelector('parsererror') || doc.documentElement.localName !== 'svg') throw new Error('Este arquivo não é um SVG válido.');
  const svg = safeGeometry(doc.documentElement), box = document.createElement('div');
  Object.assign(box.style, { position: 'absolute', left: '-10000px', top: '0', width: '800px', height: '800px', visibility: 'hidden', overflow: 'hidden' });
  svg.setAttribute('width', '800'); svg.setAttribute('height', '800');
  box.appendChild(svg); document.body.appendChild(box);
  try {
    const elements = [...svg.querySelectorAll('path,line,polyline,polygon,rect,circle,ellipse')];
    if (elements.length > 4000) throw new Error('O SVG tem muitos elementos. Simplifique o desenho antes de importar.');
    const lengths = elements.map(el => { try { return el.getTotalLength(); } catch { return 0; } });
    const total = lengths.reduce((a, b) => a + b, 0);
    if (!(total > 0) || !Number.isFinite(total)) throw new Error('Nenhum contorno encontrado. Converta textos em caminhos.');
    const step = Math.max(total / 12000, .5), raw = [];
    let lo = [Infinity, Infinity], hi = [-Infinity, -Infinity];
    elements.forEach((el, index) => {
      const length = lengths[index], matrix = el.getScreenCTM();
      if (!(length > 0) || !matrix) return;
      const count = Math.max(2, Math.ceil(length / step));
      let points = [], previous;
      for (let i = 0; i <= count; i++) {
        const p = el.getPointAtLength(length * i / count), q = new DOMPoint(p.x, p.y).matrixTransform(matrix);
        if (!Number.isFinite(q.x) || !Number.isFinite(q.y)) throw new Error('O SVG contém coordenadas inválidas.');
        if (previous && Math.hypot(q.x - previous.x, q.y - previous.y) > 3 * step * Math.max(Math.abs(matrix.a), Math.abs(matrix.d), 1e-9) + 1) {
          if (points.length > 1) raw.push(points);
          points = [];
        }
        points.push([q.x, q.y]); previous = q;
        lo = [Math.min(lo[0], q.x), Math.min(lo[1], q.y)]; hi = [Math.max(hi[0], q.x), Math.max(hi[1], q.y)];
      }
      if (points.length > 1) raw.push(points);
    });
    if (!raw.length) throw new Error('Nenhum contorno encontrado.');
    const w = Math.max(hi[0] - lo[0], 1e-9), h = Math.max(hi[1] - lo[1], 1e-9);
    const scale = Math.min(width / w, (config.arm_max - config.arm_min) * .95 / h);
    const r0 = width > 2000 ? view : view + 800 - w * scale / 2, center = (config.arm_min + config.arm_max) / 2, cy = (lo[1] + hi[1]) / 2;
    return raw.map(stroke => simplify(stroke.map(q => [Math.round(r0 + (q[0] - lo[0]) * scale), Math.round(center - (q[1] - cy) * scale)]), 1)).filter(s => s.length > 1);
  } finally { box.remove(); }
}
