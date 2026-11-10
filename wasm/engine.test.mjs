import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
import { test } from 'node:test';
import { localEnginePath } from '../webui/localEngine.ts';
import { readUiInfo, stubImports } from '../webui/src/demo/workletBackend.ts';

const sampleRate = 48000;
const blockSize = 128;
const noteOn = 0x90;
const noteOff = 0x80;
const sources = { lfo: 1, env: 2 };

const wasm = new WebAssembly.Module(readFileSync(localEnginePath()));

function engine({ prepared = true } = {}) {
  const { exports } = new WebAssembly.Instance(wasm, stubImports());
  exports._initialize?.();

  const ids = readUiInfo(exports).params.map((p) => p.id);
  const indexOf = (id) => {
    assert.ok(ids.includes(id), `no parameter ${id}`);
    return ids.indexOf(id);
  };

  const set = (id, value) => exports.setParam(indexOf(id), value);
  const modulate = (id, source, index, min, max) => exports.setModulation(indexOf(id), sources[source] ?? 0, index, min, max);
  const clearModulation = (id) => exports.setModulation(indexOf(id), 0, 0, 0, 0);

  const writePoints = (points) => new Float32Array(exports.memory.buffer, exports.points(), points.length * 2).set(points.flat());
  const curve = (lfo, points) => {
    writePoints(points);
    exports.setCurve(lfo, points.length);
  };

  const channelOut = (channel, frames) => new Float32Array(exports.memory.buffer, exports.channel(channel), frames);

  const play = (seconds) => {
    const left = [];
    const right = [];
    for (let n = 0; n < Math.round((seconds * sampleRate) / blockSize); n++) {
      exports.render(blockSize);
      left.push(...channelOut(0, blockSize));
      right.push(...channelOut(1, blockSize));
    }
    return { left, right };
  };

  const live = () => {
    const values = Array.from(new Float32Array(exports.memory.buffer, exports.live(), 16));
    return { lfoPhase: values.slice(0, 4), lfoValue: values.slice(4, 8), envelopeLevel: values.slice(8, 12) };
  };

  if (prepared) exports.prepare(sampleRate);

  return { ...exports, indexOf, set, modulate, clearModulation, writePoints, curve, channelOut, play, live };
}

// One sine on oscillator 1, straight to the output
function sineEngine() {
  const e = engine();
  e.set('filter_enable', 0);
  return e;
}

const peak = (samples) => samples.reduce((largest, sample) => Math.max(largest, Math.abs(sample)), 0);

const rms = (samples) => Math.sqrt(samples.reduce((sum, sample) => sum + sample * sample, 0) / samples.length);

// RMS of the sample-to-sample change: larger for brighter sounds
const roughness = (samples) => rms(samples.slice(1).map((sample, i) => sample - samples[i]));

const frequency = (samples) => {
  let rising = 0;
  for (let i = 1; i < samples.length; i++) if (samples[i - 1] < 0 && samples[i] >= 0) rising++;
  return rising / (samples.length / sampleRate);
};

const seconds = (samples, from, to) => samples.slice(Math.round(from * sampleRate), Math.round(to * sampleRate));

const windowRms = (samples, windowSeconds) => {
  const size = Math.round(windowSeconds * sampleRate);
  const windows = [];
  for (let start = 0; start + size <= samples.length; start += size) windows.push(rms(samples.slice(start, start + size)));
  return windows;
};

//==============================================================================
test('the ui info carries the version, the commit, the build type and every parameter as ParameterSpecs defines it', () => {
  const info = readUiInfo(engine());

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
  assert.equal(waveforms.kind, 'choice');
  assert.equal(waveforms.defaultValue, 1);
  assert.equal(waveforms.choices.length, 21);
  assert.equal(waveforms.choices.at(-1), 'Trapezoid');
});

test('a note plays at its MIDI pitch, moved by oscillator pitch, fine tune and coarse pitch', () => {
  for (const [id, value, expected] of [
    ['osc1_pitch', 0, 440],
    ['osc1_pitch', 12, 880],
    ['osc1_fine', 50, 440 * 2 ** (50 / 1200)],
    ['coarse_pitch', -12, 220],
  ]) {
    const e = sineEngine();
    e.set(id, value);
    e.midi(noteOn, 69, 100);

    const measured = frequency(e.play(0.5).left);
    assert.ok(Math.abs(measured - expected) < 3, `${id} ${value}: ${measured} Hz`);
  }
});

