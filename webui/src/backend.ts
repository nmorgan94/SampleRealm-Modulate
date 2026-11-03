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
  params: ParamInfo[];
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

// Mirrors Source/PresetManager.h: the user's presets by name, and the current one, or '' after Init.
// Loading INIT_PRESET resets everything
export interface Presets {
  list(): Promise<{ names: string[]; current: string }>;
  load(name: string): Promise<boolean>;
  save(name: string): Promise<boolean>;
  remove(name: string): Promise<boolean>;
}

export interface ModulateBackend {
  info: UiInfo;
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
  presets: Presets;
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

export const INIT_PRESET = 'Init';

export const NUM_LFOS = 4;
export const NUM_ENVELOPES = 4;
export const MAX_CURVE_POINTS = 32;
