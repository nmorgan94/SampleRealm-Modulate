// Mirrors Source/dsp/LfoCurve.h
import type { Point } from './backend.ts';
import { clamp01 } from './params.ts';

export const DEFAULT_POINTS: readonly Point[] = [
  { x: 0, y: 0.5 },
  { x: 0.25, y: 1 },
  { x: 0.5, y: 0.5 },
  { x: 0.75, y: 0 },
  { x: 1, y: 0.5 },
];

export const SHAPES: readonly { name: string; points: readonly Point[]; tension: number }[] = [
  { name: 'Sine', points: DEFAULT_POINTS, tension: 0.5 },
  { name: 'Triangle', points: DEFAULT_POINTS, tension: 1 },
  { name: 'Ramp Up', points: [{ x: 0, y: 0 }, { x: 1, y: 1 }], tension: 1 },
  { name: 'Ramp Down', points: [{ x: 0, y: 1 }, { x: 1, y: 0 }], tension: 1 },
  {
    name: 'Square',
    points: [{ x: 0, y: 1 }, { x: 0.49, y: 1 }, { x: 0.51, y: 0 }, { x: 0.99, y: 0 }, { x: 1, y: 1 }],
    tension: 1,
  },
];

export const catmullRom = (y0: number, y1: number, y2: number, y3: number, t: number, tension: number) => {
  const cardinal = (tension - 0.5) * 2;
  const scale = (1 - cardinal) * 0.5;
  const t2 = t * t;
  const t3 = t2 * t;

  const result =
    scale * (2 * y1 + (-y0 + y2) * t + (2 * y0 - 5 * y1 + 4 * y2 - y3) * t2 + (-y0 + 3 * y1 - 3 * y2 + y3) * t3) +
    cardinal * (y1 + (y2 - y1) * t);

  return clamp01(result);
};

export const evaluate = (points: readonly Point[], tension: number, at: number) => {
  const x = clamp01(at);
  if (points.length < 2) return 0.5;

  let segment = points.findIndex((_, i) => i < points.length - 1 && x <= points[i + 1]!.x);
  if (segment < 0) return points.at(-1)!.y;

  const start = points[segment]!;
  const end = points[segment + 1]!;
  const before = points[segment - 1] ?? start;
  const after = points[segment + 2] ?? end;

  // Points can share an x
  if (end.x <= start.x) return end.y;

  return catmullRom(before.y, start.y, end.y, after.y, (x - start.x) / (end.x - start.x), tension);
};

// An empty curve stands for the default shape
export const pointsOrDefault = (points: readonly Point[]) => (points.length === 0 ? DEFAULT_POINTS : points);
