import assert from 'node:assert/strict';
import { test } from 'node:test';
import { aboutText } from '../src/about.ts';

test('about shows the version and build type, the commit, where it runs, and the UI', () => {
  assert.equal(
    aboutText({ version: '1.2.3', build: 'Release', commit: 'abc1234-local' }, 'VST3 in Ableton Live'),
    'Version 1.2.3\nRelease build\nCommit abc1234-local\nVST3 in Ableton Live\nWebView UI',
  );
});
