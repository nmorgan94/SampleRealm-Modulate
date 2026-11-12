// Mirrors Source/dsp/Modulation.h, and the modulated value of ModulateSynth
import type { Live, Modulation, ParamInfo } from './backend.ts';
import { clamp01, rangeFromProportion, toProportion } from '@samplerealm/modules/editor/params.ts';


export const modulatedProportion = ({ min, max }: Modulation, sourceValue: number) => clamp01(min + sourceValue * (max - min));

export const makeDefaultRange = (source: Modulation['source'], index: number, base: number, depth = 0.25): Modulation =>
  source === 'env'
    ? { source, index, min: base, max: Math.min(1, base + depth) }
    : { source, index, min: clamp01(base - depth), max: Math.min(1, base + depth) };

export const applyDepthChange = (modulation: Modulation, base: number, delta: number): Modulation => {
  if (modulation.source === 'env') {
    const roomAbove = Math.max(0, 1 - base);
    const depth = Math.min(roomAbove, Math.max(Math.min(0.05, roomAbove), modulation.max - modulation.min + delta));
    return { ...modulation, min: base, max: base + depth };
  }

  const depth = Math.min(1, Math.max(0.05, (modulation.max - modulation.min) * 0.5 + delta));
  const roomBelow = base;
  const roomAbove = 1 - base;

  if (depth <= Math.min(roomBelow, roomAbove)) return { ...modulation, min: base - depth, max: base + depth };

  const actual = Math.min(depth, Math.max(roomBelow, roomAbove));
  return { ...modulation, min: Math.max(0, base - actual), max: Math.min(1, base + actual) };
};

export const sourceValue = (live: Live, { source, index }: Modulation) =>
  (source === 'lfo' ? live.lfoValue[index] : live.envelopeLevel[index]) ?? 0;

// The value the voices hear
export const modulatedValue = (param: ParamInfo, value: number, modulation: Modulation | null, source: number) =>
  rangeFromProportion(param, modulation ? modulatedProportion(modulation, source) : toProportion(param, value));
