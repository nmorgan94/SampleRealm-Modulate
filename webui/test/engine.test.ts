// The built engine under Node: notes, modulation, the synced LFO, and the plugin's golden renders
import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
import { test } from 'node:test';
import { blockSize, rms, sampleRate } from '@samplerealm/modules/testing/engine.ts';
import { SOURCE_NUMBERS, type ModulationSource } from '../src/backend.ts';
import { freshEngine, indexOf } from './engine.ts';

const noteOn = 0x90;
const noteOff = 0x80;

type CurvePoint = [x: number, y: number];

// A stereo engine prepared at `rate`, or unprepared for null
function synth(rate: number | null = sampleRate) {
  const { engine, info, set, view, renderBlocks } = freshEngine(2, rate);

  const modulate = (id: string, source: ModulationSource, index: number, min: number, max: number) =>
    engine.setModulation(indexOf(id), SOURCE_NUMBERS[source], index, min, max);
  const clearModulation = (id: string) => engine.setModulation(indexOf(id), 0, 0, 0, 0);

  const writePoints = (points: CurvePoint[]) => new Float32Array(engine.memory.buffer, engine.points(), points.length * 2).set(points.flat());
  const curve = (lfo: number, points: CurvePoint[]) => {
    writePoints(points);
    engine.setCurve(lfo, points.length);
  };

  const play = (seconds: number) => {
    const [left, right] = renderBlocks(Math.round((seconds * sampleRate) / blockSize) * blockSize);
    return { left: Array.from(left!), right: Array.from(right!) };
  };

  const live = () => {
    const values = Array.from(new Float32Array(engine.memory.buffer, engine.live(), 16));
    return { lfoPhase: values.slice(0, 4), lfoValue: values.slice(4, 8), envelopeLevel: values.slice(8, 12) };
  };

  return { engine, info, set, view, modulate, clearModulation, writePoints, curve, play, live };
}

// One sine on oscillator 1, straight to the output
function sineSynth() {
  const s = synth();
  s.set('filter_enable', 0);
  return s;
}

const peak = (samples: ArrayLike<number>) => Array.from(samples).reduce((largest, sample) => Math.max(largest, Math.abs(sample)), 0);

// RMS of the sample-to-sample change: larger for brighter sounds
const roughness = (samples: number[]) => rms(samples.slice(1).map((sample, i) => sample - samples[i]!));

const frequency = (samples: number[]) => {
  let rising = 0;
  for (let i = 1; i < samples.length; i++) if (samples[i - 1]! < 0 && samples[i]! >= 0) rising++;
  return rising / (samples.length / sampleRate);
};

const seconds = (samples: number[], from: number, to: number) => samples.slice(Math.round(from * sampleRate), Math.round(to * sampleRate));

const windowRms = (samples: number[], windowSeconds: number) => {
  const size = Math.round(windowSeconds * sampleRate);
  const windows: number[] = [];
  for (let start = 0; start + size <= samples.length; start += size) windows.push(rms(samples, start, start + size));
  return windows;
};

//==============================================================================
test('the ui info carries the version, the commit, the build type and every parameter as ParameterSpecs defines it', () => {
  const { info } = synth();

  assert.match(info.version, /^\d+\.\d+\.\d+$/);
  assert.match(info.commit, /^([0-9a-f]{7,}(-local)?|unknown)$/);
  assert.match(info.build, /^(Debug|Release)$/);
  assert.equal(info.params.length, 87);
  assert.equal(new Set(info.params.map((p) => p.id)).size, 87);
  assert.deepEqual(info.params.find((p) => p.id === 'glide_time'), {
    id: 'glide_time', name: 'Glide Time', kind: 'real', min: 0, max: 2, defaultValue: 0,
    step: 0.001, skew: 0.4, unit: '', choices: [],
  });
  assert.deepEqual(info.params.find((p) => p.id === 'num_voices'), {
    id: 'num_voices', name: 'Voices', kind: 'integer', min: 1, max: 8, defaultValue: 8,
    step: 1, skew: 1, unit: '', choices: [],
  });

  const waveforms = info.params.find((p) => p.id === 'osc2_waveform');
  assert.ok(waveforms);
  assert.equal(waveforms.kind, 'choice');
  assert.equal(waveforms.defaultValue, 1);
  assert.equal(waveforms.choices.length, 21);
  assert.equal(waveforms.choices.at(-1), 'Trapezoid');
});

