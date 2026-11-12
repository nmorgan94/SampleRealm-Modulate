// Mirrors Source/ui/NoisePanel.h
import type { ModulateBackend } from './backend.ts';
import { ChoiceBox, Knob, Label, PanelBox, Toggle, type Rect } from './controls.tsx';
import { useParam } from '@samplerealm/modules/editor/hooks.ts';

export function NoisePanel({ backend, bounds }: { backend: ModulateBackend; bounds: Rect }) {
  const enabled = useParam(backend, 'noise_enable').value >= 0.5;

  return (
    <PanelBox bounds={bounds}>
      <Toggle backend={backend} id="noise_enable" bounds={{ left: 10, top: 10, width: 30, height: 30 }} />
      <Label bounds={{ left: 43, top: 10, width: 60, height: 30 }} text="NOISE" bold size={16} align="left" enabled={enabled} />
      <ChoiceBox backend={backend} id="noise_type" bounds={{ left: 108, top: 10, width: 107, height: 30 }} enabled={enabled} />
      <Knob backend={backend} id="noise_level" bounds={{ left: 220, top: 10, width: 30, height: 30 }} modulatable enabled={enabled} />
    </PanelBox>
  );
}
