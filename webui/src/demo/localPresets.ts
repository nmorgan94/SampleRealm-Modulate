// Source/presets/PresetManager.cpp's rules, with user presets in the browser instead of files
import { isSamePreset, listenerSet, NUM_LFOS, SOURCE_NUMBERS, type Modulation, type ModulationSource, type Point, type PresetBackend, type PresetEntry, type PresetStatus } from '../backend.ts';

// What the plugin's preset file holds: parameters, modulation and curves. Anything missing loads at its default
export interface PresetState {
  params: Record<string, number>;
  modulation: Record<string, Modulation>;
  curves: Point[][];
}

export type StoredPreset = PresetEntry & PresetState;
export type PresetStorage = Pick<Storage, 'getItem' | 'setItem'>;

// Each plugin's demo shares the site's storage
export const STORAGE_KEY = 'samplerealm-modulate-presets';

const INIT: StoredPreset = { name: 'Init', category: '', isFactory: true, params: {}, modulation: {}, curves: [] };

const ENTITIES: Record<string, string> = { quot: '"', apos: "'", lt: '<', gt: '>', amp: '&' };

// XmlElement writes other characters it can't leave as-is by number
const decode = (text: string) =>
  text.replace(/&(?:#(\d+)|(quot|apos|lt|gt|amp));/g, (_, code: string | undefined, entity: string) =>
    code ? String.fromCodePoint(Number(code)) : ENTITIES[entity]!);

const attributes = (tag: string) =>
  Object.fromEntries([...tag.matchAll(/([\w:-]+)="([^"]*)"/g)].map(([, key, value]) => [key, decode(value!)]));

const tags = (xml: string, name: string) => [...xml.matchAll(new RegExp(`<${name}\\s[^>]*>`, 'g'))].map(([tag]) => attributes(tag));

const SOURCES: ModulationSource[] = ['lfo', 'env'];

// Reads the files PresetManager writes; there's no DOMParser in node
export function parsePreset(xml: string): StoredPreset {
  const preset = tags(xml, 'PRESET')[0];
  if (!preset) throw new Error('Not a preset');

  const params = Object.fromEntries(tags(xml, 'PARAM').map(({ id, value }) => [id, Number(value)]));

  const modulation: Record<string, Modulation> = {};
  for (const { parameterID, sourceType, sourceIndex, minValue, maxValue } of tags(xml, 'Assignment')) {
    const source = SOURCES.find((candidate) => SOURCE_NUMBERS[candidate] === Number(sourceType));
    if (parameterID && source) modulation[parameterID] = { source, index: Number(sourceIndex), min: Number(minValue), max: Number(maxValue) };
  }

  const curves: Point[][] = Array.from({ length: NUM_LFOS }, () => []);
  for (const [, tag, body] of xml.matchAll(/(<LFOCurve\s[^>]*>)([\s\S]*?)<\/LFOCurve>/g)) {
    const lfo = Number(attributes(tag!).index);
    if (lfo >= 0 && lfo < NUM_LFOS) curves[lfo] = tags(body!, 'Point').map(({ x, y }) => ({ x: Number(x), y: Number(y) }));
  }

  return { name: preset.name ?? '', category: preset.category ?? '', isFactory: true, params, modulation, curves };
}

// juce::String::compareNatural
const collator = new Intl.Collator(undefined, { numeric: true, sensitivity: 'base' });

const sorted = <T extends PresetEntry>(presets: T[]) =>
  [...presets].sort((a, b) => collator.compare(a.category, b.category) || collator.compare(a.name, b.name));

// juce::File::createLegalFileName
const legalName = (name: string) => name.trim().replace(/["#@,;:<>*^|?\\/]/g, '');

// juce::String::equalsIgnoreCase, and macOS filenames
const sameName = (a: string, b: string) => a.toLowerCase() === b.toLowerCase();

interface Options {
  factory: StoredPreset[];
  storage?: PresetStorage;
  capture(): PresetState;
  apply(state: PresetState): void;
}

export type LocalPresets = PresetBackend & { markModified(): void };

export function localPresets({ factory, storage, capture, apply }: Options): LocalPresets {
  const factoryPresets = sorted(factory);
  let users: StoredPreset[] = [];
  let status: PresetStatus = { current: INIT, modified: false };
  const listeners = listenerSet<[PresetStatus]>();

  const read = () => {
    try {
      const stored = JSON.parse(storage?.getItem(STORAGE_KEY) ?? '[]') as StoredPreset[];
      users = sorted(stored.map((preset) => ({ ...preset, isFactory: false })));
    } catch {
      // Unreadable storage keeps what's already listed
    }
  };

  const write = () => {
    try {
      storage?.setItem(STORAGE_KEY, JSON.stringify(users.map(({ name, category, params, modulation, curves }) => ({ name, category, params, modulation, curves }))));
    } catch {
      // Private windows can refuse storage; the presets last until the page closes
    }
  };

  const setStatus = ({ name, category, isFactory }: PresetEntry, modified: boolean) => {
    status = { current: { name, category, isFactory }, modified };
    listeners.notify(status);
  };

  const all = () => [...factoryPresets, ...users];
  const find = (preset: PresetEntry) => all().find((candidate) => isSamePreset(candidate, preset));
  const findUser = (name: string) => users.find((preset) => sameName(preset.name, name));

  const checkName = (name: string) => {
    if (!name) return 'Enter a preset name.';
    const factoryMatch = factoryPresets.find((preset) => sameName(preset.name, name));
    return factoryMatch ? `"${factoryMatch.name}" is a factory preset. Choose another name.` : null;
  };

  const load = (preset: StoredPreset) => {
    apply(preset);
    setStatus(preset, false);
  };

  const markModified = () => {
    if (!status.modified) setStatus(status.current, true);
  };

  const step = async (delta: number) => {
    read();
    const presets = all();
    if (presets.length === 0) return;

    const index = presets.findIndex((preset) => isSamePreset(preset, status.current));
    if (index < 0) load(delta > 0 ? presets[0]! : presets[presets.length - 1]!);
    else load(presets[(index + delta + presets.length) % presets.length]!);
  };

  read();

  return {
    status: () => status,
    onStatus: listeners.add,
    list: async () => {
      read();
      return all();
    },
    load: async (preset) => {
      const found = find(preset);
      if (found) load(found);
    },
    loadInit: async () => load(INIT),
    loadNext: () => step(1),
    loadPrevious: () => step(-1),
    exists: async (name) => findUser(legalName(name)) !== undefined,
    save: async (name, category) => {
      const trimmed = legalName(name);
      const error = checkName(trimmed);
      if (error) return error;

      const preset: StoredPreset = { name: trimmed, category, isFactory: false, ...capture() };
      users = sorted([...users.filter((user) => !sameName(user.name, trimmed)), preset]);
      write();
      setStatus(preset, false);
      return null;
    },
    rename: async (preset, newName) => {
      const trimmed = legalName(newName);
      const target = preset.isFactory ? undefined : findUser(preset.name);
      if (!target) return 'Only user presets can be renamed.';

      const error = checkName(trimmed);
      if (error) return error;

      const existing = findUser(trimmed);
      if (existing && existing !== target) return `A preset called "${trimmed}" already exists.`;

      users = sorted(users.map((user) => (user === target ? { ...user, name: trimmed } : user)));
      write();

      if (isSamePreset(status.current, target)) setStatus({ ...status.current, name: trimmed }, status.modified);
      return null;
    },
    remove: async (preset) => {
      const target = preset.isFactory ? undefined : findUser(preset.name);
      if (!target) return 'Only user presets can be deleted.';

      users = users.filter((user) => user !== target);
      write();

      if (isSamePreset(status.current, target)) markModified();
      return null;
    },
    markModified,
  };
}
