// Neither compiler sees across the bridge, so this checks both sides use the same names
import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
import { test } from 'node:test';

const cpp = readFileSync(new URL('../../../Source/webview/WebViewEditor.cpp', import.meta.url), 'utf8');
const page = readFileSync(new URL('../../src/plugin/juceBackend.ts', import.meta.url), 'utf8');

const names = (source: string, pattern: RegExp) => [...new Set([...source.matchAll(pattern)].map((match) => match[1]))].sort();

test('the page calls each native function the editor defines, and no others', () => {
  const defined = names(cpp, /withNativeFunction \("(\w+)"/g);
  assert.ok(defined.length > 0);
  assert.deepEqual(names(page, /call\('(\w+)'/g), defined);
});

test('the page listens for each event the editor sends, and no others', () => {
  const sent = names(cpp, /emitEventIfBrowserIsVisible \("(\w+)"/g);
  assert.ok(sent.length > 0);
  assert.deepEqual(names(page, /listen<.+?>\('(\w+)'/g), sent);
});

test('the page reads each piece of initialisation data the editor gives it, and no others', () => {
  const given = names(cpp, /withInitialisationData \("(\w+)"/g);
  assert.ok(given.length > 0);
  assert.deepEqual(names(page, /initial<.+?>\('(\w+)'\)/g), given);
});
