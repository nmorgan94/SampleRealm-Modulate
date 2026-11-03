// Mirrors Source/ui/VoicePanel.h
import type { ModulateBackend } from './backend.ts';
import { Knob, Label, PanelBox, Stepper, type Rect } from './controls.tsx';

const STEPPERS = [
  ['num_voices', 'VOICES'],
  ['unison_voices', 'UNISON'],
  ['unison_detune', 'DETUNE'],
  ['unison_spread', 'SPREAD'],
] as const;

export function VoicePanel({ backend, bounds }: { backend: ModulateBackend; bounds: Rect }) {
  return (
    <PanelBox bounds={bounds}>
      <Label bounds={{ left: 10, top: 10, width: 100, height: 30 }} text="VOICE" bold size={16} align="left" />
      {STEPPERS.map(([id, label], i) => (
        <Stepper key={id} backend={backend} id={id} bounds={{ left: 10, top: 51 + 33 * i, width: 100, height: 20 }} label={label} showSign={false} />
      ))}
      <Knob
        backend={backend}
        id="coarse_pitch"
        bounds={{ left: 159, top: 60, width: 52, height: 52 }}
        label={{ bounds: { left: 120, top: 45, width: 130, height: 15 }, text: 'Pitch' }}
        modulatable
      />
      <Knob
        backend={backend}
        id="glide_time"
        bounds={{ left: 158, top: 127, width: 53, height: 53 }}
        label={{ bounds: { left: 120, top: 112, width: 130, height: 15 }, text: 'Glide' }}
        modulatable
      />
    </PanelBox>
  );
}
