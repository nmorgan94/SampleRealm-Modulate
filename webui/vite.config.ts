import { readFileSync } from 'node:fs';
import { resolve } from 'node:path';
import tailwindcss from '@tailwindcss/vite';
import react from '@vitejs/plugin-react';
import { defineConfig, type Plugin } from 'vite';
import { localEnginePath } from './localEngine.ts';

const TARGETS = ['plugin', 'demo'];

// The engine `cmake --workflow --preset wasm` last built
const localEngine = (): Plugin => ({
  name: 'local-engine',
  configureServer(server) {
    server.middlewares.use('/engine.wasm', (_, res) => {
      res.setHeader('Content-Type', 'application/wasm');
      res.end(readFileSync(localEnginePath()));
    });
  },
});

export default defineConfig(({ mode }) => {
  if (!TARGETS.includes(mode)) throw new Error(`Pass --mode ${TARGETS.join(' or ')}`);

  return {
    root: resolve(import.meta.dirname, 'src', mode),
    publicDir: resolve(import.meta.dirname, 'public', mode),
    // Loaded from the plugin's zip and from the CDN's demo folder
    base: './',
    plugins: [react(), tailwindcss(), localEngine()],
    server: { port: 8099, fs: { allow: [resolve(import.meta.dirname, '..')] } },
    build: {
      outDir: resolve(import.meta.dirname, 'dist', mode),
      emptyOutDir: true,
      assetsInlineLimit: 0,
      modulePreload: { polyfill: false },
      rolldownOptions: {
        output: {
          entryFileNames: 'editor.js',
          assetFileNames: ({ names }) => (names[0]?.endsWith('.css') ? 'editor.css' : '[name][extname]'),
        },
      },
    },
  };
});
