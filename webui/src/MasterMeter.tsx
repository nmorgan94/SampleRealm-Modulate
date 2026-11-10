// Mirrors Source/ui/MasterMeter.h
import type { ModulateBackend } from './backend.ts';
import { Knob, PanelBox, type Rect } from './controls.tsx';
import { LevelMeter, PREFERRED_WIDTH } from './LevelMeter.tsx';

export function MasterMeter({ backend, bounds }: { backend: ModulateBackend; bounds: Rect }) {
  return (
    <PanelBox bounds={bounds}>
      <Knob backend={backend} id="master_gain" bounds={{ left: 6, top: 7, width: 44, height: 44 }} />
      <LevelMeter backend={backend} x={56} y={5} width={PREFERRED_WIDTH} height={bounds.height - 10} />
    </PanelBox>
  );
}