test('a note plays at its MIDI pitch, moved by oscillator pitch, fine tune and coarse pitch', () => {
  const cases: [id: string, value: number, expected: number][] = [
    ['osc1_pitch', 0, 440],
    ['osc1_pitch', 12, 880],
    ['osc1_fine', 50, 440 * 2 ** (50 / 1200)],
    ['coarse_pitch', -12, 220],
  ];

  for (const [id, value, expected] of cases) {
    const s = sineSynth();
    s.set(id, value);
    s.engine.midi(noteOn, 69, 100);

    const measured = frequency(s.play(0.5).left);
    assert.ok(Math.abs(measured - expected) < 3, `${id} ${value}: ${measured} Hz`);
  }
});

test('a release curve bends the fade: positive falls fast early, negative slowly', () => {
  const halfwayPeak = (curve: number) => {
    const s = sineSynth();
    s.set('env1_sustain', 1);
    s.set('env1_release', 0.4);
    s.set('env1_release_curve', curve);
    s.engine.midi(noteOn, 57, 100);
    s.play(0.3);
    s.engine.midi(noteOff, 57, 0);
    return peak(seconds(s.play(0.4).left, 0.19, 0.21));
  };

  const [linear, fast, slow] = [0, 0.8, -0.8].map(halfwayPeak) as [number, number, number];
  assert.ok(fast < linear * 0.6, `${fast} vs ${linear}`);
  assert.ok(slow > linear * 1.4, `${slow} vs ${linear}`);
});

test('a held note follows the sustain as it is turned', () => {
  const s = sineSynth();
  s.set('env1_sustain', 1);
  s.engine.midi(noteOn, 57, 100);
  s.play(0.3);
  const full = peak(s.play(0.05).left);

  s.set('env1_sustain', 0.5);
  s.play(0.01);

  const half = peak(s.play(0.05).left);
  assert.ok(Math.abs(half / full - 0.5) < 0.01, `${half} vs ${full}`);
});

test('changing the release while it plays carries on from the same level, at the new speed', () => {
  const s = sineSynth();
  s.set('env1_sustain', 1);
  s.set('env1_release', 1);
  s.engine.midi(noteOn, 57, 100);
  s.play(0.3);
  s.engine.midi(noteOff, 57, 0);
  s.play(0.3);

  const before = s.live().envelopeLevel[0]!;
  s.set('env1_release', 0.2);
  s.set('env1_release_curve', 0.8);
  s.play(blockSize / sampleRate);
  const after = s.live().envelopeLevel[0]!;

  assert.ok(after < before && before - after < 0.05, `${before} to ${after}`);
  assert.equal(peak(seconds(s.play(0.3).left, 0.2, 0.3)), 0);
});

test('releasing a note fades it to silence within the release time', () => {
  const s = sineSynth();
  s.set('env1_release', 0.2);
  s.engine.midi(noteOn, 57, 100);
  s.play(0.3);
  s.engine.midi(noteOff, 57, 0);

  const { left } = s.play(0.4);

  assert.ok(peak(left.slice(0, blockSize)) > 0.1);
  assert.equal(peak(seconds(left, 0.25, 0.4)), 0);
});

test('with one voice, a second held note glides there from the first', () => {
  const s = sineSynth();
  s.set('num_voices', 1);
  s.set('glide_time', 0.4);
  s.engine.midi(noteOn, 57, 100);
  s.play(0.2);
  s.engine.midi(noteOn, 69, 100);

  const midGlide = frequency(seconds(s.play(0.4).left, 0.15, 0.25));
  const settled = frequency(s.play(0.5).left);

  assert.ok(midGlide > 240 && midGlide < 420, `mid-glide ${midGlide} Hz`);
  assert.ok(Math.abs(settled - 440) < 3, `settled ${settled} Hz`);
});

