// Mirrors Source/ui/PresetActions.cpp
import { isSamePreset, type PresetBackend, type PresetEntry } from './backend.ts';
import type { MenuItem } from './Menu.tsx';
import type { Prompt } from './PromptOverlay.tsx';

export interface PresetActions {
  save(): Promise<void>;
  menuItems(): Promise<MenuItem[]>;
}

export function presetActions(presets: PresetBackend, prompt: Prompt): PresetActions {
  const report = (error: string | null) => {
    if (error) prompt.showMessage('Preset', error);
  };

  const currentUserPreset = async () => {
    const current = (await presets.list()).find((preset) => isSamePreset(preset, presets.status().current));
    return current && !current.isFactory ? current : undefined;
  };

  const saveAs = () => {
    // Factory names are reserved, so don't suggest one
    const { current } = presets.status();

    prompt.askForText('Save Preset', current.isFactory ? '' : current.name, 'Save', async (name) => {
      const save = async () => report(await presets.save(name, presets.status().current.category));
      if (!(await presets.exists(name))) return save();
      prompt.askToConfirm('Replace Preset', `"${name.trim()}" already exists. Replace it?`, 'Replace', save);
    });
  };

  // User presets save in place; anything else needs a name first
  const save = async () => {
    const user = await currentUserPreset();
    if (user) report(await presets.save(user.name, user.category));
    else saveAs();
  };

  const rename = (preset: PresetEntry) =>
    prompt.askForText('Rename Preset', preset.name, 'Rename', async (name) => report(await presets.rename(preset, name)));

  const remove = (preset: PresetEntry) =>
    prompt.askToConfirm('Delete Preset', `Delete "${preset.name}"?`, 'Delete', async () => report(await presets.remove(preset)));

  const menuItems = async () => {
    const user = await currentUserPreset();
    const items: MenuItem[] = [
      { kind: 'item', label: 'Init Preset', onSelect: () => void presets.loadInit() },
      { kind: 'separator' },
      { kind: 'item', label: 'Save As...', onSelect: saveAs },
      { kind: 'item', label: 'Rename...', disabled: !user, onSelect: () => user && rename(user) },
      { kind: 'item', label: 'Delete', disabled: !user, onSelect: () => user && remove(user) },
    ];
    if (presets.showFolder) items.push({ kind: 'separator' }, { kind: 'item', label: 'Show Preset Folder', onSelect: presets.showFolder });
    return items;
  };

  return { save, menuItems };
}
