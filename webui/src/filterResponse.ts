// Mirrors Source/dsp/Filter.h, for the filter panel's response curve
export const FilterMode = { Lowpass: 0, Highpass: 1, Bandpass: 2, Notch: 3 } as const;
export const FilterSlope = { TwoPole: 0, FourPole: 1 } as const;

const BUTTERWORTH_Q1 = 0.541196;
const BUTTERWORTH_Q2 = 1.306563;
const NEUTRAL_Q = 0.707107;
const BANDPASS_Q_SCALE = 0.643594;

type Coefficients = [b0: number, b1: number, b2: number, a1: number, a2: number];

const makeCoefficients = (mode: number, sampleRate: number, frequency: number, q: number): Coefficients => {
  const invQ = 1 / q;

  if (mode === FilterMode.Highpass) {
    const n = Math.tan((Math.PI * frequency) / sampleRate);
    const c1 = 1 / (1 + invQ * n + n * n);
    return [c1, c1 * -2, c1, c1 * 2 * (n * n - 1), c1 * (1 - invQ * n + n * n)];
  }

  const n = 1 / Math.tan((Math.PI * frequency) / sampleRate);
  const c1 = 1 / (1 + invQ * n + n * n);
  const a2 = c1 * (1 - invQ * n + n * n);

  if (mode === FilterMode.Bandpass) return [c1 * n * invQ, 0, -c1 * n * invQ, c1 * 2 * (1 - n * n), a2];
  if (mode === FilterMode.Notch) {
    const b1 = 2 * c1 * (1 - n * n);
    return [c1 * (1 + n * n), b1, c1 * (1 + n * n), b1, a2];
  }
  return [c1, c1 * 2, c1, c1 * 2 * (1 - n * n), a2];
};

const magnitude = ([b0, b1, b2, a1, a2]: Coefficients, frequency: number, sampleRate: number) => {
  const w = (2 * Math.PI * frequency) / sampleRate;
  // e^-jw and e^-2jw
  const [c1, s1, c2, s2] = [Math.cos(w), -Math.sin(w), Math.cos(2 * w), -Math.sin(2 * w)];
  const numerator = Math.hypot(b0 + b1 * c1 + b2 * c2, b1 * s1 + b2 * s2);
  const denominator = Math.hypot(1 + a1 * c1 + a2 * c2, a1 * s1 + a2 * s2);
  return numerator / denominator;
};

// The filter's gain at a frequency, for its mode, slope, cutoff and resonance (0..1)
export function filterMagnitude(mode: number, slope: number, cutoff: number, resonance: number, frequency: number, sampleRate: number) {
  const f = Math.min(20000, 0.49 * sampleRate, Math.max(20, cutoff));
  frequency = Math.min(frequency, 0.5 * sampleRate);
  const q = 0.5 + Math.min(1, Math.max(0, resonance)) * 19.5;
  const fourPole = slope === FilterSlope.FourPole && mode !== FilterMode.Notch;

  if (!fourPole) return magnitude(makeCoefficients(mode, sampleRate, f, q), frequency, sampleRate);

  const bandpass = mode === FilterMode.Bandpass;
  const q1 = bandpass ? q * BANDPASS_Q_SCALE : BUTTERWORTH_Q1;
  const q2 = bandpass ? q * BANDPASS_Q_SCALE : BUTTERWORTH_Q2 * (q / NEUTRAL_Q);

  return magnitude(makeCoefficients(mode, sampleRate, f, q1), frequency, sampleRate) * magnitude(makeCoefficients(mode, sampleRate, f, q2), frequency, sampleRate);
}
