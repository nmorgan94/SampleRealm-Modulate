// Mirrors Source/ui/About.cpp
import type { UiInfo } from './backend.ts';

export const aboutText = ({ version, build, commit }: Pick<UiInfo, 'version' | 'commit' | 'build'>, running: string) =>
  [`Version ${version}`, `${build} build`, `Commit ${commit}`, running, 'WebView UI'].join('\n');
