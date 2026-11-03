// Mirrors AudioPluginAudioProcessorEditor
import { memo, useCallback, useLayoutEffect, useMemo, useRef, useState, type PointerEvent as ReactPointerEvent } from 'react';
import type { ModulateBackend, ModulationSource } from './backend.ts';
import { Cross, MOD_TARGET, ModulationDragContext, type ModulationDrag } from './controls.tsx';
import { EDITOR_HEIGHT, EDITOR_WIDTH, EditorRootContext, pointIn } from './editorRoot.ts';
import { EnvelopePanel } from './EnvelopePanel.tsx';
import { FilterPanel } from './FilterPanel.tsx';
import { juceHeight } from './fonts.ts';
import { LfoPanel } from './LfoPanel.tsx';
import { MasterMeter } from './MasterMeter.tsx';
import { makeDefaultRange } from './modulation.ts';
import { NoisePanel } from './NoisePanel.tsx';
import { OscillatorPanel } from './OscillatorPanel.tsx';
import { toProportion } from './params.ts';
import { PresetBar } from './PresetBar.tsx';
import { VoicePanel } from './VoicePanel.tsx';

export { EDITOR_HEIGHT, EDITOR_WIDTH };

const TITLE = 'SampleRealm: MODULATE';
const TITLE_WIDTH = 550;

// drawFittedText squeezes it to fit, as far as 70% wide
function Title() {
  const text = useRef<HTMLSpanElement>(null);
  const [squeeze, setSqueeze] = useState(1);

  useLayoutEffect(() => {
    const measure = () => setSqueeze(Math.max(0.7, Math.min(1, TITLE_WIDTH / (text.current?.offsetWidth ?? TITLE_WIDTH))));
    measure();
    document.fonts?.ready.then(measure);
  }, []);

  return (
    <h1 className="absolute top-0 left-2.5 flex h-17.5 items-center" style={{ width: TITLE_WIDTH }}>
      <span
        ref={text}
        className="origin-left font-bold whitespace-nowrap text-title"
        style={{ fontSize: juceHeight(52), transform: `scaleX(${squeeze})` }}
      >
        {TITLE}
      </span>
    </h1>
  );
}

type Dragging = { source: ModulationSource; index: number; x: number; y: number };

// The demo re-renders on every key press
export const ModulateEditor = memo(function ModulateEditor({ backend }: { backend: ModulateBackend }) {
  const [root, setRoot] = useState<HTMLDivElement | null>(null);
  const [dragging, setDragging] = useState<Dragging | null>(null);

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
          <Title />
          <span className="absolute left-2.5 h-3 leading-3 text-accent/40" style={{ top: 60, fontSize: 9 }}>
            {`v${backend.info.version}`}
          </span>
          <PresetBar backend={backend} bounds={{ left: 590, top: 20, width: 180, height: 30 }} />
          <MasterMeter backend={backend} bounds={{ left: 800, top: 6, width: 90, height: 58 }} />

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
        </div>
      </ModulationDragContext.Provider>
    </EditorRootContext.Provider>
  );
});
