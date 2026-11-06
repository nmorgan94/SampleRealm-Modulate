type ParamKind = 'real' | 'integer' | 'choice';

export interface ParamInfo {
  id: string;
  name: string;
  kind: ParamKind;
  min: number;
  max: number;
  defaultValue: number;
  step: number;
  skew: number;
  unit: string;
  choices: string[];
}

export interface UiInfo {
  version: string;
  commit: string;
  build: 'Debug' | 'Release';
  params: ParamInfo[];
}

export interface About extends Pick<UiInfo, 'version' | 'commit' | 'build'> {
  // "VST3 in Ableton Live", or where else the editor is running
  running: string;
}

// Linear peak per channel since the last level
export interface Level {
  peaks: number[];
}

export type ModulationSource = 'lfo' | 'env';

// As Modulation::Source, where 0 is none
export const SOURCE_NUMBERS = { lfo: 1, env: 2 } as const;

// As ModulationManager saves it: the range is in the parameter's 0..1 proportion
export interface Modulation {
  source: ModulationSource;
  index: number;
  min: number;
  max: number;
}

// setModulation's arguments in the engine and plugin, where source 0 clears it
export const toRouting = (modulation: Modulation | null) =>
  modulation
    ? { source: SOURCE_NUMBERS[modulation.source], sourceIndex: modulation.index, min: modulation.min, max: modulation.max }
    : { source: 0, sourceIndex: 0, min: 0, max: 0 };

export interface Point {
  x: number;
  y: number;
}

// As ADSREnvelope::State
export const EnvelopeStage = { Idle: 0, Attack: 1, Decay: 2, Sustain: 3, Release: 4 } as const;
export type EnvelopeStage = (typeof EnvelopeStage)[keyof typeof EnvelopeStage];

// ModulateSynth::Live: one entry per LFO, and per envelope
export interface Live {
  lfoPhase: number[];
  lfoValue: number[];
  envelopeLevel: number[];
  envelopeProgress: number[];
  envelopeStage: EnvelopeStage[];
}

export interface PresetEntry {
  name: string;
  category: string;
  isFactory: boolean;
}

export interface PresetStatus {
  current: PresetEntry;
  modified: boolean;
}

// Mirrors Source/presets/PresetManager.h
export interface PresetBackend {
  status(): PresetStatus;
  onStatus(listener: (status: PresetStatus) => void): () => void;
  // Factory then user presets, rescanning the user ones
  list(): Promise<PresetEntry[]>;
  load(preset: PresetEntry): Promise<void>;
  loadInit(): Promise<void>;
  loadNext(): Promise<void>;
  loadPrevious(): Promise<void>;
  exists(name: string): Promise<boolean>;
  // Each resolves to an error message, or null
  save(name: string, category: string): Promise<string | null>;
  rename(preset: PresetEntry, newName: string): Promise<string | null>;
  remove(preset: PresetEntry): Promise<string | null>;
  // Only where presets are files
  showFolder?(): void;
}

export const isSamePreset = (a: PresetEntry, b: PresetEntry) => a.name === b.name && a.isFactory === b.isFactory;

export interface ModulateBackend {
  info: UiInfo;
  about: About;
  presets: PresetBackend;
  param(id: string): ParamInfo;
  // Real values, not 0..1
  getValue(id: string): number;
  setValue(id: string, value: number): void;
  beginGesture(id: string): void;
  endGesture(id: string): void;
  subscribe(id: string, listener: (value: number) => void): () => void;
  // null when the parameter has none
  getModulation(id: string): Modulation | null;
  setModulation(id: string, modulation: Modulation | null): void;
  onModulation(listener: () => void): () => void;
  // An empty curve is the default shape
  getCurve(lfo: number): Point[];
  setCurve(lfo: number, points: Point[]): void;
  onCurve(listener: (lfo: number) => void): () => void;
  onLevel(listener: (level: Level) => void): () => void;
  onLive(listener: (live: Live) => void): () => void;
}

export const paramLookup = <T>(entries: Map<string, T>) => (id: string) => {
  const found = entries.get(id);
  if (!found) throw new Error(`No parameter ${id}`);
  return found;
};

export const listenerSet = <Args extends unknown[]>() => {
  const listeners = new Set<(...args: Args) => void>();
  return {
    add: (listener: (...args: Args) => void) => {
      listeners.add(listener);
      return () => void listeners.delete(listener);
    },
    notify: (...args: Args) => listeners.forEach((listener) => listener(...args)),
  };
};

export const NUM_LFOS = 4;
export const NUM_ENVELOPES = 4;
export const MAX_CURVE_POINTS = 32;
