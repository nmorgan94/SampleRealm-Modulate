// Mirrors Source/ui/LevelMeter.h
import { useEffect, useRef, useState } from 'react';
import type { PluginBackend } from './backend.ts';
import { centred } from './layout.ts';
import { barChanged, emptyBar, FLOOR_DB, isClipping, updateBar, UPDATE_HZ, type Bar } from './meter.ts';

const BAR_WIDTH = 12;
const GAP = 3;
const MAX_CHANNELS = 2;

const widthFor = (numChannels: number) => BAR_WIDTH * numChannels + GAP * (numChannels - 1);
export const PREFERRED_WIDTH = widthFor(MAX_CHANNELS);

function useBars(backend: PluginBackend) {
  const [bars, setBars] = useState<Bar[]>(() => Array.from({ length: MAX_CHANNELS }, emptyBar));
  // The loudest peak per channel since the last tick
  const pending = useRef<number[]>(new Array(MAX_CHANNELS).fill(0));

  useEffect(
    () =>
      backend.onLevel(({ peaks }) => {
        const next = peaks.slice(0, MAX_CHANNELS).map((peak, channel) => Math.max(peak, pending.current[channel] ?? 0));
        pending.current = next;
      }),
    [backend]
  );

  useEffect(() => {
    const timer = window.setInterval(() => {
      const peaks = pending.current;
      pending.current = new Array(peaks.length).fill(0);
      setBars((current) => {
        const next = peaks.map((peak, channel) => updateBar(current[channel] ?? emptyBar(), peak));
        const changed = next.length !== current.length || next.some((bar, channel) => barChanged(current[channel], bar));
        return changed ? next : current;
      });
    }, 1000 / UPDATE_HZ);
    return () => window.clearInterval(timer);
  }, []);

  return bars;
}

function BarView({ x, height, bar }: { x: number; height: number; bar: Bar }) {
  const left = x + 0.5;
  const width = BAR_WIDTH - 1;
  const top = 0.5 + 6 + 3;
  const bodyHeight = height - 1 - 6 - 3;
  const fraction = (bar.levelDb - FLOOR_DB) / -FLOOR_DB;
  const fillHeight = (bodyHeight - 2) * fraction;

  return (
    <g>
      <rect x={left} y={0.5} width={width} height={6} rx={2} fill={isClipping(bar) ? 'var(--color-clip)' : 'var(--color-line)'} />
      <rect x={left} y={top} width={width} height={bodyHeight} rx={2} fill="var(--color-background)" />
      {fillHeight > 0 && <rect x={left + 1} y={top + 1 + bodyHeight - 2 - fillHeight} width={width - 2} height={fillHeight} rx={1.5} fill="var(--color-thumb)" />}
      <rect x={left} y={top} width={width} height={bodyHeight} rx={2} fill="none" stroke="var(--color-line)" strokeWidth={1} />
    </g>
  );
}

export function LevelMeter({ backend, x, y, width, height }: { backend: PluginBackend; x: number; y: number; width: number; height: number }) {
  const bars = useBars(backend);
  const start = centred(width, widthFor(bars.length));

  return (
    <svg className="absolute" style={{ left: x, top: y }} width={width} height={height} aria-hidden>
      {bars.map((bar, channel) => (
        <BarView key={channel} x={start + channel * (BAR_WIDTH + GAP)} height={height} bar={bar} />
      ))}
    </svg>
  );
}
