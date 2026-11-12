import { centred, type Rect } from '@samplerealm/modules/editor/layout.ts';

export const EDITOR_WIDTH = 900;
export const EDITOR_HEIGHT = 600;

export const LABEL_HEIGHT = 15;

// LabeledKnob::resized: the label along the top, a square control centred under it
export function layOutBelowLabel(bounds: Rect): { control: Rect; label: Rect } {
  const top = bounds.top + LABEL_HEIGHT;
  const height = bounds.height - LABEL_HEIGHT;
  const size = Math.max(0, Math.min(bounds.width, height));
  return {
    control: { left: bounds.left + centred(bounds.width, size), top: top + centred(height, size), width: size, height: size },
    label: { ...bounds, height: LABEL_HEIGHT },
  };
}

// A pointer's position in an element's own pixels, given its unscaled width
export function pointIn(element: Element, width: number, clientX: number, clientY: number) {
  const box = element.getBoundingClientRect();
  const scale = box.width / width;
  return { x: (clientX - box.left) / scale, y: (clientY - box.top) / scale };
}
