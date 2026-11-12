// juce::BubbleComponent::setPosition, for a slider's value popup within the editor
import type { Rect } from '@samplerealm/modules/editor/layout.ts';
import { EDITOR_HEIGHT, EDITOR_WIDTH } from './layout.ts';

const ARROW = 10;

// The bubble goes on the side with the most room, an arrow's length from the target.
// Only knobs show one, so JUCE's rule for long targets is left out
export function bubbleAt(target: Rect, width: number, height: number): { left: number; top: number } {
  const right = target.left + target.width;
  const bottom = target.top + target.height;

  const spaceAbove = Math.max(0, target.top);
  const spaceBelow = Math.max(0, EDITOR_HEIGHT - bottom);
  const spaceLeft = Math.max(0, target.left);
  const spaceRight = Math.max(0, EDITOR_WIDTH - right);

  if (Math.max(spaceAbove, spaceBelow) >= Math.max(spaceLeft, spaceRight)) {
    const left = target.left + target.width / 2 - width / 2;
    return { left, top: spaceAbove >= spaceBelow ? target.top - ARROW - height : bottom + ARROW };
  }

  const top = target.top + target.height / 2 - height / 2;
  return { left: spaceLeft > spaceRight ? target.left - ARROW - width : right + ARROW, top };
}
