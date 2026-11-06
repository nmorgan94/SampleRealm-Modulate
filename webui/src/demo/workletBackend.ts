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
import { localPresets, type PresetStorage, type StoredPreset } from './localPresets.ts';

// The demo has no host to follow
const DEMO_TEMPO = 120;

const NOTE_ON = 0x90;
const NOTE_OFF = 0x80;
const VELOCITY = 100;

// WASI imports are never called
export const stubImports = () => {
  const stub = new Proxy({}, { get: () => () => 0 });
  return new Proxy({}, { get: () => stub }) as WebAssembly.Imports;
};

export type Engine = Record<string, CallableFunction> & { memory: WebAssembly.Memory };

export const readUiInfo = (engine: Engine) => {
  const bytes = new Uint8Array(engine.memory.buffer, engine.uiInfo());
  return JSON.parse(new TextDecoder().decode(bytes.subarray(0, bytes.indexOf(0)))) as UiInfo;
};

export async function loadEngine(url: string): Promise<{ wasm: ArrayBuffer; info: UiInfo }> {
  const response = await fetch(url);
  if (!response.ok) throw new Error(`${url}: ${response.status}`);

  const wasm = await response.arrayBuffer();
  const { instance } = await WebAssembly.instantiate(wasm, stubImports());
  const engine = instance.exports as Engine;
  engine._initialize?.();
  return { wasm, info: readUiInfo(engine) };
}

// The node's port, so tests can stand in for it
export type EnginePort = Pick<MessagePort, 'postMessage'> & { onmessage: ((e: MessageEvent) => void) | null };

// Storage can be missing or throw, e.g. in a private window
export const browserStorage = (): PresetStorage | undefined => {
  try {
    return globalThis.localStorage;
  } catch {
    return undefined;
  }
};

export type WorkletBackend = ModulateBackend & {
  noteOn(note: number): void;
  noteOff(note: number): void;
  midi(bytes: ArrayLike<number>): void;
  attach(port: EnginePort): void;
};

export function workletBackend(info: UiInfo, factoryPresets: StoredPreset[] = [], storage?: PresetStorage): WorkletBackend {
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
  let pending: unknown[] = [{ type: 'tempo', bpm: DEMO_TEMPO }];

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
    factory: factoryPresets,
    storage,
    capture: () => ({
      params: Object.fromEntries(info.params.map(({ id }) => [id, entry(id).value])),
      modulation: Object.fromEntries(modulations),
      curves: curves.map((points) => points.map((point) => ({ ...point }))),
    }),
    apply: (state) => {
      // Ids this engine doesn't have are skipped, as the APVTS skips them
      for (const { id, defaultValue } of info.params) applyValue(id, state.params[id] ?? defaultValue);
      for (const { id } of info.params) storeModulation(id, state.modulation[id] ?? null);
      modulationListeners.notify();
      for (let lfo = 0; lfo < NUM_LFOS; lfo++) applyCurve(lfo, state.curves[lfo] ?? []);
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
    about: { version: info.version, commit: info.commit, build: info.build, running: 'Web demo' },
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
