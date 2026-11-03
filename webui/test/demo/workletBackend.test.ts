import assert from 'node:assert/strict';
import { test } from 'node:test';
import { INIT_PRESET, MAX_CURVE_POINTS, type Level, type Live } from '../../src/backend.ts';
import { STORAGE_KEY, workletBackend, type EnginePort, type PresetStorage } from '../../src/demo/workletBackend.ts';
import { indexOf, info } from '../engine.ts';

const fakePort = () => {
  const sent: { type: string }[] = [];
  const port: EnginePort = { postMessage: (message) => sent.push(message), onmessage: null };
  const receive = (data: unknown) => port.onmessage?.({ data } as MessageEvent);
  return { port, sent, receive };
};

const memoryStorage = (): PresetStorage & { items: Map<string, string> } => {
  const items = new Map<string, string>();
  return { items, getItem: (key) => items.get(key) ?? null, setItem: (key, value) => void items.set(key, value) };
};

test('changes made before the worklet starts reach it in order, after the demo tempo', () => {
  const backend = workletBackend(info, memoryStorage());
  backend.setValue('filter_cutoff', 2500);
  backend.setModulation('filter_cutoff', { source: 'env', index: 1, min: 0.2, max: 0.8 });
  backend.setCurve(2, [{ x: 0, y: 0.1 }, { x: 1, y: 0.1 }]);
  backend.noteOn(60);

  const { port, sent } = fakePort();
  backend.attach(port);

  assert.deepEqual(sent, [
    { type: 'tempo', bpm: 120 },
    { type: 'param', index: indexOf('filter_cutoff'), value: 2500 },
    { type: 'modulation', index: indexOf('filter_cutoff'), source: 2, sourceIndex: 1, min: 0.2, max: 0.8 },
    { type: 'curve', lfo: 2, points: [0, 0.1, 1, 0.1] },
    { type: 'midi', bytes: [0x90, 60, 100] },
  ]);

  backend.setModulation('filter_cutoff', null);
  assert.deepEqual(sent.at(-1), { type: 'modulation', index: indexOf('filter_cutoff'), source: 0, sourceIndex: 0, min: 0, max: 0 });
});

test("curves are cut to what the engine's points() holds", () => {
  const backend = workletBackend(info, memoryStorage());
  const { port, sent } = fakePort();
  backend.attach(port);

  const long = Array.from({ length: MAX_CURVE_POINTS + 8 }, (_, i) => ({ x: i / (MAX_CURVE_POINTS + 7), y: 0.5 }));
  const kept = long.slice(0, MAX_CURVE_POINTS);
  backend.setCurve(0, long);

  assert.deepEqual(backend.getCurve(0), kept);
  assert.deepEqual(sent.at(-1), { type: 'curve', lfo: 0, points: kept.flatMap(({ x, y }) => [x, y]) });
});

test('a preset brings back parameters, modulation and curves, and Init clears them', async () => {
  const storage = memoryStorage();
  const backend = workletBackend(info, storage);
  const modulation = { source: 'lfo', index: 2, min: 0.1, max: 0.6 } as const;
  const curve = [{ x: 0, y: 0.3 }, { x: 0.5, y: 0.9 }, { x: 1, y: 0.3 }];

  backend.setValue('osc2_enable', 1);
  backend.setValue('master_gain', -6);
  backend.setModulation('osc1_morph', modulation);
  backend.setCurve(1, curve);
  assert.equal(await backend.presets.save('Bright'), true);
  assert.ok(storage.items.has(STORAGE_KEY));

  let modulationChanges = 0;
  backend.onModulation(() => modulationChanges++);

  assert.equal(await backend.presets.load(INIT_PRESET), true);
  assert.equal(backend.getValue('osc2_enable'), 0);
  assert.equal(backend.getValue('master_gain'), 0);
  assert.equal(backend.getModulation('osc1_morph'), null);
  assert.deepEqual(backend.getCurve(1), []);
  assert.ok(modulationChanges > 0);
  assert.deepEqual(await backend.presets.list(), { names: ['Bright'], current: '' });

  assert.equal(await backend.presets.load('Bright'), true);
  assert.equal(backend.getValue('osc2_enable'), 1);
  assert.equal(backend.getValue('master_gain'), -6);
  assert.deepEqual(backend.getModulation('osc1_morph'), modulation);
  assert.deepEqual(backend.getCurve(1), curve);
  assert.deepEqual(await backend.presets.list(), { names: ['Bright'], current: 'Bright' });
});

test('removing the current preset leaves none current, and missing presets fail to load', async () => {
  const backend = workletBackend(info, memoryStorage());
  await backend.presets.save('Gone');
  assert.equal(await backend.presets.remove('Gone'), true);

  assert.deepEqual(await backend.presets.list(), { names: [], current: '' });
  assert.equal(await backend.presets.load('Gone'), false);
  assert.equal(await backend.presets.save(INIT_PRESET), false);
});

test('without storage, saving says it failed', async () => {
  const backend = workletBackend(info, undefined);
  assert.equal(await backend.presets.save('Nowhere'), false);
  assert.deepEqual(await backend.presets.list(), { names: [], current: '' });
});

test("the worklet's levels and live state reach their listeners", () => {
  const backend = workletBackend(info, memoryStorage());
  const levels: Level[] = [];
  const lives: Live[] = [];
  backend.onLevel((level) => levels.push(level));
  backend.onLive((live) => lives.push(live));

  const { port, receive } = fakePort();
  backend.attach(port);

  const live: Live = { lfoPhase: [0.5, 0, 0, 0], lfoValue: [1, 0, 0, 0], envelopeLevel: [0.7, 0, 0, 0], envelopeProgress: [0, 0, 0, 0], envelopeStage: [3, 0, 0, 0] };
  receive({ type: 'level', peaks: [0.25, 0.5] });
  receive({ type: 'live', live });

  assert.deepEqual(levels, [{ peaks: [0.25, 0.5] }]);
  assert.deepEqual(lives, [live]);
});
