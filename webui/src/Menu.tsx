// Mirrors CustomLookAndFeel's popup menu
import { useLayoutEffect, useRef, useState } from 'react';
import { createPortal } from 'react-dom';
import { areaInEditor, EDITOR_HEIGHT, EDITOR_WIDTH, useEditorRoot, type Rect } from './editorRoot.ts';
import { juceHeight } from './fonts.ts';

// Ticked items, such as the current preset, are green instead of ticked
export type MenuItem =
  | { kind: 'header'; label: string }
  | { kind: 'separator' }
  | { kind: 'item'; label: string; ticked?: boolean; disabled?: boolean; onSelect?: () => void }
  | { kind: 'submenu'; label: string; ticked?: boolean; items: MenuItem[] };

interface MenuProps {
  items: MenuItem[];
  // Opens below this, or above it when there's no room
  anchor: Rect;
  onClose: () => void;
}

// Where a list goes: to the right of `left`, or left of `leftIfNoRoom` when there's no room
type Placement = { left: number; leftIfNoRoom: number; top: number; bottom: number };

function Arrow() {
  return (
    <svg className="ml-2 flex-none" width={6} height={8} viewBox="0 0 6 8" aria-hidden>
      <path d="M0 0 L6 4 L0 8 Z" fill="var(--color-accent)" />
    </svg>
  );
}

function MenuList({ items, placement, minWidth, onClose }: { items: MenuItem[]; placement: Placement; minWidth: number; onClose: () => void }) {
  const list = useRef<HTMLUListElement>(null);
  const rows = useRef<(HTMLLIElement | null)[]>([]);
  const [box, setBox] = useState<Rect | null>(null);
  const [open, setOpen] = useState<number | null>(null);

  // Kept inside the editor, as JUCE keeps menus on screen
  useLayoutEffect(() => {
    const element = list.current;
    if (!element) return;

    const { offsetWidth: width, offsetHeight: height } = element;
    const left = Math.max(0, placement.left + width <= EDITOR_WIDTH ? placement.left : placement.leftIfNoRoom - width);
    const top = placement.top + height <= EDITOR_HEIGHT ? placement.top : Math.max(0, Math.min(placement.bottom - height, EDITOR_HEIGHT - height));
    setBox({ left, top, width, height });
  }, [placement.left, placement.leftIfNoRoom, placement.top, placement.bottom, items.length]);

  const submenu = open === null ? null : items[open];
  const row = open === null ? null : rows.current[open];

  return (
    <>
      <ul
        ref={list}
        className="absolute z-41 overflow-y-auto border border-widget-border bg-widget py-1"
        style={{ left: box?.left ?? placement.left, top: box?.top ?? placement.top, visibility: box ? 'visible' : 'hidden', minWidth, maxHeight: EDITOR_HEIGHT, fontSize: juceHeight(14) }}
        role="menu"
      >
        {items.map((item, i) => {
          if (item.kind === 'separator') return <li key={i} className="mx-2 my-1 h-px bg-widget-border" role="separator" />;

          if (item.kind === 'header') {
            return (
              <li key={i} className="px-2.5 py-0.5 font-bold whitespace-nowrap text-accent" role="presentation">
                {item.label}
              </li>
            );
          }

          const disabled = item.kind === 'item' && item.disabled;
          const colour = disabled ? 'var(--color-dim)' : item.ticked ? 'var(--color-accent)' : 'var(--color-fg)';
          const highlighted = open === i;

          return (
            <li
              key={i}
              ref={(element) => void (rows.current[i] = element)}
              className={`group flex items-center justify-between px-2.5 py-0.5 whitespace-nowrap ${disabled ? '' : 'cursor-pointer hover:bg-accent/20'} ${highlighted ? 'bg-accent/20' : ''}`}
              style={{ color: colour }}
              role="menuitem"
              aria-disabled={disabled}
              aria-haspopup={item.kind === 'submenu'}
              onPointerEnter={(e) => {
                if (e.pointerType !== 'touch') setOpen(item.kind === 'submenu' ? i : null);
              }}
              onClick={() => {
                if (item.kind === 'submenu') setOpen(highlighted ? null : i);
                else if (!disabled) {
                  onClose();
                  item.onSelect?.();
                }
              }}
            >
              <span className={disabled ? '' : 'group-hover:text-accent'}>{item.label}</span>
              {item.kind === 'submenu' && <Arrow />}
            </li>
          );
        })}
      </ul>
      {box && submenu?.kind === 'submenu' && row && (
        <MenuList
          key={open}
          items={submenu.items}
          placement={{ left: box.left + box.width, leftIfNoRoom: box.left, top: box.top + row.offsetTop - 5, bottom: box.top + row.offsetTop + row.offsetHeight }}
          minWidth={0}
          onClose={onClose}
        />
      )}
    </>
  );
}

export function Menu({ items, anchor, onClose }: MenuProps) {
  const root = useEditorRoot();

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
      <MenuList
        items={items}
        placement={{ left: anchor.left, leftIfNoRoom: anchor.left + anchor.width, top: anchor.top + anchor.height, bottom: anchor.top }}
        minWidth={anchor.width}
        onClose={onClose}
      />
    </>,
    root
  );
}

// Opens a menu anchored on an element; render `element` to show it
export function useMenu() {
  const root = useEditorRoot();
  const [shown, setShown] = useState<{ anchor: Rect; items: MenuItem[] } | null>(null);

  const open = (element: HTMLElement, items: MenuItem[], at?: { x: number; y: number }) => {
    if (!root) return;
    const area = areaInEditor(element, root);
    setShown({ anchor: at ? { left: area.left + at.x, top: area.top + at.y, width: 0, height: 0 } : area, items });
  };
  const close = () => setShown(null);

  return { open, element: shown && <Menu anchor={shown.anchor} items={shown.items} onClose={close} /> };
}