test('a release curve bends the fade: positive falls fast early, negative slowly', () => {
  const halfwayPeak = (curve) => {
    const e = sineEngine();
    e.set('env1_sustain', 1);
    e.set('env1_release', 0.4);
    e.set('env1_release_curve', curve);
    e.midi(noteOn, 57, 100);
    e.play(0.3);
    e.midi(noteOff, 57, 0);
    return peak(seconds(e.play(0.4).left, 0.19, 0.21));
  };

  const [linear, fast, slow] = [0, 0.8, -0.8].map(halfwayPeak);
  assert.ok(fast < linear * 0.6, `${fast} vs ${linear}`);
  assert.ok(slow > linear * 1.4, `${slow} vs ${linear}`);
});

test('a held note follows the sustain as it is turned', () => {
  const e = sineEngine();
  e.set('env1_sustain', 1);
  e.midi(noteOn, 57, 100);
  e.play(0.3);
  const full = peak(e.play(0.05).left);

  e.set('env1_sustain', 0.5);
  e.play(0.01);

  const half = peak(e.play(0.05).left);
  assert.ok(Math.abs(half / full - 0.5) < 0.01, `${half} vs ${full}`);
});

test('changing the release while it plays carries on from the same level, at the new speed', () => {
  const e = sineEngine();
  e.set('env1_sustain', 1);
  e.set('env1_release', 1);
  e.midi(noteOn, 57, 100);
  e.play(0.3);
  e.midi(noteOff, 57, 0);
  e.play(0.3);

  const before = e.live().envelopeLevel[0];
  e.set('env1_release', 0.2);
  e.set('env1_release_curve', 0.8);
  e.play(blockSize / sampleRate);
  const after = e.live().envelopeLevel[0];

  assert.ok(after < before && before - after < 0.05, `${before} to ${after}`);
  assert.equal(peak(seconds(e.play(0.3).left, 0.2, 0.3)), 0);
});

test('releasing a note fades it to silence within the release time', () => {
  const e = sineEngine();
  e.set('env1_release', 0.2);
  e.midi(noteOn, 57, 100);
  e.play(0.3);
  e.midi(noteOff, 57, 0);

  const { left } = e.play(0.4);

  assert.ok(peak(left.slice(0, blockSize)) > 0.1);
  assert.equal(peak(seconds(left, 0.25, 0.4)), 0);
});

test('with one voice, a second held note glides there from the first', () => {
  const e = sineEngine();
  e.set('num_voices', 1);
  e.set('glide_time', 0.4);
  e.midi(noteOn, 57, 100);
  e.play(0.2);
  e.midi(noteOn, 69, 100);

  const midGlide = frequency(seconds(e.play(0.4).left, 0.15, 0.25));
  const settled = frequency(e.play(0.5).left);

  assert.ok(midGlide > 240 && midGlide < 420, `mid-glide ${midGlide} Hz`);
  assert.ok(Math.abs(settled - 440) < 3, `settled ${settled} Hz`);
});

test('in Rate mode the glide time is per octave, and in Time mode it is per slide', () => {
  const glideFrom45 = (mode, note) => {
    const e = sineEngine();
    e.set('num_voices', 1);
    e.set('glide_time', 0.2);
    e.set('glide_mode', mode);
    e.midi(noteOn, 45, 100);
    e.play(0.1);
    e.midi(noteOn, note, 100);
    return frequency(seconds(e.play(0.45).left, 0.25, 0.45));
  };

  const rateOctave = glideFrom45(1, 57);
  const rateTwoOctaves = glideFrom45(1, 69);
  const timeTwoOctaves = glideFrom45(0, 69);

  assert.ok(Math.abs(rateOctave - 220) < 10, `Rate, one octave ${rateOctave} Hz`);
  assert.ok(rateTwoOctaves > 330 && rateTwoOctaves < 400, `Rate, two octaves ${rateTwoOctaves} Hz`);
  assert.ok(Math.abs(timeTwoOctaves - 440) < 10, `Time, two octaves ${timeTwoOctaves} Hz`);
});

test('a note played mid-glide slides on from where the pitch is', () => {
  const e = sineEngine();
  e.set('num_voices', 1);
  e.set('glide_time', 0.4);
  e.set('glide_mode', 0);
  e.midi(noteOn, 45, 100);
  e.play(0.1);
  e.midi(noteOn, 69, 100);
  e.play(0.2);
  e.midi(noteOn, 64, 100);

  const start = frequency(e.play(0.1).left);

  assert.ok(Math.abs(start - 231) < 15, `start ${start} Hz`);
});

test('with one voice, a note played legato keeps the envelope going', () => {
  const e = sineEngine();
  e.set('num_voices', 1);
  e.set('env1_attack', 1);
  e.midi(noteOn, 57, 100);
  const before = peak(e.play(0.5).left.slice(-8 * blockSize));
  e.midi(noteOn, 64, 100);
  const after = peak(e.play(0.02).left);

  assert.ok(after > 0.9 * before, `${before} before, ${after} after`);
});

