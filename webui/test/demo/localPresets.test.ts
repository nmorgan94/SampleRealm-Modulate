// Modulate's preset state in the demo. The shared rules are SampleRealm-Modules' Tests/web/localPresets.test.ts
import assert from 'node:assert/strict';
import { test } from 'node:test';
import { parsePreset } from '@samplerealm/modules/demo/localPresets.ts';
import { memoryStorage, presetXml } from '@samplerealm/modules/testing/presets.ts';
import { parsePresetState, STORAGE_KEY } from '../../src/demo/presetState.ts';
import { workletBackend } from '../../src/demo/workletBackend.ts';
import { info } from '../engine.ts';

// Fixed factory presets, as in Tests/PresetTests.cpp
const factory = [
  `<?xml version="1.0" encoding="UTF-8"?>
<PRESET name="Wobble" category="Bass" author="SampleRealm" formatVersion="1">
  <Parameters>
    <PARAM id="filter_cutoff" value="400.0"/>
    <ModMatrix>
      <Assignment parameterID="filter_cutoff" sourceType="1" sourceIndex="2" minValue="0.2" maxValue="0.8"/>
    </ModMatrix>
    <LFOCurves>
      <LFOCurve index="2">
        <Point x="0.0" y="0.0"/>
        <Point x="0.5" y="1.0"/>
        <Point x="1.0" y="0.0"/>
      </LFOCurve>
    </LFOCurves>
  </Parameters>
</PRESET>`,
  presetXml('Sub', 'Bass', { filter_cutoff: 150 }),
  presetXml('Bright', 'Lead', { filter_cutoff: 8000 }),
];

const FACTORY_NAMES = ['Sub', 'Wobble', 'Bright'];

const setUp = (storage = memoryStorage()) => {
  const backend = workletBackend(info, { factoryPresets: factory, storage });
  return { backend, presets: backend.presets, storage, cutoff: () => backend.getValue('filter_cutoff') };
};

const named = <T extends { name: string }>(list: T[], name: string) => {
  const found = list.find((preset) => preset.name === name);
  assert.ok(found, `No preset ${name}`);
  return found;
};

test('factory files are read with their category, values, modulation and curves', () => {
  assert.deepEqual(parsePreset(factory[0]!, parsePresetState), {
    name: 'Wobble',
    category: 'Bass',
    isFactory: true,
    params: { filter_cutoff: 400 },
    modulation: { filter_cutoff: { source: 'lfo', index: 2, min: 0.2, max: 0.8 } },
    curves: [[], [], [{ x: 0, y: 0 }, { x: 0.5, y: 1 }, { x: 1, y: 0 }], []],
  });
});

test('a loaded preset sets its values, modulation and curves, and shows its name', async () => {
  const { backend, presets, cutoff } = setUp();
  assert.equal(presets.status().current.name, 'Init');

  await presets.load(named(await presets.list(), 'Wobble'));

  assert.equal(cutoff(), 400);
  assert.deepEqual(backend.getModulation('filter_cutoff'), { source: 'lfo', index: 2, min: 0.2, max: 0.8 });
  assert.equal(backend.getCurve(2).length, 3);
  assert.deepEqual(presets.status(), { current: { name: 'Wobble', category: 'Bass', isFactory: true }, modified: false });
});

test('a preset without modulation or curves clears them', async () => {
  const { backend, presets } = setUp();
  await presets.load(named(await presets.list(), 'Wobble'));
  await presets.load(named(await presets.list(), 'Sub'));

  assert.equal(backend.getModulation('filter_cutoff'), null);
  assert.deepEqual(backend.getCurve(2), []);
});

test('changing modulation or a curve marks the preset modified', async () => {
  const { backend, presets } = setUp();

  await presets.loadInit();
  backend.setModulation('filter_cutoff', { source: 'env', index: 1, min: 0.1, max: 0.9 });
  assert.equal(presets.status().modified, true);

  await presets.loadInit();
  backend.setCurve(0, [{ x: 0, y: 0 }, { x: 1, y: 1 }]);
  assert.equal(presets.status().modified, true);
});

