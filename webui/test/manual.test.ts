import assert from 'node:assert/strict';
import { test } from 'node:test';
import { parseManual } from '../src/manual.ts';

// The same text and blocks as Tests/ManualTests.cpp
test('the manual parses headings, bullets and paragraphs', () => {
  const blocks = parseManual('# Title\r\n\r\nSome text\r\nover two lines.\r\n## Section\n- One\n  wrapped\n- Two\n\n#NotAHeading\n');

  assert.deepEqual(blocks, [
    { kind: 'heading', text: 'Title' },
    { kind: 'paragraph', text: 'Some text over two lines.' },
    { kind: 'subheading', text: 'Section' },
    { kind: 'bullet', text: 'One wrapped' },
    { kind: 'bullet', text: 'Two' },
    { kind: 'paragraph', text: '#NotAHeading' },
  ]);
});
