// Mirrors Source/ui/MasterMeter.h
import { useEffect, useRef, useState } from 'react';
import type { ModulateBackend } from './backend.ts';
import { at, Knob, PanelBox, type Rect } from './controls.tsx';
import { barChanged, emptyBar, FLOOR_DB, isClipping, updateBar, UPDATE_HZ, type Bar } from './meter.ts';

const CHANNELS = 2;

function useBars(backend: ModulateBackend) {
  const [bars, setBars] = useState<Bar[]>(() => Array.from({ length: CHANNELS }, emptyBar));
  // The loudest peak per channel since the last tick
  const pending = useRef<number[]>(new Array(CHANNELS).fill(0));

  useEffect(
    () =>
      backend.onLevel(({ peaks }) => {
        pending.current = pending.current.map((held, channel) => Math.max(held, peaks[channel] ?? 0));
      }),
    [backend]
  );

  useEffect(() => {
    const timer = window.setInterval(() => {
      const peaks = pending.current;
      pending.current = new Array(CHANNELS).fill(0);
      setBars((current) => {
        const next = peaks.map((peak, channel) => updateBar(current[channel] ?? emptyBar(), peak));
        return next.some((bar, channel) => barChanged(current[channel]!, bar)) ? next : current;
      });
    }, 1000 / UPDATE_HZ);
    return () => window.clearInterval(timer);
  }, []);

  return bars;
}

function MeterBar({ bounds, bar }: { bounds: Rect; bar: Bar }) {
  const fraction = (bar.levelDb - FLOOR_DB) / -FLOOR_DB;
  const inner = bounds.height - 2;

  return (
    <div className="border border-[#303030] bg-[#050505]" style={at(bounds)}>
      {fraction > 0.0001 && (
        <div className="absolute inset-x-0 bottom-0 bg-linear-to-t from-meter-low to-accent" style={{ height: inner * fraction }} />
      )}
      {isClipping(bar) && <div className="absolute -inset-x-px -top-px h-0.75 bg-red-600" />}
    </div>
  );
}

export function MasterMeter({ backend, bounds }: { backend: ModulateBackend; bounds: Rect }) {
  const bars = useBars(backend);

  return (
    <PanelBox bounds={bounds}>
      <Knob backend={backend} id="master_gain" bounds={{ left: 6, top: 7, width: 44, height: 44 }} />
      <MeterBar bounds={{ left: 60, top: 5, width: 8, height: 48 }} bar={bars[0]!} />
      <MeterBar bounds={{ left: 70, top: 5, width: 8, height: 48 }} bar={bars[1]!} />
    </PanelBox>
  );
}
