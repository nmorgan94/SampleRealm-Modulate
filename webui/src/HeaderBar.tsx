// Mirrors Source/ui/HeaderBar.cpp
import type { ReactNode } from 'react';
import { aboutText } from './about.ts';
import type { PresetBackend, UiInfo } from './backend.ts';
import { FittedText, TextButton, type Rect } from './controls.tsx';
import { juceHeight } from './fonts.ts';
import { centred } from './layout.ts';
import { useMenu } from './Menu.tsx';
import { presetActions } from './presetActions.ts';
import { PresetBar } from './PresetBar.tsx';
import type { Prompt } from './PromptOverlay.tsx';

// HeaderBar::Layout, which each plugin sets
export interface HeaderLayout {
  height: number;
  // Left of the title, right of the meter
  edge: number;
  // A longer title is squeezed to fit
  titleWidth: number;
  titleHeight: number;
  // Either side of the preset bar and menu
  gap: number;
  controlsHeight: number;
  meterWidth: number;
  // Above and below the meter
  meterInset: number;
}

// HeaderBar::resized
const layOut = (width: number, layout: HeaderLayout) => {
  const { height, edge, titleWidth, gap, controlsHeight, meterWidth, meterInset } = layout;
  const meter: Rect = { left: width - edge - meterWidth, top: meterInset, width: meterWidth, height: height - 2 * meterInset };
  const left = edge + titleWidth + gap;
  const controls: Rect = { left, top: centred(height, controlsHeight), width: meter.left - gap - left, height: controlsHeight };
  return {
    title: { left: edge, top: 0, width: titleWidth, height },
    meter,
    menu: { ...controls, left: controls.left + controls.width - 28, width: 28 },
    presetBar: { ...controls, width: controls.width - 28 - 4 },
  };
};

interface HeaderBarProps {
  title: string;
  width: number;
  layout: HeaderLayout;
  backend: { info: Pick<UiInfo, 'version' | 'commit' | 'build'>; running: string; presets: PresetBackend };
  prompt: Prompt;
  showManual(): void;
  meter(bounds: Rect): ReactNode;
}

export function HeaderBar({ title, width, layout, backend, prompt, showManual, meter }: HeaderBarProps) {
  const menu = useMenu();
  const boxes = layOut(width, layout);
  const actions = presetActions(backend.presets, prompt);

  const showMenu = async (button: HTMLElement) => {
    menu.open(button, [
      ...(await actions.menuItems()),
      { kind: 'separator' },
      { kind: 'item', label: 'Manual', onSelect: showManual },
      { kind: 'item', label: 'About', onSelect: () => prompt.showMessage('About', aboutText(backend.info, backend.running)) },
    ]);
  };

  return (
    <>
      <div className="absolute inset-x-0 top-0 bg-header" style={{ height: layout.height }} />
      <h1
        className="absolute flex items-center font-bold whitespace-nowrap text-title [&>span]:origin-left"
        style={{ ...boxes.title, fontSize: juceHeight(layout.titleHeight) }}
      >
        <FittedText text={title} width={layout.titleWidth} />
      </h1>
      <PresetBar presets={backend.presets} actions={actions} bounds={boxes.presetBar} />
      <TextButton text="..." style={boxes.menu} onClick={(e) => void showMenu(e.currentTarget)} />
      {menu.element}
      {meter(boxes.meter)}
    </>
  );
}
