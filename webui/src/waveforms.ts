// Mirrors Source/dsp/WavetableGenerator.h, for the oscillator displays
export const WAVETABLE_SIZE = 2048;

const f = Math.fround;
const TWO_PI = f(2 * Math.PI);

// The angle as the C++ works it out in float, so the branches on a sine's sign agree at zero crossings
const rad = (phase: number, multiple = 1) => (multiple === 1 ? f(TWO_PI * phase) : f(f(TWO_PI * phase) * multiple));

// std::round, which rounds halves away from zero
const roundHalfAway = (value: number) => Math.sign(value) * Math.round(Math.abs(value));

const clamp = (low: number, high: number, value: number) => Math.min(high, Math.max(low, value));

type Generator = (phase: number, i: number, size: number) => number;

const each = (sample: Generator) => (size: number) => Float32Array.from({ length: size }, (_, i) => sample(i / size, i, size));

const GENERATORS: readonly ((size: number) => Float32Array)[] = [
  // Sine
  each((_, i, size) => Math.sin(f(f(TWO_PI * i) / size))),
  // Saw
  each((phase) => 2 * phase - 1),
  // Square
  each((_, i, size) => (i < size / 2 ? 1 : -1)),
  // Triangle
  each((phase, i, size) => (i < size / 2 ? -1 + 4 * phase : 3 - 4 * phase)),
  // Pulse 25%
  each((_, i, size) => (i < size / 4 ? 1 : -1)),
  // Pulse 10%
  each((_, i, size) => (i < Math.trunc(size / 10) ? 1 : -1)),
  // Formant Sine
  each((_, i, size) => {
    const sine = Math.sin(f(f(TWO_PI * i) / size));
    return Math.sqrt(Math.abs(sine)) * (sine > 0 ? 1 : -1);
  }),
  // Formant Vowel
  each((phase) => clamp(-1, 1, Math.sin(rad(phase)) + Math.sin(rad(phase, 5)) * 0.4)),
  // Bitcrushed Ramp
  each((phase) => roundHalfAway((1 - 2 * phase) * 8) / 8),
  // Fm Operator: 3:1, index 1.5
  each((phase) => Math.sin(f(rad(phase) + f(Math.sin(rad(phase, 3)) * 1.5)))),
  // Feedback Fm
  (size) => {
    const samples = new Float32Array(size);
    let last = 0;
    for (let i = 0; i < size; i++) {
      last = f(Math.sin(f(rad(i / size) + f(last * 0.8))));
      samples[i] = last;
    }
    return samples;
  },
  // Wavefolded Sine
  each((phase) => {
    let driven = Math.sin(rad(phase)) * 3;
    if (driven > 1) driven = 2 - driven;
    if (driven < -1) driven = -2 - driven;
    return driven;
  }),
  // Fractal Pulse
  each((_, i, size) => [1, 0.25, -1, -0.25][Math.trunc((i * 4) / size)]!),
  // Double Saw
  each((phase) => (1 - 2 * phase) * 0.6 - (1 - 2 * ((phase * 2) % 1)) * 0.4),
  // Resonant Saw
  each((phase) => clamp(-1, 1, 1 - 2 * phase + Math.sin(rad(phase, 12)) * Math.exp(-phase * 4) * 0.5)),
  // Prime Cluster
  each((phase) => Math.sin(rad(phase, 2)) * 0.6 + Math.sin(rad(phase, 5)) * 0.3 + Math.sin(rad(phase, 11)) * 0.1),
  // Odd Harmonic Cluster
  each(
    (phase) => (Math.sin(rad(phase)) + Math.sin(rad(phase, 3)) * 0.5 + Math.sin(rad(phase, 5)) * 0.25 + Math.sin(rad(phase, 7)) * 0.125) * 0.533
  ),
  // Log Square
  each((phase) => {
    const sine = Math.sin(rad(phase));
    return sine >= 0 ? Math.log1p(sine * 9) / Math.log1p(9) : -Math.log1p(-sine * 9) / Math.log1p(9);
  }),
  // Asymmetric Rectified Sine
  each((phase) => {
    const sine = Math.sin(rad(phase));
    return sine >= 0 ? sine : Math.abs(sine) * 0.5 - 0.5;
  }),
  // Interleaved Saw Pulse
  each((phase) => (phase < 0.5 ? 1 - 4 * phase : phase < 0.75 ? 1 : -1)),
  // Trapezoid
  each((phase) => clamp(-0.7, 0.7, phase < 0.5 ? 4 * phase - 1 : 3 - 4 * phase) / 0.7),
];

export const NUM_WAVEFORMS = GENERATORS.length;

export const generateWavetable = (type: number, size = WAVETABLE_SIZE) => GENERATORS[type]!(size);

// As the oscillator blends its two sources
export const blend = (a: Float32Array, b: Float32Array, morph: number) => {
  const m = Math.min(1, Math.max(0, morph));
  return a.map((sample, i) => sample + m * (b[i]! - sample));
};
