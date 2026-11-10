import assert from 'node:assert/strict';
import { test } from 'node:test';
import { draggedCurve, envelopeShape, shapeProgress } from '../src/envelope.ts';
import { engine } from './engine.ts';

test('segments bend as EnvelopeCurve::shapeProgress does, at any curve', () => {
  for (const curve of [-1, -0.5, -0.0005, 0, 0.0005, 0.3, 1]) {
    for (let step = -2; step <= 102; step++) {
      const progress = step / 100;
      const expected = engine.envelopeCurve(progress, curve);
      assert.ok(Math.abs(shapeProgress(progress, curve) - expected) < 1e-5, `curve ${curve}, progress ${progress}`);
    }
  }
});

test('a curve of 0 is a straight line, and every curve keeps its ends', () => {
  for (const progress of [0.1, 0.5, 0.9]) assert.equal(shapeProgress(progress, 0), progress);

  for (const curve of [-1, 0.4, 1]) {
    assert.equal(shapeProgress(0, curve), 0);
    assert.equal(shapeProgress(1, curve), 1);
  }
});

test('a handle follows the pointer it is dragged with', () => {
  const handles = (curves: [number, number, number]) => envelopeShape({ attack: 0.5, decay: 0.5, sustain: 0.5, release: 0.5, curves }, 300, 100).handles;

  handles([0, 0, 0]).forEach((handle, i) => {
    for (const dy of [-30, 30]) {
      const curves: [number, number, number] = [0, 0, 0];
      curves[i] = draggedCurve(i, 0, dy);
      const moved = handles(curves)[i]!.y - handle.y;
      assert.equal(Math.sign(moved), Math.sign(dy), `segment ${i}, dy ${dy}`);
    }
  });
});

test('the sustain is flat at its level, whatever the release curve', () => {
  const { decayX, sustainX, sustainY, path } = envelopeShape({ attack: 0.5, decay: 0.5, sustain: 0.7, release: 0.5, curves: [0, 0, 1] }, 300, 100);
  const points = [...path.matchAll(/[ML] (\S+) (\S+)/g)].map(([, x, y]) => ({ x: Number(x), y: Number(y) }));
  const middle = (decayX + sustainX) / 2;
  const after = points.findIndex(({ x }) => x > middle);
  const [p, q] = [points[after - 1]!, points[after]!];

  assert.ok(Math.abs(p.y + ((q.y - p.y) * (middle - p.x)) / (q.x - p.x) - sustainY) < 1e-9);
});
