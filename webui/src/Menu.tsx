// Mirrors CustomLookAndFeel's popup menu
import { useLayoutEffect, useRef, useState } from 'react';
import { createPortal } from 'react-dom';
import { areaInEditor, EDITOR_HEIGHT, useEditorRoot, type Rect } from './editorRoot.ts';
import { juceHeight } from './fonts.ts';

export type MenuItem = { label: string; onSelect: () => void; colour?: string } | 'separator';

interface MenuProps {
  items: MenuItem[];
  // Opens below this, or above it when there's no room
  anchor: Rect;
  onClose: () => void;
}

export function Menu({ items, anchor, onClose }: MenuProps) {
  const root = useEditorRoot();
  const list = useRef<HTMLUListElement>(null);
  const [top, setTop] = useState(anchor.top + anchor.height);

  useLayoutEffect(() => {
    const height = list.current?.offsetHeight ?? 0;
    const below = anchor.top + anchor.height;
    setTop(below + height <= EDITOR_HEIGHT ? below : Math.max(0, Math.min(anchor.top - height, EDITOR_HEIGHT - height)));
  }, [anchor, items.length]);

  if (!root) return null;

  return createPortal(
    <>
      <div
        className="absolute inset-0 z-40"
        onPointerDown={onClose}
        onContextMenu={(e) => {
          e.preventDefault();
          onClose();
        }}
      />
      <ul
        ref={list}
        className="absolute z-41 overflow-y-auto border border-widget-border bg-widget py-1"
        style={{ left: anchor.left, top, minWidth: anchor.width, maxHeight: EDITOR_HEIGHT, fontSize: juceHeight(14) }}
        role="menu"
      >
        {items.map((item, i) =>
          item === 'separator' ? (
            <li key={i} className="mx-2 my-1 h-px bg-widget-border" role="separator" />
          ) : (
            <li
              key={i}
              className="group cursor-pointer px-2.5 py-0.5 whitespace-nowrap hover:bg-accent/20"
              style={{ color: item.colour ?? 'var(--color-fg)' }}
              role="menuitem"
              onClick={() => {
                onClose();
                item.onSelect();
              }}
            >
              <span className={item.colour ? 'group-hover:brightness-140' : 'group-hover:text-accent'}>{item.label}</span>
            </li>
          )
        )}
      </ul>
    </>,
    root
  );
}

// Opens a menu anchored on an element
export function useMenu() {
  const root = useEditorRoot();
  const [anchor, setAnchor] = useState<Rect | null>(null);

  const open = (element: HTMLElement, at?: { x: number; y: number }) => {
    if (!root) return;
    const area = areaInEditor(element, root);
    setAnchor(at ? { left: area.left + at.x, top: area.top + at.y, width: 0, height: 0 } : area);
  };

  return { anchor, open, close: () => setAnchor(null) };
}

