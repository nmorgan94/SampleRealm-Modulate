// Mirrors Source/ui/Oscillator.h
import { useMemo } from 'react';
import type { ModulateBackend } from './backend.ts';
import { at, ChoiceBox, Knob, Label, PanelBox, Stepper, Toggle, type Rect } from './controls.tsx';
import { useParam } from '@samplerealm/modules/editor/hooks.ts';
import { useLive, useModulation } from './hooks.ts';
import { modulatedValue, sourceValue } from './modulation.ts';
import { blend, generateWavetable } from './waveforms.ts';

// Every fourth sample of the table is plenty for 220 pixels
const DRAWN_EVERY = 4;

function WaveformView({ backend, ids, bounds, enabled }: { backend: ModulateBackend; ids: { a: string; b: string; morph: string }; bounds: Rect; enabled: boolean }) {
  const a = useParam(backend, ids.a).value;
  const b = useParam(backend, ids.b).value;
  const morph = useParam(backend, ids.morph);
  const modulation = useModulation(backend, ids.morph);
  const live = useLive(backend, modulation !== null);

  const tableA = useMemo(() => generateWavetable(Math.round(a)), [a]);
  const tableB = useMemo(() => generateWavetable(Math.round(b)), [b]);
  const heard = modulation && live ? modulatedValue(morph.param, morph.value, modulation, sourceValue(live, modulation)) : morph.value;
  const samples = blend(tableA, tableB, heard);

  const width = bounds.width - 10;
  const height = bounds.height - 10;
  const centreY = 5 + height / 2;
  const path = Array.from({ length: samples.length / DRAWN_EVERY }, (_, n) => {
    const i = n * DRAWN_EVERY;
    return `${n === 0 ? 'M' : 'L'} ${5 + (i / samples.length) * width} ${centreY - samples[i]! * height * 0.4}`;
  }).join(' ');
  const alpha = enabled ? 1 : 0.3;

  return (
    <svg style={at(bounds)} width={bounds.width} height={bounds.height} aria-hidden>
      <rect width={bounds.width} height={bounds.height} rx={4} fill="#000" fillOpacity={alpha} />
      <line x1={5} x2={5 + width} y1={centreY} y2={centreY} stroke="#808080" strokeOpacity={0.2 * alpha} />
      <path d={path} stroke="var(--color-accent)" strokeOpacity={alpha} strokeWidth={2} fill="none" />
      {[1, 2, 3].map((i) => (
        <line key={i} x1={5} x2={5 + width} y1={5 + (i / 4) * height} y2={5 + (i / 4) * height} stroke="#808080" strokeOpacity={0.1 * alpha} />
      ))}
    </svg>
  );
}

export function OscillatorPanel({ backend, index, bounds }: { backend: ModulateBackend; index: number; bounds: Rect }) {
  const id = (name: string) => `osc${index + 1}_${name}`;
  const enabled = useParam(backend, id('enable')).value >= 0.5;

  return (
    <PanelBox bounds={bounds}>
      <Toggle backend={backend} id={id('enable')} bounds={{ left: 10, top: 10, width: 30, height: 30 }} />
      <Label bounds={{ left: 43, top: 10, width: 68, height: 30 }} text={`OSC ${index + 1}`} enabled={enabled} />
      <ChoiceBox backend={backend} id={id('waveform')} bounds={{ left: 114, top: 10, width: 91, height: 30 }} enabled={enabled} />
      <Knob backend={backend} id={id('gain')} bounds={{ left: 210, top: 10, width: 30, height: 30 }} modulatable enabled={enabled} />

      <Stepper backend={backend} id={id('pitch')} bounds={{ left: 10, top: 50, width: 46, height: 20 }} label="SEM" enabled={enabled} />
      <Stepper backend={backend} id={id('fine')} bounds={{ left: 62, top: 50, width: 46, height: 20 }} label="FIN" enabled={enabled} />
      <ChoiceBox backend={backend} id={id('waveform_b')} bounds={{ left: 114, top: 45, width: 91, height: 30 }} enabled={enabled} />
      <Knob backend={backend} id={id('morph')} bounds={{ left: 210, top: 45, width: 30, height: 30 }} modulatable enabled={enabled} />

      <WaveformView
        backend={backend}
        ids={{ a: id('waveform'), b: id('waveform_b'), morph: id('morph') }}
        bounds={{ left: 10, top: 80, width: 230, height: bounds.height - 90 }}
        enabled={enabled}
      />
    </PanelBox>
  );
}
