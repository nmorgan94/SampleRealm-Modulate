import assert from 'node:assert/strict';
import test from 'node:test';
import { layOutBelowLabel } from '../src/layout.ts';

test('a labelled knob puts its name along the top and centres the knob under it', () => {
  // FilterPanel's cutoff cell
  assert.deepEqual(layOutBelowLabel({ left: 80, top: 190, width: 70, height: 55 }), {
    label: { left: 80, top: 190, width: 70, height: 15 },
    control: { left: 95, top: 205, width: 40, height: 40 },
  });

  // VoicePanel's glide cell: an odd margin rounds down, as withSizeKeepingCentre does
  assert.deepEqual(layOutBelowLabel({ left: 120, top: 112, width: 130, height: 68 }).control, { left: 158, top: 127, width: 53, height: 53 });
});
