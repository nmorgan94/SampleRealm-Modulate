// Mirrors Source/ui/HeaderBar.cpp
import { aboutText } from './about.ts';
import type { ModulateBackend } from './backend.ts';
import { FittedText, TextButton, type Rect } from './controls.tsx';
import { juceHeight } from './fonts.ts';
import { MasterMeter } from './MasterMeter.tsx';
import { useMenu } from './Menu.tsx';
import { presetActions } from './presetActions.ts';
import { PresetBar } from './PresetBar.tsx';
import type { Prompt } from './PromptOverlay.tsx';

const HEADER_HEIGHT = 70;

const EDGE = 10;
const GAP = 12;
const TITLE_WIDTH = 400;
const METER_WIDTH = 90;
const CONTROLS_HEIGHT = 30;

// HeaderBar::resized
const layOut = (width: number) => {
  const meter: Rect = { left: width - EDGE - METER_WIDTH, top: 6, width: METER_WIDTH, height: HEADER_HEIGHT - 12 };
  const left = EDGE + TITLE_WIDTH + GAP;
  const controls: Rect = { left, top: (HEADER_HEIGHT - CONTROLS_HEIGHT) / 2, width: meter.left - GAP - left, height: CONTROLS_HEIGHT };
  return {
    meter,
    menu: { ...controls, left: controls.left + controls.width - 28, width: 28 },
    presetBar: { ...controls, width: controls.width - 28 - 4 },
  };
};

function Title({ text }: { text: string }) {
  return (
    <h1
      className="absolute top-0 flex items-center font-bold whitespace-nowrap text-title [&>span]:origin-left"
      style={{ left: EDGE, width: TITLE_WIDTH, height: HEADER_HEIGHT, fontSize: juceHeight(36) }}
    >
      <FittedText text={text} width={TITLE_WIDTH} />
    </h1>
  );
}

interface HeaderBarProps {
  title: string;
  width: number;
  backend: ModulateBackend;
  prompt: Prompt;
  showManual(): void;
}

// A fragment, not a box, so the menus stay positioned against the editor
export function HeaderBar({ title, width, backend, prompt, showManual }: HeaderBarProps) {
  const menu = useMenu();
  const layout = layOut(width);
  const actions = presetActions(backend.presets, prompt);

  const showMenu = async (button: HTMLElement) => {
    menu.open(button, [
      ...(await actions.menuItems()),
      { kind: 'separator' },
      { kind: 'item', label: 'Manual', onSelect: showManual },
      { kind: 'item', label: 'About', onSelect: () => prompt.showMessage('About', aboutText(backend.about)) },
    ]);
  };

  return (
    <>
      <Title text={title} />
      <span className="absolute h-3 leading-3 text-accent/40" style={{ left: EDGE, top: 60, fontSize: 9 }}>
        {`v${backend.info.version}`}
      </span>
      <PresetBar presets={backend.presets} actions={actions} bounds={layout.presetBar} />
      <TextButton text="..." style={layout.menu} onClick={(e) => void showMenu(e.currentTarget)} />
      {menu.element}
      <MasterMeter backend={backend} bounds={layout.meter} />
    </>
  );
}
