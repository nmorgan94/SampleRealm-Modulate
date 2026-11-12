import type { PluginBackend } from '@samplerealm/modules/backend.ts';

export * from '@samplerealm/modules/backend.ts';

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

export interface ModulateBackend extends PluginBackend {
  // null when the parameter has none
  getModulation(id: string): Modulation | null;
  setModulation(id: string, modulation: Modulation | null): void;
  onModulation(listener: () => void): () => void;
  // An empty curve is the default shape
  getCurve(lfo: number): Point[];
  setCurve(lfo: number, points: Point[]): void;
  onCurve(listener: (lfo: number) => void): () => void;
  onLive(listener: (live: Live) => void): () => void;
}

export const NUM_LFOS = 4;
export const NUM_ENVELOPES = 4;
export const MAX_CURVE_POINTS = 32;
