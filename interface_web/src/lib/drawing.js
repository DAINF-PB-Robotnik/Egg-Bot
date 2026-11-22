export const ROTATION = 3200;
export const DEFAULT_CONFIG = { pen_up: 60, pen_down: 40, pen_ms: 500, speed_down: 300, speed_up: 800, arm_min: -400, arm_max: 400 };
export const PATTERNS = [
  { id: 'flora', name: 'Flora', caption: 'Um toque orgânico' },
  { id: 'star', name: 'Estrela', caption: 'Geometria em pontas' },
  { id: 'circle', name: 'Círculo', caption: 'Simples e contínuo' },
  { id: 'square', name: 'Quadrado', caption: 'Linhas precisas' },
  { id: 'sine', name: 'Onda', caption: 'Uma volta de leveza' },
  { id: 'multi', name: 'Ondas', caption: 'Ritmo em dobro' },
  { id: 'helix', name: 'Espiral', caption: 'Duas voltas no ovo' },
];

// CRC e formato compatíveis com EGGJOB 1 do firmware existente.
const crcTable = Array.from({ length: 256 }, (_, index) => {
  let c = index;
  for (let i = 0; i < 8; i++) c = c & 1 ? 0xEDB88320 ^ (c >>> 1) : c >>> 1;
  return c >>> 0;
});
export function crc32(text) {
  let c = 0xFFFFFFFF;
  for (const b of new TextEncoder().encode(text)) c = crcTable[(c ^ b) & 255] ^ (c >>> 8);
  return ((c ^ 0xFFFFFFFF) >>> 0).toString(16).toUpperCase().padStart(8, '0');
}

// Algoritmo da interface original, preservado nesta migração visual.
export function simplify(points, epsilon = 2) {
  if (points.length < 3) return points.slice();
  const keep = new Uint8Array(points.length), stack = [[0, points.length - 1]];
  keep[0] = keep[points.length - 1] = 1;
  while (stack.length) {
    const [a, b] = stack.pop(), A = points[a], B = points[b];
    const dx = B[0] - A[0], dy = B[1] - A[1], length = Math.hypot(dx, dy);
    let max = 0, index = -1;
    for (let i = a + 1; i < b; i++) {
      const distance = length < 1e-6 ? Math.hypot(points[i][0] - A[0], points[i][1] - A[1]) : Math.abs(dy * (points[i][0] - A[0]) - dx * (points[i][1] - A[1])) / length;
      if (distance > max) { max = distance; index = i; }
    }
    if (max > epsilon) { keep[index] = 1; stack.push([a, index], [index, b]); }
  }
  return points.filter((_, i) => keep[i]);
}

export function buildJob(name, strokes) {
  const body = [];
  for (const stroke of strokes) {
    if (!stroke.length) continue;
    body.push('U', `L ${stroke[0][0]} ${stroke[0][1]}`, 'D');
    for (const [r, a] of stroke.slice(1)) body.push(`L ${r} ${a}`);
  }
  if (!body.length) return '';
  body.push('U');
  const safeName = name.normalize('NFD').replace(/[\u0300-\u036f]/g, '').replace(/[^a-zA-Z0-9_-]/g, '_').slice(0, 24) || 'desenho';
  const text = `EGGJOB 1\nN ${safeName}\nP ${body.length}\n${body.join('\n')}\n`;
  return `${text}END ${crc32(text)}\n`;
}

export function prepareDrawing(strokes, config, name) {
  let clipped = 0;
  const simplified = strokes.map(stroke => simplify(stroke.map(([r, a]) => {
    const point = [Math.round(Math.max(-6400, Math.min(6400, r))), Math.round(Math.max(config.arm_min, Math.min(config.arm_max, a)))];
    if (point[0] !== r || point[1] !== a) clipped++;
    return point;
  })));
  const file = buildJob(name, simplified), bytes = new TextEncoder().encode(file).length;
  const lines = file ? Number(file.split('\n')[2].slice(2)) : 0;
  return { file, strokes: simplified, points: simplified.reduce((n, s) => n + s.length, 0), bytes, lines, clipped, tooLarge: bytes > 65536 || lines > 8000 };
}

export function parseJob(text) {
  const points = [[0, 0, false]];
  let pen = false;
  for (const line of text.split('\n')) {
    if (line === 'U') pen = false;
    else if (line === 'D') pen = true;
    else if (/^L -?\d+ -?\d+$/.test(line)) {
      const [, r, a] = line.split(' ').map(Number);
      points.push([r, a, pen]);
    }
  }
  return points;
}

export function pattern(id, config = DEFAULT_CONFIG, view = 0) {
  const center = (config.arm_min + config.arm_max) / 2, half = (config.arm_max - config.arm_min) / 2, cx = view + 800;
  const points = [], add = (r, a) => points.push([Math.round(r), Math.round(a)]);
  if (id === 'flora') {
    // Roseta desenhada em um único traço; exemplo visual adicional.
    for (let i = 0; i <= 720; i++) {
      const t = i / 720 * Math.PI * 2, radius = half * (.55 + .28 * Math.cos(7 * t));
      add(cx + radius * Math.cos(t), center + radius * Math.sin(t));
    }
    return points;
  }
  if (id === 'sine' || id === 'multi') {
    const n = id === 'sine' ? 64 : 128;
    for (let i = 0; i < n; i++) { const t = i / (n - 1); add(view + t * ROTATION, center + .8 * half * Math.sin(2 * Math.PI * t * (id === 'sine' ? 1 : 2))); }
  } else if (id === 'helix') {
    for (let i = 0; i < 256; i++) { const t = i / 255; add(t * 2 * ROTATION, center + .8 * half * (2 * t - 1)); }
  } else if (id === 'circle') {
    for (let i = 0; i <= 128; i++) { const t = 2 * Math.PI * i / 128; add(cx + .8 * half * Math.cos(t), center + .8 * half * Math.sin(t)); }
  } else if (id === 'square') {
    for (const [x, y] of [[-1, -1], [1, -1], [1, 1], [-1, 1], [-1, -1]]) add(cx + .7 * half * x, center + .7 * half * y);
  } else {
    for (let i = 0; i <= 10; i++) { const radius = .85 * half * (i % 2 ? .375 : 1), t = Math.PI / 2 + i * Math.PI / 5; add(cx + radius * Math.cos(t), center + radius * Math.sin(t)); }
  }
  return points;
}

export function estimate(strokes, config = DEFAULT_CONFIG) {
  let seconds = 0, r = 0, a = 0;
  for (const stroke of strokes) {
    if (!stroke.length) continue;
    seconds += Math.hypot(stroke[0][0] - r, stroke[0][1] - a) / config.speed_up + 2 * config.pen_ms / 1000;
    [r, a] = stroke[0];
    for (const point of stroke.slice(1)) { seconds += Math.hypot(point[0] - r, point[1] - a) / config.speed_down; [r, a] = point; }
  }
  return Math.ceil(seconds + Math.hypot(r, a) / config.speed_up);
}
export function formatTime(seconds = 0) {
  const s = Math.max(0, Math.round(seconds));
  return s >= 3600 ? `${Math.floor(s / 3600)} h ${Math.floor(s % 3600 / 60)} min` : s >= 60 ? `${Math.floor(s / 60)} min ${s % 60} s` : `${s} s`;
}
export function download(text, filename) {
  const url = URL.createObjectURL(new Blob([text], { type: 'text/plain;charset=utf-8' }));
  const a = document.createElement('a'); a.href = url; a.download = filename; a.click();
  setTimeout(() => URL.revokeObjectURL(url), 1000);
}