test('Init restores the defaults and clears modulation and curves', async () => {
  const { backend, presets, cutoff } = setUp();
  await presets.load(named(await presets.list(), 'Wobble'));
  backend.setModulation('osc1_morph', { source: 'env', index: 1, min: 0.1, max: 0.9 });

  await presets.loadInit();

  assert.equal(cutoff(), backend.param('filter_cutoff').defaultValue);
  assert.equal(backend.getModulation('filter_cutoff'), null);
  assert.equal(backend.getModulation('osc1_morph'), null);
  assert.deepEqual(backend.getCurve(2), []);
  assert.deepEqual(presets.status(), { current: { name: 'Init', category: '', isFactory: true }, modified: false });
});

test('user presets keep their values, modulation and curves past the page', async () => {
  const { backend, presets, storage } = setUp();
  backend.setValue('filter_cutoff', 3000);
  backend.setModulation('osc1_morph', { source: 'lfo', index: 1, min: 0.2, max: 0.7 });
  backend.setCurve(1, [{ x: 0, y: 0.3 }, { x: 1, y: 0.3 }]);

  assert.equal(await presets.save('Mine', 'Lead'), null);
  assert.deepEqual((await presets.list()).map((preset) => preset.name), [...FACTORY_NAMES, 'Mine']);

  // A new page reads it back
  const reopened = setUp(storage);
  await reopened.presets.load(named(await reopened.presets.list(), 'Mine'));
  assert.equal(reopened.cutoff(), 3000);
  assert.deepEqual(reopened.backend.getModulation('osc1_morph'), { source: 'lfo', index: 1, min: 0.2, max: 0.7 });
  assert.deepEqual(reopened.backend.getCurve(1), [{ x: 0, y: 0.3 }, { x: 1, y: 0.3 }]);
});

test('an LFO with no points keeps the next LFO its own curve', () => {
  const xml = `<PRESET name="Empty" category="Bass"><Parameters>
    <LFOCurves>
      <LFOCurve index="0"/>
      <LFOCurve index="1"><Point x="0.0" y="0.2"/><Point x="1.0" y="0.8"/></LFOCurve>
    </LFOCurves>
  </Parameters></PRESET>`;

  assert.deepEqual(parsePresetState(xml).curves, [[], [{ x: 0, y: 0.2 }, { x: 1, y: 0.8 }], [], []]);
});

test('a value that is missing or not a number loads at its default', () => {
  const xml = '<PRESET name="Bad"><Parameters><PARAM id="filter_cutoff"/><PARAM id="osc1_gain" value=""/><PARAM id="osc2_gain" value="loud"/></Parameters></PRESET>';

  assert.deepEqual(parsePresetState(xml).params, {});
});

test('a stored preset loads its values, modulation and curves, skipping ids this engine lacks', async () => {
  const storage = memoryStorage({
    [STORAGE_KEY]: JSON.stringify([{
      name: 'Old',
      category: 'Lead',
      params: { gone: 1, filter_cutoff: 900 },
      modulation: { osc1_morph: { source: 'env', index: 0, min: 0.1, max: 0.5 } },
      curves: [[{ x: 0, y: 1 }, { x: 1, y: 0 }]],
    }]),
  });
  const { backend, presets, cutoff } = setUp(storage);

  await presets.load(named(await presets.list(), 'Old'));

  // An id this engine doesn't have is skipped, as the APVTS skips it
  assert.equal(cutoff(), 900);
  assert.deepEqual(backend.getModulation('osc1_morph'), { source: 'env', index: 0, min: 0.1, max: 0.5 });
  assert.deepEqual(backend.getCurve(0), [{ x: 0, y: 1 }, { x: 1, y: 0 }]);
  assert.deepEqual(presets.status(), { current: { name: 'Old', category: 'Lead', isFactory: false }, modified: false });
});