test('with one voice, letting go of a key returns to the one still held', () => {
  const e = sineEngine();
  e.set('num_voices', 1);
  e.midi(noteOn, 57, 100);
  e.play(0.1);
  e.midi(noteOn, 69, 100);
  e.play(0.1);
  e.midi(noteOff, 69, 0);

  const { left } = e.play(0.5);
  const held = frequency(seconds(left, 0.1, 0.5));

  assert.ok(Math.abs(held - 220) < 3, `held ${held} Hz`);
  assert.ok(peak(seconds(left, 0.4, 0.5)) > 0.1);
});

test('unison spread splits the voices between left and right', () => {
  const play = (spread) => {
    const e = sineEngine();
    e.set('unison_voices', 3);
    e.set('unison_detune', 30);
    e.set('unison_spread', spread);
    e.midi(noteOn, 57, 100);
    return e.play(0.3);
  };

  const centred = play(0);
  assert.deepEqual(centred.left, centred.right);

  const spread = play(100);
  const difference = spread.left.map((sample, i) => sample - spread.right[i]);
  assert.ok(peak(difference) > 0.05, `largest L-R ${peak(difference)}`);
});

test('the lowpass filter takes out the highs, more as the cutoff falls', () => {
  const play = (cutoff) => {
    const e = engine();
    e.set('osc1_waveform', 1);
    e.set('filter_resonance', 0);
    e.set('filter_cutoff', cutoff);
    e.midi(noteOn, 45, 100);
    return roughness(seconds(e.play(0.4).left, 0.1, 0.4));
  };

  const open = play(20000);
  const closed = play(200);
  assert.ok(closed < open / 4, `roughness ${closed} at 200 Hz, ${open} at 20 kHz`);
});

test('drive saturates, and stays finite with HQ oversampling', () => {
  const play = (drive) => {
    const e = sineEngine();
    e.set('filter_enable', 1);
    e.set('filter_cutoff', 20000);
    e.set('filter_resonance', 0);
    e.set('drive', drive);
    e.set('drive_hq', 1);
    e.midi(noteOn, 57, 100);
    return seconds(e.play(0.4).left, 0.1, 0.4);
  };

  const clean = play(0);
  const driven = play(1);

  assert.ok(driven.every(Number.isFinite));
  assert.ok(rms(driven) > rms(clean) * 1.2, `rms ${rms(driven)} driven, ${rms(clean)} clean`);
  assert.ok(peak(driven) < 1.1, `peak ${peak(driven)}`);
});

test('an LFO on oscillator gain moves the level, and clearing it stops that', () => {
  const e = sineEngine();
  e.set('lfo1_mode', 1);
  e.set('lfo1_rate', 4);
  e.modulate('osc1_gain', 'lfo', 0, 0, 1);
  e.midi(noteOn, 57, 100);
  e.play(0.2);

  const modulated = windowRms(e.play(1).left, 0.025);
  assert.ok(Math.max(...modulated) > 3 * Math.min(...modulated), `levels ${Math.min(...modulated)} to ${Math.max(...modulated)}`);

  e.clearModulation('osc1_gain');
  e.play(0.1);

  const steady = windowRms(e.play(0.5).left, 0.025);
  assert.ok(Math.max(...steady) < 1.05 * Math.min(...steady), `levels ${Math.min(...steady)} to ${Math.max(...steady)}`);
});

test('an envelope on the cutoff opens the filter as it rises', () => {
  const e = engine();
  e.set('osc1_waveform', 1);
  e.set('filter_resonance', 0);
  e.set('env2_attack', 1);
  e.set('env2_sustain', 1);
  e.modulate('filter_cutoff', 'env', 1, 0.1, 0.9);
  e.midi(noteOn, 45, 100);

  const { left } = e.play(1.1);
  const early = roughness(seconds(left, 0.05, 0.15));
  const late = roughness(seconds(left, 0.95, 1.05));

  assert.ok(late > 2 * early, `roughness ${early} early, ${late} late`);
});

