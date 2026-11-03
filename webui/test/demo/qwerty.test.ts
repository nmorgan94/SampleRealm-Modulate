import assert from 'node:assert/strict';
import { test } from 'node:test';
import { qwertyLabel, qwertySemitone } from '../../src/demo/qwerty.ts';

test('the home row plays the white keys of an octave from C, the row above the black keys', () => {
  assert.deepEqual(['KeyA', 'KeyS', 'KeyD', 'KeyF', 'KeyG', 'KeyH', 'KeyJ', 'KeyK'].map(qwertySemitone), [0, 2, 4, 5, 7, 9, 11, 12]);
  assert.deepEqual(['KeyW', 'KeyE', 'KeyT', 'KeyY', 'KeyU'].map(qwertySemitone), [1, 3, 6, 8, 10]);
});

test('keys outside the map play nothing, and notes past it have no label', () => {
  assert.equal(qwertySemitone('KeyZ'), undefined);
  assert.equal(qwertyLabel(24), undefined);
});