test('in Rate mode the glide time is per octave, and in Time mode it is per slide', () => {
  const glideFrom45 = (mode: number, note: number) => {
    const s = sineSynth();
    s.set('num_voices', 1);
    s.set('glide_time', 0.2);
    s.set('glide_mode', mode);
    s.engine.midi(noteOn, 45, 100);
    s.play(0.1);
    s.engine.midi(noteOn, note, 100);
    return frequency(seconds(s.play(0.45).left, 0.25, 0.45));
  };

  const rateOctave = glideFrom45(1, 57);
  const rateTwoOctaves = glideFrom45(1, 69);
  const timeTwoOctaves = glideFrom45(0, 69);

  assert.ok(Math.abs(rateOctave - 220) < 10, `Rate, one octave ${rateOctave} Hz`);
  assert.ok(rateTwoOctaves > 330 && rateTwoOctaves < 400, `Rate, two octaves ${rateTwoOctaves} Hz`);
  assert.ok(Math.abs(timeTwoOctaves - 440) < 10, `Time, two octaves ${timeTwoOctaves} Hz`);
});

test('a note played mid-glide slides on from where the pitch is', () => {
  const s = sineSynth();
  s.set('num_voices', 1);
  s.set('glide_time', 0.4);
  s.set('glide_mode', 0);
  s.engine.midi(noteOn, 45, 100);
  s.play(0.1);
  s.engine.midi(noteOn, 69, 100);
  s.play(0.2);
  s.engine.midi(noteOn, 64, 100);

  const start = frequency(s.play(0.1).left);

  assert.ok(Math.abs(start - 231) < 15, `start ${start} Hz`);
});

test('with one voice, a note played legato keeps the envelope going', () => {
  const s = sineSynth();
  s.set('num_voices', 1);
  s.set('env1_attack', 1);
  s.engine.midi(noteOn, 57, 100);
  const before = peak(s.play(0.5).left.slice(-8 * blockSize));
  s.engine.midi(noteOn, 64, 100);
  const after = peak(s.play(0.02).left);

  assert.ok(after > 0.9 * before, `${before} before, ${after} after`);
});

test('with one voice, letting go of a key returns to the one still held', () => {
  const s = sineSynth();
  s.set('num_voices', 1);
  s.engine.midi(noteOn, 57, 100);
  s.play(0.1);
  s.engine.midi(noteOn, 69, 100);
  s.play(0.1);
  s.engine.midi(noteOff, 69, 0);

  const { left } = s.play(0.5);
  const held = frequency(seconds(left, 0.1, 0.5));

  assert.ok(Math.abs(held - 220) < 3, `held ${held} Hz`);
  assert.ok(peak(seconds(left, 0.4, 0.5)) > 0.1);
});

test('unison spread splits the voices between left and right', () => {
  const play = (spread: number) => {
    const s = sineSynth();
    s.set('unison_voices', 3);
    s.set('unison_detune', 30);
    s.set('unison_spread', spread);
    s.engine.midi(noteOn, 57, 100);
    return s.play(0.3);
  };

  const centred = play(0);
  assert.deepEqual(centred.left, centred.right);

  const spread = play(100);
  const difference = spread.left.map((sample, i) => sample - spread.right[i]!);
  assert.ok(peak(difference) > 0.05, `largest L-R ${peak(difference)}`);
});

test('the lowpass filter takes out the highs, more as the cutoff falls', () => {
  const play = (cutoff: number) => {
    const s = synth();
    s.set('osc1_waveform', 1);
    s.set('filter_resonance', 0);
    s.set('filter_cutoff', cutoff);
    s.engine.midi(noteOn, 45, 100);
    return roughness(seconds(s.play(0.4).left, 0.1, 0.4));
  };

  const open = play(20000);
  const closed = play(200);
  assert.ok(closed < open / 4, `roughness ${closed} at 200 Hz, ${open} at 20 kHz`);
});

