// Mirrors AudioPluginAudioProcessorEditor
import { memo, useCallback, useMemo, useState, type PointerEvent as ReactPointerEvent } from 'react';
import { EditorRootContext } from '@samplerealm/modules/editor/editorRoot.ts';
import { HeaderBar, type HeaderLayout } from '@samplerealm/modules/editor/HeaderBar.tsx';
import { useManual } from '@samplerealm/modules/editor/ManualOverlay.tsx';
import { toProportion } from '@samplerealm/modules/editor/params.ts';
import { usePrompt } from '@samplerealm/modules/editor/PromptOverlay.tsx';
import manualText from '../../Assets/Manual.md?raw';
import type { ModulateBackend, ModulationSource } from './backend.ts';
import { Cross, MOD_TARGET, ModulationDragContext, type ModulationDrag } from './controls.tsx';
import { EnvelopePanel } from './EnvelopePanel.tsx';
import { FilterPanel } from './FilterPanel.tsx';
import { EDITOR_HEIGHT, EDITOR_WIDTH, pointIn } from './layout.ts';
import { LfoPanel } from './LfoPanel.tsx';
import { MasterMeter } from './MasterMeter.tsx';
import { makeDefaultRange } from './modulation.ts';
import { NoisePanel } from './NoisePanel.tsx';
import { OscillatorPanel } from './OscillatorPanel.tsx';
import { VoicePanel } from './VoicePanel.tsx';

// AudioPluginAudioProcessorEditor::headerLayout
const HEADER_LAYOUT: HeaderLayout = {
  height: 70,
  edge: 10,
  titleWidth: 400,
  titleHeight: 36,
  gap: 12,
  controlsHeight: 30,
  meterWidth: 90,
  meterInset: 6,
};

type Dragging = { source: ModulationSource; index: number; x: number; y: number };

// The demo re-renders on every key press
export const ModulateEditor = memo(function ModulateEditor({ backend }: { backend: ModulateBackend }) {
  const [root, setRoot] = useState<HTMLDivElement | null>(null);
  const [dragging, setDragging] = useState<Dragging | null>(null);
  const { prompt, overlay } = usePrompt();
  const manual = useManual(manualText);

  const inEditor = useCallback(
    (clientX: number, clientY: number) => (root ? pointIn(root, EDITOR_WIDTH, clientX, clientY) : { x: clientX, y: clientY }),
    [root]
  );

  const drag = useMemo<ModulationDrag>(
    () => ({
      start: (source, index, e: ReactPointerEvent) => {
        setDragging({ source, index, ...inEditor(e.clientX, e.clientY) });

        const move = (event: PointerEvent) => setDragging((current) => current && { ...current, ...inEditor(event.clientX, event.clientY) });
        const finish = (event: PointerEvent) => {
          window.removeEventListener('pointermove', move);
          window.removeEventListener('pointerup', finish);
          window.removeEventListener('pointercancel', finish);
          setDragging(null);
          if (event.type !== 'pointerup') return;

          const id = document.elementFromPoint(event.clientX, event.clientY)?.closest(`[${MOD_TARGET}]`)?.getAttribute(MOD_TARGET);
          if (!id) return;
          backend.setModulation(id, makeDefaultRange(source, index, toProportion(backend.param(id), backend.getValue(id))));
        };

        window.addEventListener('pointermove', move);
        window.addEventListener('pointerup', finish);
        window.addEventListener('pointercancel', finish);
      },
    }),
    [backend, inEditor]
  );

  return (
    <EditorRootContext.Provider value={root}>
      <ModulationDragContext.Provider value={drag}>
        <div ref={setRoot} className="relative overflow-hidden bg-background font-display text-fg select-none" style={{ width: EDITOR_WIDTH, height: EDITOR_HEIGHT }}>
          <HeaderBar
            title="SampleRealm: MODULATE"
            width={EDITOR_WIDTH}
            layout={HEADER_LAYOUT}
            backend={backend}
            prompt={prompt}
            showManual={manual.showManual}
            meter={(bounds) => <MasterMeter backend={backend} bounds={bounds} />}
          />

          {[0, 1, 2].map((index) => (
            <OscillatorPanel key={index} backend={backend} index={index} bounds={{ left: 10, top: 75 + 176 * index, width: 250, height: index === 2 ? 168 : 166 }} />
          ))}

          <NoisePanel backend={backend} bounds={{ left: 270, top: 75, width: 260, height: 50 }} />
          <FilterPanel backend={backend} bounds={{ left: 270, top: 135, width: 260, height: 260 }} />
          <VoicePanel backend={backend} bounds={{ left: 270, top: 405, width: 260, height: 190 }} />

          <LfoPanel backend={backend} bounds={{ left: 540, top: 75, width: 350, height: 255 }} />
          <EnvelopePanel backend={backend} bounds={{ left: 540, top: 340, width: 350, height: 255 }} />

          {dragging && (
            <div className="pointer-events-none absolute z-50 -translate-1/2" style={{ left: dragging.x, top: dragging.y }}>
              <Cross size={16} />
            </div>
          )}
          {overlay}
          {manual.overlay}
        </div>
      </ModulationDragContext.Provider>
    </EditorRootContext.Provider>
  );
});
