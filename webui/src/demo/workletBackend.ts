// Modulate's own: the shared one carries no modulation, curves or live state
import { browserStorage, localPresets, type PresetStorage } from '@samplerealm/modules/demo/localPresets.ts';
import type { EnginePort } from '@samplerealm/modules/demo/workletBackend.ts';
import {
  listenerSet,
  MAX_CURVE_POINTS,
  NUM_LFOS,
  paramLookup,
  toRouting,
  type Level,
  type Live,
  type ModulateBackend,
  type Modulation,
  type Point,
  type UiInfo,
} from '../backend.ts';
import { INIT_STATE, parsePresetState, STORAGE_KEY } from './presetState.ts';

const NOTE_ON = 0x90;
const NOTE_OFF = 0x80;
const VELOCITY = 100;

export type WorkletBackend = ModulateBackend & {
  noteOn(note: number): void;
  noteOff(note: number): void;
  midi(bytes: ArrayLike<number>): void;
  attach(port: EnginePort): void;
};

interface Options {
  // The factory preset files' text
  factoryPresets?: string[];
  // The page's own unless given
  storage?: PresetStorage;
}

export function workletBackend(info: UiInfo, { factoryPresets = [], storage = browserStorage() }: Options = {}): WorkletBackend {
  const entry = paramLookup(new Map(info.params.map((param, index) => [param.id, { param, index, value: param.defaultValue }])));
  const listeners = new Map<string, Set<(value: number) => void>>();
  const modulations = new Map<string, Modulation>();
  const curves: Point[][] = Array.from({ length: NUM_LFOS }, () => []);
  const modulationListeners = listenerSet<[]>();
  const curveListeners = listenerSet<[lfo: number]>();
  const levelListeners = listenerSet<[level: Level]>();
  const liveListeners = listenerSet<[live: Live]>();
  let port: EnginePort | null = null;
  // Sent once the worklet attaches
  let pending: unknown[] = [];

  const post = (message: unknown) => {
    if (port) port.postMessage(message);
    else pending.push(message);
  };

  // Loading a preset applies through these, so they don't mark it modified
  const applyValue = (id: string, value: number) => {
    const found = entry(id);
    post({ type: 'param', index: found.index, value });
    found.value = value;
    listeners.get(id)?.forEach((listener) => listener(value));
  };

  // Doesn't notify, so a preset load can notify once for all of them
  const storeModulation = (id: string, modulation: Modulation | null) => {
    if (modulation) modulations.set(id, modulation);
    else modulations.delete(id);

    post({ type: 'modulation', index: entry(id).index, ...toRouting(modulation) });
  };

  const applyCurve = (lfo: number, points: Point[]) => {
    // The engine's points() holds no more
    const kept = points.slice(0, MAX_CURVE_POINTS);
    curves[lfo] = kept.map(({ x, y }) => ({ x, y }));
    post({ type: 'curve', lfo, points: kept.flatMap(({ x, y }) => [x, y]) });
    curveListeners.notify(lfo);
  };

  const presets = localPresets({
    storageKey: STORAGE_KEY,
    storage,
    factory: factoryPresets,
    parse: parsePresetState,
    init: INIT_STATE,
    capture: () => ({
      params: Object.fromEntries(info.params.map(({ id }) => [id, entry(id).value])),
      modulation: Object.fromEntries(modulations),
      curves: curves.map((points) => points.map((point) => ({ ...point }))),
    }),
    apply: ({ params = {}, modulation = {}, curves: stored = [] }) => {
      // Ids this engine doesn't have are skipped, as the APVTS skips them
      for (const { id, defaultValue } of info.params) applyValue(id, params[id] ?? defaultValue);
      for (const { id } of info.params) storeModulation(id, modulation[id] ?? null);
      modulationListeners.notify();
      for (let lfo = 0; lfo < NUM_LFOS; lfo++) applyCurve(lfo, stored[lfo] ?? []);
    },
  });

  const setValue = (id: string, value: number) => {
    applyValue(id, value);
    presets.markModified();
  };

  const midi = (bytes: ArrayLike<number>) => {
    const [status = 0, data1 = 0, data2 = 0] = Array.from(bytes);
    post({ type: 'midi', bytes: [status, data1, data2] });
  };

  return {
    info,
    running: 'Web demo',
    presets,
    param: (id) => entry(id).param,
    getValue: (id) => entry(id).value,
    setValue,
    beginGesture: () => {},
    endGesture: () => {},
    subscribe: (id, listener) => {
      const set = listeners.get(id) ?? new Set();
      listeners.set(id, set.add(listener));
      return () => set.delete(listener);
    },
    getModulation: (id) => modulations.get(id) ?? null,
    setModulation: (id, modulation) => {
      storeModulation(id, modulation);
      modulationListeners.notify();
      presets.markModified();
    },
    onModulation: modulationListeners.add,
    getCurve: (lfo) => curves[lfo] ?? [],
    setCurve: (lfo, points) => {
      applyCurve(lfo, points);
      presets.markModified();
    },
    onCurve: curveListeners.add,
    onLevel: levelListeners.add,
    onLive: liveListeners.add,
    noteOn: (note) => midi([NOTE_ON, note, VELOCITY]),
    noteOff: (note) => midi([NOTE_OFF, note, 0]),
    midi,
    attach: (enginePort) => {
      port = enginePort;
      port.onmessage = ({ data }) => {
        if (data.type === 'level') levelListeners.notify({ peaks: data.peaks });
        else if (data.type === 'live') liveListeners.notify(data.live);
      };
      pending.forEach((message) => enginePort.postMessage(message));
      pending = [];
    },
  };
}
