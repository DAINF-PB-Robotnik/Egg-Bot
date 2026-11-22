import test from 'node:test';
import assert from 'node:assert/strict';
import { buildJob, crc32, parseJob, prepareDrawing, DEFAULT_CONFIG, PATTERNS, pattern } from '../src/lib/drawing.js';

test('CRC corresponde ao vetor padrão e ao arquivo aceito na revisão', () => {
  assert.equal(crc32('123456789'), 'CBF43926');
  assert.equal(buildJob('teste', [[[100, 50], [200, 100]], [[300, 0], [400, 0]]]), 'EGGJOB 1\nN teste\nP 9\nU\nL 100 50\nD\nL 200 100\nU\nL 300 0\nD\nL 400 0\nU\nEND 7F0C0FC0\n');
});
test('traços distintos preservam as viagens com a caneta levantada', () => {
  const points = parseJob(buildJob('teste', [[[100, 50], [200, 100]], [[300, 0], [400, 0]]]));
  assert.deepEqual(points, [[0, 0, false], [100, 50, false], [200, 100, true], [300, 0, false], [400, 0, true]]);
});
test('nome e coordenadas respeitam o contrato do firmware', () => {
  const p = prepareDrawing([[[-7000.5, -900], [7000.5, 900]]], DEFAULT_CONFIG, 'Olá <ovo>!');
  assert.match(p.file, /^EGGJOB 1\nN Ola__ovo__\n/);
  assert.equal(p.clipped, 2);
  assert.deepEqual(p.strokes, [[[-6400, -400], [6400, 400]]]);
  assert.equal(prepareDrawing([], DEFAULT_CONFIG, 'vazio').file, '');
});
test('desenho excedente é sinalizado antes do envio', () => {
  const manyStrokes = Array.from({ length: 2700 }, () => [[0, 0]]);
  assert.equal(prepareDrawing(manyStrokes, DEFAULT_CONFIG, 'grande').tooLarge, true);
});
test('padrões geram pontos inteiros dentro dos limites declarados', () => {
  for (const { id } of PATTERNS) {
    const points = pattern(id);
    assert.ok(points.length > 1, id);
    assert.ok(points.every(([r, a]) => Number.isInteger(r) && Number.isInteger(a) && Math.abs(r) <= 6400 && a >= -400 && a <= 400), id);
  }
});
