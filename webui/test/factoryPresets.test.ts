import assert from 'node:assert/strict';
import { readdirSync, readFileSync } from 'node:fs';
import { test } from 'node:test';
import { NUM_LFOS } from '../src/backend.ts';
import { parsePreset } from '../src/demo/localPresets.ts';
import { snap } from '../src/params.ts';
import { engine, param } from './engine.ts';

// Typos here would load silently as defaults
const DIR = new URL('../../Assets/Presets/', import.meta.url);
const presets = readdirSync(DIR)
  .filter((file) => file.endsWith('.modulate'))
  .map((file) => ({ file, ...parsePreset(readFileSync(new URL(file, DIR), 'utf8')) }));

test('there are factory presets, each named after its file and in a category', () => {
  assert.ok(presets.length > 0);
  for (const { file, name, category } of presets) {
    assert.equal(file, `${name}.modulate`);
    assert.ok(category, `${name} has no category`);
  }
});

test('every value is one its parameter can take', () => {
  for (const { name, params } of presets)
    for (const [id, value] of Object.entries(params)) assert.equal(snap(param(id), value), value, `${name}: ${id}`);
});

test('modulation targets a parameter from one of the four sources, within 0..1', () => {
  for (const { name, modulation } of presets)
    for (const [id, { index, min, max }] of Object.entries(modulation)) {
      param(id);
      assert.ok(index >= 0 && index < NUM_LFOS, `${name}: ${id} source ${index}`);
      assert.ok(min >= 0 && max <= 1, `${name}: ${id} range ${min}..${max}`);
    }
});

test('drawn curves run from 0 to 1 in x, within the points the engine keeps', () => {
  for (const { name, curves } of presets)
    for (const points of curves.filter((curve) => curve.length > 0)) {
      assert.ok(points.length <= engine.maxCurvePoints(), name);
      assert.equal(points[0]!.x, 0, name);
      assert.equal(points.at(-1)!.x, 1, name);
      assert.ok(points.every((point, i) => i === 0 || point.x >= points[i - 1]!.x), name);
      assert.ok(points.every(({ y }) => y >= 0 && y <= 1), name);
    }
});
