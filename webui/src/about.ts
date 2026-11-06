// Mirrors Source/ui/About.cpp
import type { About } from './backend.ts';

export const aboutText = ({ version, build, commit, running }: About) =>
  [`Version ${version}`, `${build} build`, `Commit ${commit}`, running, 'WebView UI'].join('\n');
