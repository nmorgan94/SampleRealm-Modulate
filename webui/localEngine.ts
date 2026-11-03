import { existsSync } from 'node:fs';
import { resolve } from 'node:path';

const ENGINE = resolve(import.meta.dirname, '../wasm/build-wasm/engine.wasm');

export const localEnginePath = () => {
  if (!existsSync(ENGINE)) throw new Error('Build wasm/ first: cmake --workflow --preset wasm');
  return ENGINE;
};
