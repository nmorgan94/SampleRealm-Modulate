import assert from 'node:assert/strict';
import { test } from 'node:test';
import { generateWavetable, NUM_WAVEFORMS, WAVETABLE_SIZE } from '../src/waveforms.ts';
import { engine, floats, param } from './engine.ts';

test('there is a waveform for each choice', () => {
  assert.equal(NUM_WAVEFORMS, param('osc1_waveform').choices.length);
});

test('each waveform matches WavetableGenerator', () => {
  for (let type = 0; type < NUM_WAVEFORMS; type++) {
    const expected = floats(engine.waveform(type), WAVETABLE_SIZE);
    const actual = generateWavetable(type);
    const largest = expected.reduce((worst, sample, i) => Math.max(worst, Math.abs(sample - actual[i]!)), 0);
    assert.ok(largest < 1e-5, `${param('osc1_waveform').choices[type]}: off by ${largest}`);
  }
});
