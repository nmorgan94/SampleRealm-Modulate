import { engineAt } from '@samplerealm/modules/testing/engine.ts';
import { localEnginePath } from '../localEngine.ts';
import { paramLookup } from '../src/backend.ts';

// Makes a fresh engine from the last wasm build, stereo, prepared at `rate` or unprepared for null
export const freshEngine = engineAt(localEnginePath());

// One engine for the pure functions the web UI's ports are tested against
export const { engine, info } = freshEngine();

export const param = paramLookup(new Map(info.params.map((p) => [p.id, p])));

export const indexOf = (id: string) => info.params.findIndex((p) => p.id === id);

export const floats = (pointer: number, length: number) => Array.from(new Float32Array(engine.memory.buffer, pointer, length));

export const writePoints = (points: readonly { x: number; y: number }[]) =>
  new Float32Array(engine.memory.buffer, engine.points(), points.length * 2).set(points.flatMap(({ x, y }) => [x, y]));