test('drive saturates, and stays finite with HQ oversampling', () => {
  const play = (drive: number) => {
    const s = sineSynth();
    s.set('filter_enable', 1);
    s.set('filter_cutoff', 20000);
    s.set('filter_resonance', 0);
    s.set('drive', drive);
    s.set('drive_hq', 1);
    s.engine.midi(noteOn, 57, 100);
    return seconds(s.play(0.4).left, 0.1, 0.4);
  };

  const clean = play(0);
  const driven = play(1);

  assert.ok(driven.every(Number.isFinite));
  assert.ok(rms(driven) > rms(clean) * 1.2, `rms ${rms(driven)} driven, ${rms(clean)} clean`);
  assert.ok(peak(driven) < 1.1, `peak ${peak(driven)}`);
});

test('an LFO on oscillator gain moves the level, and clearing it stops that', () => {
  const s = sineSynth();
  s.set('lfo1_mode', 1);
  s.set('lfo1_rate', 4);
  s.modulate('osc1_gain', 'lfo', 0, 0, 1);
  s.engine.midi(noteOn, 57, 100);
  s.play(0.2);

  const modulated = windowRms(s.play(1).left, 0.025);
  assert.ok(Math.max(...modulated) > 3 * Math.min(...modulated), `levels ${Math.min(...modulated)} to ${Math.max(...modulated)}`);

  s.clearModulation('osc1_gain');
  s.play(0.1);

  const steady = windowRms(s.play(0.5).left, 0.025);
  assert.ok(Math.max(...steady) < 1.05 * Math.min(...steady), `levels ${Math.min(...steady)} to ${Math.max(...steady)}`);
});

test('an envelope on the cutoff opens the filter as it rises', () => {
  const s = synth();
  s.set('osc1_waveform', 1);
  s.set('filter_resonance', 0);
  s.set('env2_attack', 1);
  s.set('env2_sustain', 1);
  s.modulate('filter_cutoff', 'env', 1, 0.1, 0.9);
  s.engine.midi(noteOn, 45, 100);

  const { left } = s.play(1.1);
  const early = roughness(seconds(left, 0.05, 0.15));
  const late = roughness(seconds(left, 0.95, 1.05));

  assert.ok(late > 2 * early, `roughness ${early} early, ${late} late`);
});

test('a drawn curve and its tension shape the LFO', () => {
  const s = synth();
  s.set('lfo1_mode', 1);
  s.modulate('filter_cutoff', 'lfo', 0, 0, 1);

  s.curve(0, [[0, 0.9], [1, 0.9]]);
  s.play(0.1);
  assert.ok(Math.abs(s.live().lfoValue[0]! - 0.9) < 1e-6, `flat curve ${s.live().lfoValue[0]}`);

  const defaultPoints: CurvePoint[] = [[0, 0.5], [0.25, 1], [0.5, 0.5], [0.75, 0], [1, 0.5]];
  s.curve(0, []);
  s.set('lfo1_tension', 1);
  s.play(0.1);

  const { lfoPhase, lfoValue } = s.live();
  s.writePoints(defaultPoints);
  assert.ok(Math.abs(lfoValue[0]! - s.engine.curveValue(defaultPoints.length, 1, lfoPhase[0])) < 1e-3);
  assert.ok(Math.abs(lfoValue[0]! - s.engine.curveValue(defaultPoints.length, 0.5, lfoPhase[0])) > 1e-2);
});

// LFO 1 free-running at a quarter note, on the cutoff
function syncedLfoSynth() {
  const s = synth();
  s.set('lfo1_mode', 1);
  s.set('lfo1_sync', 1);
  s.set('lfo1_sync_rate', 6);
  s.modulate('filter_cutoff', 'lfo', 0, 0, 1);
  return s;
}

test('a synced LFO runs at the tempo', () => {
  for (const [bpm, expectedPhase] of [[120, 0.5], [60, 0.25]] as const) {
    const s = syncedLfoSynth();
    s.engine.setTransport(bpm, false, 0);
    s.play(0.25);

    assert.ok(Math.abs(s.live().lfoPhase[0]! - expectedPhase) < 0.01, `${bpm} BPM: phase ${s.live().lfoPhase[0]}`);
  }
});

