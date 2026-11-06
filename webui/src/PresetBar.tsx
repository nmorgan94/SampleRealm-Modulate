// Mirrors Source/ui/PresetBar.cpp
import { useSyncExternalStore } from 'react';
import { isSamePreset, type PresetBackend, type PresetEntry } from './backend.ts';
import { TextButton, type Rect } from './controls.tsx';
import { useMenu, type MenuItem } from './Menu.tsx';
import type { PresetActions } from './presetActions.ts';

// PresetBar::resized
function layOut({ left, top, width, height }: Rect) {
  const at = (x: number, w: number): Rect => ({ left: x, top, width: w, height });
  const save = at(left + width - 48, 48);
  const previous = at(left, 24);
  const next = at(save.left - 8 - 24, 24);
  const nameLeft = previous.left + previous.width + 4;
  return { previous, next, save, name: at(nameLeft, next.left - 4 - nameLeft) };
}

interface PresetBarProps {
  presets: PresetBackend;
  actions: PresetActions;
  bounds: Rect;
}

export function PresetBar({ presets, actions, bounds }: PresetBarProps) {
  const status = useSyncExternalStore(presets.onStatus, presets.status);
  const menu = useMenu();
  const layout = layOut(bounds);

  const showPresetMenu = async (button: HTMLElement) => {
    const list = await presets.list();
    const { current } = presets.status();
    const item = (preset: PresetEntry): MenuItem => ({
      kind: 'item',
      label: preset.name,
      ticked: isSamePreset(preset, current),
      onSelect: () => void presets.load(preset),
    });

    // Factory presets by category, user presets flat
    const categories = new Map<string, PresetEntry[]>();
    for (const preset of list.filter((p) => p.isFactory)) {
      const category = preset.category || 'Other';
      categories.set(category, [...(categories.get(category) ?? []), preset]);
    }
    const users = list.filter((preset) => !preset.isFactory);

    const next: MenuItem[] = [{ kind: 'header', label: 'Factory' }];
    for (const category of [...categories.keys()].sort()) {
      const entries = categories.get(category)!;
      next.push({ kind: 'submenu', label: category, ticked: entries.some((p) => isSamePreset(p, current)), items: entries.map(item) });
    }
    if (categories.size === 0) next.push({ kind: 'item', label: 'No factory presets yet', disabled: true });

    next.push({ kind: 'header', label: 'User' }, ...users.map(item));
    if (users.length === 0) next.push({ kind: 'item', label: 'No user presets yet', disabled: true });

    menu.open(button, next);
  };

  return (
    <>
      <TextButton text="<" style={layout.previous} onClick={() => void presets.loadPrevious()} />
      <TextButton text={status.current.name + (status.modified ? ' *' : '')} style={layout.name} onClick={(e) => void showPresetMenu(e.currentTarget)} />
      <TextButton text=">" style={layout.next} onClick={() => void presets.loadNext()} />
      <TextButton text="Save" style={layout.save} onClick={() => void actions.save()} />
      {menu.element}
    </>
  );
}
