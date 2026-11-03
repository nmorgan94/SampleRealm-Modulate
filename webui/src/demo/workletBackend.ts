import {
  INIT_PRESET,
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

export const STORAGE_KEY = 'samplerealm-modulate-presets';

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

export type PresetStorage = Pick<Storage, 'getItem' | 'setItem'>;

// What the plugin's preset file holds: parameters, modulation and curves
export interface StoredPreset {
  params: Record<string, number>;
  modulation: Record<string, Modulation>;
  curves: Point[][];
}

// Storage can be missing or throw, e.g. in a private window
const browserStorage = (): PresetStorage | undefined => {
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

export function workletBackend(info: UiInfo, storage: PresetStorage | undefined = browserStorage()): WorkletBackend {
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
  let currentPreset = '';

  const post = (message: unknown) => {
    if (port) port.postMessage(message);
    else pending.push(message);
  };

  const setValue = (id: string, value: number) => {
    const found = entry(id);
    post({ type: 'param', index: found.index, value });
    found.value = value;
    listeners.get(id)?.forEach((listener) => listener(value));
  };

  const setModulation = (id: string, modulation: Modulation | null) => {
    if (modulation) modulations.set(id, modulation);
    else modulations.delete(id);

    post({ type: 'modulation', index: entry(id).index, ...toRouting(modulation) });
    modulationListeners.notify();
  };

  const setCurve = (lfo: number, points: Point[]) => {
    // The engine's points() holds no more
    const kept = points.slice(0, MAX_CURVE_POINTS);
    curves[lfo] = kept.map(({ x, y }) => ({ x, y }));
    post({ type: 'curve', lfo, points: kept.flatMap(({ x, y }) => [x, y]) });
    curveListeners.notify(lfo);
  };

  const apply = (preset: StoredPreset) => {
    // Ids this engine doesn't have are skipped, as the APVTS skips them
    for (const { id, defaultValue } of info.params) setValue(id, preset.params[id] ?? defaultValue);
    for (const { id } of info.params) setModulation(id, preset.modulation[id] ?? null);
    for (let lfo = 0; lfo < NUM_LFOS; lfo++) setCurve(lfo, preset.curves[lfo] ?? []);
  };

  const readPresets = (): Record<string, StoredPreset> => {
    try {
      return JSON.parse(storage?.getItem(STORAGE_KEY) ?? '{}') as Record<string, StoredPreset>;
    } catch {
      return {};
    }
  };

  const writePresets = (presets: Record<string, StoredPreset>) => {
    try {
      storage?.setItem(STORAGE_KEY, JSON.stringify(presets));
      return storage !== undefined;
    } catch {
      return false;
    }
  };

  const midi = (bytes: ArrayLike<number>) => {
    const [status = 0, data1 = 0, data2 = 0] = Array.from(bytes);
    post({ type: 'midi', bytes: [status, data1, data2] });
  };

  return {
    info,
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
    setModulation,
    onModulation: modulationListeners.add,
    getCurve: (lfo) => curves[lfo] ?? [],
    setCurve,
    onCurve: curveListeners.add,
    onLevel: levelListeners.add,
    onLive: liveListeners.add,
    presets: {
      list: async () => ({ names: Object.keys(readPresets()).sort((a, b) => a.localeCompare(b)), current: currentPreset }),
      load: async (name) => {
        const stored = name === INIT_PRESET ? { params: {}, modulation: {}, curves: [] } : readPresets()[name];
        if (!stored) return false;
        apply(stored);
        currentPreset = name === INIT_PRESET ? '' : name;
        return true;
      },
      save: async (name) => {
        if (!name || name === INIT_PRESET) return false;
        const preset: StoredPreset = {
          params: Object.fromEntries(info.params.map(({ id }) => [id, entry(id).value])),
          modulation: Object.fromEntries(modulations),
          curves: curves.map((points) => points.map((point) => ({ ...point }))),
        };
        const saved = writePresets({ ...readPresets(), [name]: preset });
        if (saved) currentPreset = name;
        return saved;
      },
      remove: async (name) => {
        const presets = readPresets();
        if (!(name in presets)) return false;
        delete presets[name];
        if (name === currentPreset) currentPreset = '';
        return writePresets(presets);
      },
    },
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
