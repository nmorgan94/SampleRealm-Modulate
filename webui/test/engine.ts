import { readFileSync } from 'node:fs';
import { localEnginePath } from '../localEngine.ts';
import { paramLookup } from '../src/backend.ts';
import { readUiInfo, stubImports, type Engine } from '../src/demo/workletBackend.ts';

const instance = new WebAssembly.Instance(new WebAssembly.Module(readFileSync(localEnginePath())), stubImports());

export const engine = instance.exports as Engine;
engine._initialize?.();

export const info = readUiInfo(engine);

export const param = paramLookup(new Map(info.params.map((p) => [p.id, p])));

export const indexOf = (id: string) => info.params.findIndex((p) => p.id === id);

export const floats = (pointer: number, length: number) => Array.from(new Float32Array(engine.memory.buffer, pointer, length));

export const writePoints = (points: readonly { x: number; y: number }[]) =>
  new Float32Array(engine.memory.buffer, engine.points(), points.length * 2).set(points.flatMap(({ x, y }) => [x, y]));
