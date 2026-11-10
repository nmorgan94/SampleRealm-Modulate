import type { Rect } from './editorRoot.ts';

// juce::Rectangle::withSizeKeepingCentre
export const centred = (outer: number, inner: number) => Math.trunc((outer - inner) / 2);

export const LABEL_HEIGHT = 15;

// LabeledSlider::resized: the label along the top, a square control centred under it
export function layOutBelowLabel(bounds: Rect): { control: Rect; label: Rect } {
  const top = bounds.top + LABEL_HEIGHT;
  const height = bounds.height - LABEL_HEIGHT;
  const size = Math.max(0, Math.min(bounds.width, height));
  return {
    control: { left: bounds.left + centred(bounds.width, size), top: top + centred(height, size), width: size, height: size },
    label: { ...bounds, height: LABEL_HEIGHT },
  };
}
