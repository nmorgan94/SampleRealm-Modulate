// Mirrors Source/ui/PresetBar.h
import { useEffect, useState, type ComponentProps } from 'react';
import { INIT_PRESET, type ModulateBackend } from './backend.ts';
import { ComboBox, type Rect } from './controls.tsx';
import { juceHeight } from './fonts.ts';
import type { MenuItem } from './Menu.tsx';

const ACTION_COLOUR = 'var(--color-accent)';
const DELETE_COLOUR = 'var(--color-delete)';

type Dialog = { kind: 'save'; name: string } | { kind: 'delete'; name: string };

function DialogButton({ children, ...props }: ComponentProps<'button'>) {
  return (
    <button type="button" className="h-6.5 min-w-16 cursor-pointer rounded-sm border border-widget-border bg-widget px-2 hover:text-accent" {...props}>
      {children}
    </button>
  );
}

export function PresetBar({ backend, bounds }: { backend: ModulateBackend; bounds: Rect }) {
  const [names, setNames] = useState<string[]>([]);
  const [current, setCurrent] = useState('');
  const [dialog, setDialog] = useState<Dialog | null>(null);

  const refresh = async () => {
    const list = await backend.presets.list();
    setNames(list.names);
    setCurrent(list.current);
  };

  useEffect(() => {
    refresh();
  }, [backend]);

  const load = async (name: string) => {
    await backend.presets.load(name);
    await refresh();
  };

  const save = async (name: string) => {
    setDialog(null);
    const trimmed = name.trim();
    if (trimmed && (await backend.presets.save(trimmed))) await refresh();
  };

  const remove = async (name: string) => {
    setDialog(null);
    await backend.presets.remove(name);
    await refresh();
  };

  // Rebuilt as it opens, so Delete follows the current preset
  const items = (): MenuItem[] => [
    { label: INIT_PRESET, onSelect: () => load(INIT_PRESET) },
    { label: 'Save', colour: ACTION_COLOUR, onSelect: () => (current ? save(current) : setDialog({ kind: 'save', name: '' })) },
    { label: 'Save As...', colour: ACTION_COLOUR, onSelect: () => setDialog({ kind: 'save', name: current }) },
    ...(current ? [{ label: `Delete "${current}"`, colour: DELETE_COLOUR, onSelect: () => setDialog({ kind: 'delete', name: current }) }] : []),
    'separator' as const,
    ...names.map((name) => ({ label: name, onSelect: () => load(name) })),
  ];

  return (
    <>
      <ComboBox bounds={bounds} text={current || INIT_PRESET} items={items} />

      {dialog && (
        <div className="absolute inset-0 z-50 flex items-center justify-center bg-black/50" style={{ fontSize: juceHeight(14) }}>
          <form
            className="min-w-70 rounded-md border border-accent bg-panel p-4"
            onSubmit={(e) => {
              e.preventDefault();
              if (dialog.kind === 'save') save(new FormData(e.currentTarget).get('presetName')?.toString() ?? '');
              else remove(dialog.name);
            }}
          >
            <h2 className="mb-2 font-bold">{dialog.kind === 'save' ? 'Save Preset' : 'Delete Preset'}</h2>
            {dialog.kind === 'save' ? (
              <>
                <p className="mb-2">Enter a name for this preset:</p>
                <input
                  className="mb-3 w-full rounded-sm border border-widget-border bg-black px-1.5 py-1"
                  name="presetName"
                  defaultValue={dialog.name}
                  autoFocus
                  onFocus={(e) => e.currentTarget.select()}
                />
              </>
            ) : (
              <p className="mb-3">{`Delete preset "${dialog.name}"?`}</p>
            )}
            <div className="flex justify-end gap-1.5">
              <DialogButton type="submit" autoFocus={dialog.kind === 'delete'}>
                {dialog.kind === 'save' ? 'Save' : 'Delete'}
              </DialogButton>
              <DialogButton onClick={() => setDialog(null)}>Cancel</DialogButton>
            </div>
          </form>
        </div>
      )}
    </>
  );
}
