import assert from 'node:assert/strict';
import { test } from 'node:test';
import { bubbleAt } from '../src/bubble.ts';

const knob = (left: number, top: number, size = 44) => ({ left, top, width: size, height: size });

test('the master gain knob, top right, gets its bubble on the left', () => {
  assert.deepEqual(bubbleAt(knob(806, 13), 30, 20), { left: 806 - 10 - 30, top: 13 + 22 - 10 });
});

test('a knob near the left edge gets its bubble on the right', () => {
  assert.deepEqual(bubbleAt(knob(20, 300), 30, 20), { left: 20 + 44 + 10, top: 300 + 22 - 10 });
});

test('a knob in the middle at the bottom gets its bubble above', () => {
  assert.deepEqual(bubbleAt(knob(428, 540), 30, 20), { left: 428 + 22 - 15, top: 540 - 10 - 20 });
});
