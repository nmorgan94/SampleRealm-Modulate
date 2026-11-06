// Mirrors Manual::parse in Source/ui/Manual.cpp
export type BlockKind = 'heading' | 'subheading' | 'bullet' | 'paragraph';
export interface Block {
  kind: BlockKind;
  text: string;
}

const STARTS: [prefix: string, kind: BlockKind][] = [
  ['## ', 'subheading'],
  ['# ', 'heading'],
  ['- ', 'bullet'],
];

export function parseManual(markdown: string): Block[] {
  const blocks: Block[] = [];
  let canContinue = false;

  for (const rawLine of markdown.split(/\r\n|\r|\n/)) {
    const line = rawLine.trim();
    const start = STARTS.find(([prefix]) => line.startsWith(prefix));
    const last = blocks.at(-1);

    if (!line) {
      canContinue = false;
    } else if (start) {
      const [prefix, kind] = start;
      blocks.push({ kind, text: line.slice(prefix.length).trim() });
      canContinue = kind === 'bullet';
    } else if (canContinue && last) {
      last.text += ` ${line}`;
    } else {
      blocks.push({ kind: 'paragraph', text: line });
      canContinue = true;
    }
  }

  return blocks;
}
