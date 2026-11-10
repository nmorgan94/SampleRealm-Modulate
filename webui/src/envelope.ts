// Mirrors Source/dsp/EnvelopeCurve.h and the shape Source/ui/EnvelopeEditor.h draws
import { clamp01 } from './params.ts';

const SUSTAIN_DISPLAY_TIME = 0.3;
const CURVE_PER_PIXEL = 1 / 120;
const SEGMENT_STEPS = 32;
// Towards each segment's end
const DRAG_DIRECTIONS = [-1, 1, 1] as const;

export const shapeProgress = (progress: number, curve: number) => {
  if (progress <= 0 || progress >= 1 || Math.abs(curve) < 0.001) return clamp01(progress);
  const steepness = 1 + Math.abs(curve) * 19;
  return curve > 0 ? Math.log(1 + progress * (steepness - 1)) / Math.log(steepness) : (steepness ** progress - 1) / (steepness - 1);
};

type Envelope = {
  attack: number;
  decay: number;
  sustain: number;
  release: number;
  curves: readonly [number, number, number];
};

type Position = { x: number; y: number };

// Attack, decay and release
type Segment = { from: Position; to: Position; curve: number };

const segmentPoint = ({ from, to, curve }: Segment, t: number): Position => ({
  x: from.x + (to.x - from.x) * t,
  y: from.y + (to.y - from.y) * shapeProgress(t, curve),
});

export const draggedCurve = (index: number, startCurve: number, dy: number) => startCurve + dy * DRAG_DIRECTIONS[index]! * CURVE_PER_PIXEL;

export function envelopeShape({ attack, decay, sustain, release, curves }: Envelope, width: number, height: number) {
  const [a, d, s, r] = [Math.max(0.001, attack), Math.max(0.001, decay), clamp01(sustain), Math.max(0.001, release)];
  const total = a + d + SUSTAIN_DISPLAY_TIME + r;
  const attackX = (a / total) * width;
  const decayX = attackX + (d / total) * width;
  const sustainX = decayX + (SUSTAIN_DISPLAY_TIME / total) * width;
  const sustainY = height - s * height;

  const segments: Segment[] = [
    { from: { x: 0, y: height }, to: { x: attackX, y: 0 }, curve: curves[0] },
    { from: { x: attackX, y: 0 }, to: { x: decayX, y: sustainY }, curve: curves[1] },
    { from: { x: sustainX, y: sustainY }, to: { x: width, y: height }, curve: curves[2] },
  ];

  const path = segments
    .flatMap((segment) => Array.from({ length: SEGMENT_STEPS + 1 }, (_, i) => segmentPoint(segment, i / SEGMENT_STEPS)))
    .map(({ x, y }, i) => `${i === 0 ? 'M' : 'L'} ${x} ${y}`)
    .join(' ');

  return { attackX, decayX, sustainX, sustainY, path, handles: segments.map((segment) => segmentPoint(segment, 0.5)) };
}
