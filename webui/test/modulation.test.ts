import assert from 'node:assert/strict';
import { test } from 'node:test';
import { SOURCE_NUMBERS, type Modulation } from '../src/backend.ts';
import { applyDepthChange, makeDefaultRange, modulatedValue } from '../src/modulation.ts';
import { engine, floats, indexOf, param } from './engine.ts';

const near = (actual: number, expected: number, tolerance: number, message: string) =>
  assert.ok(Math.abs(actual - expected) <= tolerance, `${message}: ${actual}, expected ${expected}`);

test('dropping a source sets the range Modulation::makeDefaultRange does', () => {
  for (const source of ['lfo', 'env'] as const) {
    for (const base of [0, 0.1, 0.5, 0.9, 1]) {
      const [min, max] = floats(engine.defaultRange(SOURCE_NUMBERS[source], base), 2);
      const range = makeDefaultRange(source, 2, base);
      near(range.min, min!, 1e-6, `${source} min at ${base}`);
      near(range.max, max!, 1e-6, `${source} max at ${base}`);
      assert.equal(range.index, 2);
    }
  }
});

test('dragging the badge changes the depth as Modulation::applyDepthChange does', () => {
  for (const source of ['lfo', 'env'] as const) {
    for (const [min, max, base] of [[0.25, 0.75, 0.5], [0, 0.3, 0.1], [0.6, 1, 0.9], [0.4, 0.45, 0.4]]) {
      for (const delta of [-0.3, -0.01, 0, 0.02, 0.4]) {
        const [expectedMin, expectedMax] = floats(engine.depthChange(SOURCE_NUMBERS[source], min, max, base, delta), 2);
        const changed = applyDepthChange({ source, index: 0, min: min!, max: max! }, base!, delta);
        const label = `${source} ${min}-${max} at ${base} by ${delta}`;
        near(changed.min, expectedMin!, 1e-6, `${label} min`);
        near(changed.max, expectedMax!, 1e-6, `${label} max`);
      }
    }
  }
});

test('the live dot sits where the voices hear the parameter', () => {
  const modulation: Modulation = { source: 'lfo', index: 0, min: 0.2, max: 0.9 };

  for (const [id, value] of [['filter_cutoff', 1000], ['osc1_pitch', 7], ['lfo1_sync_rate', 3], ['glide_time', 0.3]] as const) {
    for (const source of [0, 0.37, 1]) {
      const expected = engine.modulatedValue(indexOf(id), value, modulation.min, modulation.max, source);
      const actual = modulatedValue(param(id), value, modulation, source);
      near(actual, expected, 1e-4 * Math.max(1, Math.abs(expected)), `${id} with the source at ${source}`);
    }

    const unmodulated = engine.modulatedValue(indexOf(id), value, 0, 0, 0);
    near(modulatedValue(param(id), value, null, 0.5), value, 1e-4 * Math.max(1, value), `${id} unmodulated`);
    assert.ok(Number.isFinite(unmodulated));
  }
});