test('a drawn curve and its tension shape the LFO', () => {
  const e = engine();
  e.set('lfo1_mode', 1);
  e.modulate('filter_cutoff', 'lfo', 0, 0, 1);

  e.curve(0, [[0, 0.9], [1, 0.9]]);
  e.play(0.1);
  assert.ok(Math.abs(e.live().lfoValue[0] - 0.9) < 1e-6, `flat curve ${e.live().lfoValue[0]}`);

  const defaultPoints = [[0, 0.5], [0.25, 1], [0.5, 0.5], [0.75, 0], [1, 0.5]];
  e.curve(0, []);
  e.set('lfo1_tension', 1);
  e.play(0.1);

  const { lfoPhase, lfoValue } = e.live();
  e.writePoints(defaultPoints);
  assert.ok(Math.abs(lfoValue[0] - e.curveValue(defaultPoints.length, 1, lfoPhase[0])) < 1e-3);
  assert.ok(Math.abs(lfoValue[0] - e.curveValue(defaultPoints.length, 0.5, lfoPhase[0])) > 1e-2);
});

// LFO 1 free-running at a quarter note, on the cutoff
function syncedLfoEngine() {
  const e = engine();
  e.set('lfo1_mode', 1);
  e.set('lfo1_sync', 1);
  e.set('lfo1_sync_rate', 6);
  e.modulate('filter_cutoff', 'lfo', 0, 0, 1);
  return e;
}

test('a synced LFO runs at the tempo', () => {
  for (const [bpm, expectedPhase] of [[120, 0.5], [60, 0.25]]) {
    const e = syncedLfoEngine();
    e.setTransport(bpm, false, 0);
    e.play(0.25);

    assert.ok(Math.abs(e.live().lfoPhase[0] - expectedPhase) < 0.01, `${bpm} BPM: phase ${e.live().lfoPhase[0]}`);
  }
});

test('a synced LFO follows the host position while it plays', () => {
  const e = syncedLfoEngine();
  e.setTransport(120, true, 0.25);
  e.play(0.25);

  // Half a beat on from a quarter
  assert.ok(Math.abs(e.live().lfoPhase[0] - 0.75) < 0.01, `phase ${e.live().lfoPhase[0]}`);
});

test('master gain scales the output', () => {
  const play = (gain) => {
    const e = sineEngine();
    e.set('master_gain', gain);
    e.midi(noteOn, 57, 100);
    return peak(e.play(0.3).left);
  };

  assert.ok(Math.abs(play(-6) / play(0) - 10 ** (-6 / 20)) < 1e-3);
});

test('the meter holds the loudest output sample until read', () => {
  const e = sineEngine();
  e.midi(noteOn, 57, 100);
  e.render(blockSize);

  assert.equal(e.peak(0), Math.fround(peak(e.channelOut(0, blockSize))));
  assert.equal(e.peak(0), 0);
});

//==============================================================================
// Emscripten's tan, exp and log round differently in the last bit from macOS's, and the
// resonant bandpass in the second scenario magnifies that to about 1e-3
test('renders the same as the plugin, to within libm rounding', () => {
  const golden = JSON.parse(readFileSync(new URL('../Tests/fixtures/golden.json', import.meta.url), 'utf8'));
  const { blockSize: size, blocksPerFrame } = golden;

  for (const scenario of golden.scenarios) {
    const e = engine({ prepared: false });

    for (const [id, value] of Object.entries(scenario.params)) e.set(id, value);
    for (const { param, source, index, min, max } of scenario.modulation) e.modulate(param, source, index, min, max);
    for (const { lfo, points } of scenario.curves) e.curve(lfo, points);

    e.prepare(golden.sampleRate);

    const fingerprint = [];
    let sumOfSquares = [0, 0];
    let peaks = [0, 0];

    for (let block = 0; block < scenario.blocks; block++) {
      for (const { block: at, note, velocity } of scenario.notes) {
        if (at === block) e.midi(velocity > 0 ? noteOn : noteOff, note, velocity);
      }

      e.render(size);

      for (const channel of [0, 1]) {
        for (const sample of e.channelOut(channel, size)) {
          sumOfSquares[channel] += Math.fround(sample * sample);
          peaks[channel] = Math.max(peaks[channel], Math.abs(sample));
        }
      }

      if ((block + 1) % blocksPerFrame === 0) {
        const frameRms = (channel) => Math.sqrt(sumOfSquares[channel] / (size * blocksPerFrame));
        fingerprint.push([frameRms(0), frameRms(1), peaks[0], peaks[1]]);
        sumOfSquares = [0, 0];
        peaks = [0, 0];
      }
    }

    assert.equal(fingerprint.length, scenario.fingerprint.length, scenario.name);

    const firstMismatch = fingerprint.findIndex((frame, i) =>
      frame.some((value, j) => Math.abs(value - scenario.fingerprint[i][j]) > 1e-5 + 5e-3 * Math.abs(scenario.fingerprint[i][j])));

    assert.equal(firstMismatch, -1, `${scenario.name}: frame ${firstMismatch} is ${fingerprint[firstMismatch]}, the plugin's ${scenario.fingerprint[firstMismatch]}`);
  }
});