test('a synced LFO follows the host position while it plays', () => {
  const s = syncedLfoSynth();
  s.engine.setTransport(120, true, 0.25);
  s.play(0.25);

  // Half a beat on from a quarter
  assert.ok(Math.abs(s.live().lfoPhase[0]! - 0.75) < 0.01, `phase ${s.live().lfoPhase[0]}`);
});

test('master gain scales the output', () => {
  const play = (gain: number) => {
    const s = sineSynth();
    s.set('master_gain', gain);
    s.engine.midi(noteOn, 57, 100);
    return peak(s.play(0.3).left);
  };

  assert.ok(Math.abs(play(-6) / play(0) - 10 ** (-6 / 20)) < 1e-3);
});

test('the meter holds the loudest output sample until read', () => {
  const s = sineSynth();
  s.engine.midi(noteOn, 57, 100);
  s.engine.render(blockSize);

  assert.equal(s.engine.peak(0), Math.fround(peak(s.view(0, blockSize))));
  assert.equal(s.engine.peak(0), 0);
});

//==============================================================================
interface Golden {
  sampleRate: number;
  blockSize: number;
  blocksPerFrame: number;
  scenarios: {
    name: string;
    blocks: number;
    params: Record<string, number>;
    modulation: { param: string; source: ModulationSource; index: number; min: number; max: number }[];
    curves: { lfo: number; points: CurvePoint[] }[];
    notes: { block: number; note: number; velocity: number }[];
    fingerprint: number[][];
  }[];
}

// Emscripten's tan, exp and log round differently in the last bit from macOS's, and the
// resonant bandpass in the second scenario magnifies that to about 1e-3
test('renders the same as the plugin, to within libm rounding', () => {
  const golden = JSON.parse(readFileSync(new URL('../../Tests/fixtures/golden.json', import.meta.url), 'utf8')) as Golden;
  const { blockSize: size, blocksPerFrame } = golden;

  for (const scenario of golden.scenarios) {
    const s = synth(null);

    for (const [id, value] of Object.entries(scenario.params)) s.set(id, value);
    for (const { param, source, index, min, max } of scenario.modulation) s.modulate(param, source, index, min, max);
    for (const { lfo, points } of scenario.curves) s.curve(lfo, points);

    s.engine.prepare(golden.sampleRate, 2);

    const fingerprint: number[][] = [];
    let sumOfSquares = [0, 0];
    let peaks = [0, 0];

    for (let block = 0; block < scenario.blocks; block++) {
      for (const { block: at, note, velocity } of scenario.notes) {
        if (at === block) s.engine.midi(velocity > 0 ? noteOn : noteOff, note, velocity);
      }

      s.engine.render(size);

      for (const channel of [0, 1]) {
        for (const sample of s.view(channel, size)) {
          sumOfSquares[channel]! += Math.fround(sample * sample);
          peaks[channel] = Math.max(peaks[channel]!, Math.abs(sample));
        }
      }

      if ((block + 1) % blocksPerFrame === 0) {
        const frameRms = (channel: number) => Math.sqrt(sumOfSquares[channel]! / (size * blocksPerFrame));
        fingerprint.push([frameRms(0), frameRms(1), peaks[0]!, peaks[1]!]);
        sumOfSquares = [0, 0];
        peaks = [0, 0];
      }
    }

    assert.equal(fingerprint.length, scenario.fingerprint.length, scenario.name);

    const firstMismatch = fingerprint.findIndex((frame, i) =>
      frame.some((value, j) => Math.abs(value - scenario.fingerprint[i]![j]!) > 1e-5 + 5e-3 * Math.abs(scenario.fingerprint[i]![j]!)));

    assert.equal(firstMismatch, -1, `${scenario.name}: frame ${firstMismatch} is ${fingerprint[firstMismatch]}, the plugin's ${scenario.fingerprint[firstMismatch]}`);
  }
});
