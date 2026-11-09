// Mirrors ManualOverlay in Source/ui/Manual.cpp
import { useCallback, useRef, useState } from 'react';
import manualText from '../../Assets/Manual.md?raw';
import { TextButton } from './controls.tsx';
import { juceHeight } from './fonts.ts';
import { Modal, TITLE_HEIGHT } from './Modal.tsx';
import { parseManual, type Block, type BlockKind } from './manual.ts';

const BLOCKS = parseManual(manualText);

const PANEL = { width: 'calc(100% - 24px)', height: 'calc(100% - 24px)' };
const PADDING = { x: 16, y: 12 };
const CLOSE = { right: PADDING.x, top: PADDING.y, width: 70, height: TITLE_HEIGHT };
const SCROLLER = { left: PADDING.x, top: PADDING.y + TITLE_HEIGHT + 8, right: PADDING.x, bottom: PADDING.y };
// Room for LookAndFeel_V4's 8 pixel scroll bar and a gap
const SCROLL_BAR_ROOM = 8 + 4;
const BULLET_INDENT = 14;

const STYLES: Record<BlockKind, { height: number; bold: boolean; gapAbove: number }> = {
  heading: { height: 16, bold: true, gapAbove: 14 },
  subheading: { height: 14, bold: true, gapAbove: 14 },
  bullet: { height: 12, bold: false, gapAbove: 4 },
  paragraph: { height: 12, bold: false, gapAbove: 8 },
};

function Page({ blocks }: { blocks: Block[] }) {
  return (
    <div style={{ marginRight: SCROLL_BAR_ROOM }}>
      {blocks.map(({ kind, text }, i) => {
        const { height, bold, gapAbove } = STYLES[kind];
        const isBullet = kind === 'bullet';
        return (
          <div
            key={i}
            className={`relative ${bold ? 'font-bold' : ''}`}
            style={{ marginTop: i > 0 ? gapAbove : 0, paddingLeft: isBullet ? BULLET_INDENT : 0, fontSize: juceHeight(height), lineHeight: `${height}px` }}
          >
            {isBullet && <span className="absolute rounded-full bg-fg" style={{ left: 4, top: (height - 4) / 2, width: 4, height: 4 }} aria-hidden />}
            {text}
          </div>
        );
      })}
    </div>
  );
}

function Overlay({ close }: { close(): void }) {
  // Focused, so the arrow and page keys scroll it
  const scroller = useRef<HTMLDivElement>(null);

  return (
    <Modal label="Manual" titleInset={PADDING} panel={PANEL} onClose={close} focus={scroller}>
      <TextButton text="Close" style={CLOSE} onClick={close} />
      <div
        ref={scroller}
        tabIndex={-1}
        className="absolute overflow-y-auto outline-none [scrollbar-color:var(--color-scrollbar)_transparent] [scrollbar-width:thin]"
        style={SCROLLER}
      >
        <Page blocks={BLOCKS} />
      </div>
    </Modal>
  );
}

export function useManual() {
  const [open, setOpen] = useState(false);
  const close = useCallback(() => setOpen(false), []);
  const showManual = useCallback(() => setOpen(true), []);

  // Mounted afresh each time, so it opens at the top as the native one does
  const overlay = open && <Overlay close={close} />;
  return { showManual, overlay };
}
