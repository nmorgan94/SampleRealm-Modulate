import assert from 'node:assert/strict';
import { test } from 'node:test';
import { MAX_CURVE_POINTS } from '../src/backend.ts';
import { DEFAULT_POINTS, evaluate, SHAPES } from '../src/curves.ts';
import { engine, writePoints } from './engine.ts';

const drawn = [{ x: 0, y: 0.2 }, { x: 0.3, y: 0.9 }, { x: 0.65, y: 0.1 }, { x: 1, y: 0.2 }];
const sharedX = [{ x: 0, y: 0.2 }, { x: 0, y: 0.9 }, { x: 1, y: 0.2 }];

test('curves evaluate as LfoCurve::evaluate does, at any tension', () => {
  for (const points of [DEFAULT_POINTS, drawn, sharedX, ...SHAPES.map((shape) => shape.points)]) {
    writePoints(points);

    for (const tension of [0, 0.25, 0.5, 0.8, 1]) {
      for (let step = 0; step <= 100; step++) {
        const x = step / 100;
        const expected = engine.curveValue(points.length, tension, x);
        assert.ok(Math.abs(evaluate(points, tension, x) - expected) < 1e-5, `${points.length} points, tension ${tension}, x ${x}`);
      }
    }
  }
});

test('fewer than two points sit in the middle', () => {
  assert.equal(evaluate([], 0.5, 0.3), 0.5);
  assert.equal(evaluate([{ x: 0, y: 1 }], 0.5, 0.3), 0.5);
});

test('MAX_CURVE_POINTS is LfoCurve::maxPoints', () => {
  assert.equal(MAX_CURVE_POINTS, engine.maxCurvePoints());
});
