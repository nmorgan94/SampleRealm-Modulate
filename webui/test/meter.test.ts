// The same cases as the Ballistics tests in Tests/MeterTests.cpp
import assert from 'node:assert/strict';
import { test } from 'node:test';
import { barChanged, CLIP_HOLD_TICKS, emptyBar, FLOOR_DB, isClipping, updateBar, UPDATE_HZ, type Bar } from '../src/meter.ts';

const near = (actual: number, expected: number, tolerance: number) =>
  assert.ok(Math.abs(actual - expected) <= tolerance, `${actual} is not within ${tolerance} of ${expected}`);

const tick = (bar: Bar, peak: number, times = 1) => {
  for (let i = 0; i < times; i++) bar = updateBar(bar, peak);
  return bar;
};

test('bars rise at once and fall smoothly', () => {
  let bar = emptyBar();
  assert.equal(bar.levelDb, FLOOR_DB);

  bar = tick(bar, 0.5);
  near(bar.levelDb, -6.02, 0.01);

  bar = tick(bar, 1);
  near(bar.levelDb, 0, 1e-6);

  // One second of silence falls 20 dB
  bar = tick(bar, 0, UPDATE_HZ);
  near(bar.levelDb, -20, 0.01);

  // Then it settles on the floor and stops asking for repaints
  bar = tick(bar, 0, 10 * UPDATE_HZ);
  assert.equal(bar.levelDb, FLOOR_DB);
  assert.equal(barChanged(bar, updateBar(bar, 0)), false);
});

test('the clip light holds after going over 0 dB', () => {
  for (const peak of [1.5, Infinity]) {
    let bar = emptyBar();
    assert.equal(isClipping(bar), false);

    bar = tick(bar, peak);
    assert.equal(isClipping(bar), true, `${peak}`);
    assert.equal(bar.levelDb, 0, 'capped at the top of the scale');

    bar = tick(bar, 0, CLIP_HOLD_TICKS - 1);
    assert.equal(isClipping(bar), true);
    assert.ok(bar.levelDb < 0, 'even an infinite peak falls away');

    const next = updateBar(bar, 0);
    assert.equal(barChanged(bar, next), true, 'the light going out needs a repaint');
    assert.equal(isClipping(next), false);
  }
});
