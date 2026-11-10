// Mirrors LevelMeter::Ballistics in Source/ui/LevelMeter.h
export const UPDATE_HZ = 30;
export const CLIP_HOLD_TICKS = 2 * UPDATE_HZ;
export const FLOOR_DB = -60;
export const FALL_DB_PER_TICK = 20 / UPDATE_HZ;

export interface Bar {
  levelDb: number;
  clipTicks: number;
}

export const emptyBar = (): Bar => ({ levelDb: FLOOR_DB, clipTicks: 0 });

export const isClipping = (bar: Bar) => bar.clipTicks > 0;

const gainToDecibels = (gain: number) => (gain > 0 ? Math.max(FLOOR_DB, 20 * Math.log10(gain)) : FLOOR_DB);

// Takes the peak since the last tick, as linear gain
export const updateBar = (bar: Bar, peak: number): Bar => {
  const clipTicks = peak > 1 ? CLIP_HOLD_TICKS : Math.max(0, bar.clipTicks - 1);
  // Capped at the top of the scale, so even an infinite peak can still fall away
  const levelDb = Math.min(0, Math.max(gainToDecibels(peak), bar.levelDb - FALL_DB_PER_TICK));
  return { levelDb, clipTicks };
};

export const barChanged = (before: Bar, after: Bar) =>
  Math.abs(before.levelDb - after.levelDb) > 1e-6 || isClipping(before) !== isClipping(after);
