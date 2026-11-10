import type { ParamInfo } from './backend.ts';

export const clamp01 = (value: number) => Math.min(1, Math.max(0, value));

// As a JUCE slider moves: snapped to the step
export const fromProportion = (param: ParamInfo, proportion: number) => snap(param, rangeFromProportion(param, proportion));

// NormalisableRange::convertFrom0to1, which doesn't snap
export const rangeFromProportion = (param: ParamInfo, proportion: number) => {
  const p = clamp01(proportion);
  return param.min + (param.max - param.min) * (param.skew === 1 || p === 0 ? p : Math.pow(p, 1 / param.skew));
};

export const toProportion = (param: ParamInfo, value: number) => {
  const p = clamp01((value - param.min) / (param.max - param.min));
  return param.skew === 1 ? p : Math.pow(p, param.skew);
};

// roundToInt, which rounds halves to even
const roundHalfEven = (value: number) => {
  const rounded = Math.round(value);
  return Math.abs(value % 1) === 0.5 && rounded % 2 !== 0 ? rounded - 1 : rounded;
};

export const snap = (param: ParamInfo, value: number) => {
  const clamped = Math.min(param.max, Math.max(param.min, value));
  if (param.kind !== 'real') return roundHalfEven(clamped);
  if (param.step <= 0) return clamped;
  const snapped = param.min + Math.round((clamped - param.min) / param.step) * param.step;
  // Avoids 0.7000000000000001
  return Math.min(param.max, Number(snapped.toFixed(decimalPlaces(param.step))));
};

// As AudioParameterFloat counts them from its step
export const decimalPlaces = (step: number) => {
  if (step <= 0) return 7;
  if (Number.isInteger(step)) return 0;

  let places = 7;
  let scaled = Math.abs(Math.round(step * 10 ** places));
  while (scaled % 10 === 0 && places > 0) {
    places--;
    scaled /= 10;
  }
  return places;
};

// JUCE's default text for each kind of parameter
export const formatValue = (param: ParamInfo, value: number) => {
  if (param.kind === 'choice') return param.choices[Math.round(value)] ?? '';
  if (param.kind === 'integer') return `${Math.round(value)}`;

  return value.toFixed(decimalPlaces(param.step));
};

// As LabeledSlider's typed values: a choice takes only one of its names
export const parseValue = (param: ParamInfo, text: string): number | null => {
  if (param.kind === 'choice') {
    const typed = text.trim().toLowerCase();
    const index = param.choices.findIndex((choice) => choice.toLowerCase() === typed);
    return index < 0 ? null : index;
  }

  const number = parseFloat(text);
  if (Number.isNaN(number)) return null;
  return snap(param, number);
};
