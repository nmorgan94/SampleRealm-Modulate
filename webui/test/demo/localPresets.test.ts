import assert from 'node:assert/strict';
import { test } from 'node:test';
import { parsePreset, STORAGE_KEY, type PresetStorage } from '../../src/demo/localPresets.ts';
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
  '<PRESET name="Sub" category="Bass"><Parameters><PARAM id="filter_cutoff" value="150"/></Parameters></PRESET>',
  '<PRESET name="Bright" category="Lead"><Parameters><PARAM id="filter_cutoff" value="8000"/></Parameters></PRESET>',
].map(parsePreset);

const FACTORY_NAMES = ['Sub', 'Wobble', 'Bright'];

const fakeStorage = (): PresetStorage & { items: Map<string, string> } => {
  const items = new Map<string, string>();
  return { items, getItem: (key) => items.get(key) ?? null, setItem: (key, value) => void items.set(key, value) };
};

const setUp = (storage = fakeStorage()) => {
  const backend = workletBackend(info, factory, storage);
  return { backend, presets: backend.presets, storage, cutoff: () => backend.getValue('filter_cutoff') };
};

const named = <T extends { name: string }>(list: T[], name: string) => {
  const found = list.find((preset) => preset.name === name);
  assert.ok(found, `No preset ${name}`);
  return found;
};

test('factory files are read with their category, values, modulation and curves', () => {
  assert.deepEqual(named(factory, 'Wobble'), {
    name: 'Wobble',
    category: 'Bass',
    isFactory: true,
    params: { filter_cutoff: 400 },
    modulation: { filter_cutoff: { source: 'lfo', index: 2, min: 0.2, max: 0.8 } },
    curves: [[], [], [{ x: 0, y: 0 }, { x: 0.5, y: 1 }, { x: 1, y: 0 }], []],
  });
});

test('names keep the characters XmlElement writes by number', () => {
  assert.equal(parsePreset('<PRESET name="Caf&#233; &amp; Bar" category="Keys"/>').name, 'Café & Bar');
});

test('factory presets are listed by category, then name', async () => {
  const { presets } = setUp();
  assert.deepEqual((await presets.list()).map((preset) => preset.name), FACTORY_NAMES);
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

test('moving a parameter marks the preset modified until the next load', async () => {
  const { backend, presets, cutoff } = setUp();
  const bright = named(await presets.list(), 'Bright');
  await presets.load(bright);

  const seen: boolean[] = [];
  presets.onStatus((status) => seen.push(status.modified));

  backend.setValue('filter_cutoff', 1000);
  assert.equal(presets.status().modified, true);

  await presets.load(bright);
  assert.equal(presets.status().modified, false);
  assert.equal(cutoff(), 8000);
  assert.deepEqual(seen, [true, false]);
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

test('next and previous wrap, and start from either end after Init', async () => {
  const { presets, cutoff } = setUp();
  const name = () => presets.status().current.name;

  await presets.loadNext();
  assert.equal(name(), FACTORY_NAMES[0]);

  await presets.loadPrevious();
  assert.equal(name(), FACTORY_NAMES.at(-1));

  await presets.loadNext();
  assert.equal(name(), FACTORY_NAMES[0]);

  await presets.loadInit();
  await presets.loadPrevious();
  assert.equal(name(), FACTORY_NAMES.at(-1));
  assert.equal(cutoff(), 8000);
});

test('user presets save, rename and delete, and outlast the page', async () => {
  const { backend, presets, storage } = setUp();
  backend.setValue('filter_cutoff', 3000);
  backend.setModulation('osc1_morph', { source: 'lfo', index: 1, min: 0.2, max: 0.7 });
  backend.setCurve(1, [{ x: 0, y: 0.3 }, { x: 1, y: 0.3 }]);

  assert.equal(await presets.save('Mine', 'Lead'), null);
  assert.deepEqual(presets.status(), { current: { name: 'Mine', category: 'Lead', isFactory: false }, modified: false });
  assert.equal(await presets.exists('mine'), true);
  assert.deepEqual((await presets.list()).map((preset) => preset.name), [...FACTORY_NAMES, 'Mine']);

  // A new page reads it back
  const reopened = setUp(storage);
  await reopened.presets.load(named(await reopened.presets.list(), 'Mine'));
  assert.equal(reopened.cutoff(), 3000);
  assert.deepEqual(reopened.backend.getModulation('osc1_morph'), { source: 'lfo', index: 1, min: 0.2, max: 0.7 });
  assert.deepEqual(reopened.backend.getCurve(1), [{ x: 0, y: 0.3 }, { x: 1, y: 0.3 }]);

  assert.equal(await presets.rename(named(await presets.list(), 'Mine'), 'Yours'), null);
  assert.equal(presets.status().current.name, 'Yours');
  assert.deepEqual((await presets.list()).filter((preset) => !preset.isFactory).map((preset) => preset.name), ['Yours']);

  assert.equal(await presets.remove(named(await presets.list(), 'Yours')), null);
  assert.equal(await presets.exists('Yours'), false);
  assert.equal(presets.status().modified, true);
  assert.equal(storage.items.get(STORAGE_KEY), '[]');
});

test("user presets can't take a factory name", async () => {
  const { presets } = setUp();

  assert.match((await presets.save('wobble', '')) ?? '', /factory preset/);
  assert.equal(await presets.save('   ', ''), 'Enter a preset name.');
  assert.equal(await presets.exists('wobble'), false);

  assert.equal(await presets.save('Mine', ''), null);
  assert.match((await presets.rename(named(await presets.list(), 'Mine'), 'Sub')) ?? '', /factory preset/);
  assert.equal(await presets.exists('Mine'), true);
});

test("a stored preset loads past ids this engine doesn't have", async () => {
  const storage = fakeStorage();
  storage.setItem(STORAGE_KEY, JSON.stringify([{ name: 'Other', category: '', params: { gone: 1, filter_cutoff: 900 }, modulation: {}, curves: [] }]));
  const { presets, cutoff } = setUp(storage);

  await presets.load(named(await presets.list(), 'Other'));

  assert.equal(cutoff(), 900);
  assert.equal(presets.status().current.name, 'Other');
});

test('next and previous skip a user preset deleted in another tab', async () => {
  const { presets, storage } = setUp();
  await presets.save('A', '');
  await presets.save('B', '');
  await presets.load(named(await presets.list(), 'A'));

  const otherTab = setUp(storage).presets;
  assert.equal(await otherTab.remove(named(await otherTab.list(), 'B')), null);

  await presets.loadNext();
  assert.equal(presets.status().current.name, FACTORY_NAMES[0]);
});

test('unreadable storage leaves only the factory presets', async () => {
  const storage = fakeStorage();
  storage.setItem(STORAGE_KEY, 'not json');
  const { presets } = setUp(storage);
  assert.deepEqual((await presets.list()).map((preset) => preset.name), FACTORY_NAMES);
});
