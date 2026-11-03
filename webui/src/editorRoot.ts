import { createContext, useContext } from 'react';

export const EDITOR_WIDTH = 900;
export const EDITOR_HEIGHT = 600;

// Where popups go, so the editor's own clipping doesn't cut them off
export const EditorRootContext = createContext<HTMLElement | null>(null);

export const useEditorRoot = () => useContext(EditorRootContext);

export interface Rect {
  left: number;
  top: number;
  width: number;
  height: number;
}

// In the editor's own pixels, which the demo's scaling doesn't change
export function areaInEditor(element: HTMLElement, root: HTMLElement): Rect {
  let left = 0;
  let top = 0;
  for (let node: HTMLElement | null = element; node && node !== root; node = node.offsetParent as HTMLElement | null) {
    left += node.offsetLeft;
    top += node.offsetTop;
  }
  return { left, top, width: element.offsetWidth, height: element.offsetHeight };
}

// A pointer's position in an element's own pixels, given its unscaled width
export function pointIn(element: Element, width: number, clientX: number, clientY: number) {
  const box = element.getBoundingClientRect();
  const scale = box.width / width;
  return { x: (clientX - box.left) / scale, y: (clientY - box.top) / scale };
}
