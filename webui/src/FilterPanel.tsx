// Mirrors Source/ui/FilterPanel.h and FilterResponseDisplay.h
import type { ModulateBackend } from './backend.ts';
import { at, ChoiceBox, Knob, Label, PanelBox, PillToggle, Toggle, type Rect } from './controls.tsx';
import { FilterMode, filterMagnitude } from './filterResponse.ts';
import { juceHeight } from '@samplerealm/modules/editor/fonts.ts';
import { useParam } from '@samplerealm/modules/editor/hooks.ts';
import { clamp01 } from '@samplerealm/modules/editor/params.ts';
import { useLive, useModulation } from './hooks.ts';
import { LABEL_HEIGHT } from './layout.ts';
import { modulatedValue, sourceValue } from './modulation.ts';

// Only the curve's shape, so any rate will do
const DISPLAY_SAMPLE_RATE = 48000;
const CURVE_POINTS = 200;

function useHeard(backend: ModulateBackend, id: string) {
  const { value, param } = useParam(backend, id);
  const modulation = useModulation(backend, id);
  const live = useLive(backend, modulation !== null);
  return modulation && live ? modulatedValue(param, value, modulation, sourceValue(live, modulation)) : value;
}

function ResponseDisplay({ backend, bounds, enabled }: { backend: ModulateBackend; bounds: Rect; enabled: boolean }) {
  const mode = Math.round(useParam(backend, 'filter_mode').value);
  const slope = Math.round(useParam(backend, 'filter_slope').value);
  const cutoff = useHeard(backend, 'filter_cutoff');
  const resonance = useHeard(backend, 'filter_resonance');
  const { width, height } = bounds;

  const points = Array.from({ length: CURVE_POINTS }, (_, i) => {
    const t = i / (CURVE_POINTS - 1);
    const magnitude = filterMagnitude(mode, slope, cutoff, resonance, 20 * Math.pow(1000, t), DISPLAY_SAMPLE_RATE);
    const db = 20 * Math.log10(Math.max(magnitude, 0.00001));
    return [t * width, clamp01((20 - db) / 80) * height] as const;
  });
  const curve = points.map(([x, y], i) => `${i === 0 ? 'M' : 'L'} ${x} ${y}`).join(' ');
  const colour = enabled ? 'var(--color-accent)' : 'var(--color-dim)';
  const labelColour = enabled ? 'var(--color-dim)' : 'var(--color-widget-border)';

  return (
    <div style={at(bounds)}>
      <svg className="absolute inset-0" width={width} height={height} aria-hidden>
        <rect width={width} height={height} rx={4} fill="var(--color-display)" />
        <path d={`${curve} L ${width} ${height} L 0 ${height} Z`} fill={colour} fillOpacity={0.2} />
        <path d={curve} stroke={colour} strokeWidth={2} fill="none" />
      </svg>
      <div className="absolute inset-x-0 bottom-0 flex h-3 justify-between leading-3" style={{ color: labelColour, fontSize: juceHeight(10) }}>
        <span>20</span>
        <span>1k</span>
        <span>20k</span>
      </div>
    </div>
  );
}

export function FilterPanel({ backend, bounds }: { backend: ModulateBackend; bounds: Rect }) {
  const enabled = useParam(backend, 'filter_enable').value >= 0.5;
  const mode = Math.round(useParam(backend, 'filter_mode').value);
  // A notch has no skirt to steepen
  const slopeUsable = enabled && mode !== FilterMode.Notch;

  const knob = (id: string, text: string, left: number) => (
    <Knob
      backend={backend}
      id={id}
      bounds={{ left, top: 190, width: 70, height: LABEL_HEIGHT + 40 }}
      label={{ text }}
      modulatable
      enabled={enabled}
    />
  );

  return (
    <PanelBox bounds={bounds}>
      <Toggle backend={backend} id="filter_enable" bounds={{ left: 10, top: 10, width: 30, height: 30 }} />
      <Label bounds={{ left: 50, top: 10, width: 80, height: 30 }} text="FILTER" bold size={16} align="left" enabled={enabled} />

      <Label bounds={{ left: 10, top: 45, width: 50, height: 25 }} text="Mode" align="left" enabled={enabled} />
      <ChoiceBox backend={backend} id="filter_mode" bounds={{ left: 60, top: 45, width: 110, height: 25 }} enabled={enabled} />
      <ChoiceBox backend={backend} id="filter_slope" bounds={{ left: 175, top: 45, width: 75, height: 25 }} enabled={slopeUsable} />

      <ResponseDisplay backend={backend} bounds={{ left: 10, top: 80, width: 240, height: 100 }} enabled={enabled} />

      {knob('drive', 'Drive', 15)}
      <PillToggle backend={backend} id="drive_hq" bounds={{ left: 65, top: 205, width: 20, height: 14 }} text="HQ" enabled={enabled} />
      {knob('filter_cutoff', 'Cutoff', 95)}
      {knob('filter_resonance', 'Resonance', 175)}
    </PanelBox>
  );
}
