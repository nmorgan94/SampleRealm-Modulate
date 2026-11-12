import { webuiConfig } from '@samplerealm/modules/build/webuiConfig.js';
import tailwindcss from '@tailwindcss/vite';
import react from '@vitejs/plugin-react';
import { defineConfig } from 'vite';
import { localEnginePath } from './localEngine.ts';

export default defineConfig(({ mode }) =>
  webuiConfig({ mode, webuiDir: import.meta.dirname, plugins: [react(), tailwindcss()], enginePath: localEnginePath }),
);
