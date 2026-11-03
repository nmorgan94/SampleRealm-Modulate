import assert from 'node:assert/strict';
import { test } from 'node:test';
import { filterMagnitude } from '../src/filterResponse.ts';
import { engine } from './engine.ts';

test("the response curve is the filter's, for every mode and slope", () => {
  for (const mode of [0, 1, 2, 3]) {
    for (const slope of [0, 1]) {
      for (const [cutoff, resonance] of [[1000, 0.5], [80, 0], [12000, 1], [20000, 0.3]]) {
        for (const frequency of [20, 63, 250, 1000, 4000, 9000, 20000]) {
          const expected = engine.filterMagnitude(mode, slope, cutoff, resonance, frequency, 48000);
          const actual = filterMagnitude(mode, slope, cutoff!, resonance!, frequency, 48000);
          const label = `mode ${mode} slope ${slope} cutoff ${cutoff} resonance ${resonance} at ${frequency} Hz`;
          // The floor is -80 dB, under the curve's -60 dB bottom, where a notch's float and double nulls differ
          assert.ok(Math.abs(actual - expected) <= 1e-3 * expected + 1e-4, `${label}: ${actual}, expected ${expected}`);
        }
      }
    }
  }
});
