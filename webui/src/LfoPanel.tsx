// Mirrors Source/ui/LFOPanel.h and CurveEditor.h
import { useMemo, useRef, useState, type PointerEvent as ReactPointerEvent } from 'react';
import { MAX_CURVE_POINTS, NUM_LFOS, type ModulateBackend, type Point } from './backend.ts';
import { ChoiceBox, ComboBox, Knob, Label, PanelBox, PillToggle, SourceTabs, type Rect } from './controls.tsx';
import { evaluate, pointsOrDefault, SHAPES } from './curves.ts';
import { CurveBackground, ControlPoint, curvePath, PLAYHEAD_COLOUR } from './curveDisplay.tsx';
import { useParam } from '@samplerealm/modules/editor/hooks.ts';
import { clamp01 } from '@samplerealm/modules/editor/params.ts';
import { useCurve, useLive } from './hooks.ts';
import { LABEL_HEIGHT, pointIn } from './layout.ts';

const RESOLUTION = 200;
const HIT_DISTANCE = 10;
const MIN_GAP = 0.01;

function CurveEditor({ backend, lfo, bounds }: { backend: ModulateBackend; lfo: number; bounds: Rect }) {
  const stored = useCurve(backend, lfo);
  const tension = useParam(backend, `lfo${lfo + 1}_tension`).value;
  const live = useLive(backend);
  const [hovered, setHovered] = useState<number | null>(null);
  const dragged = useRef<number | null>(null);

  const points = pointsOrDefault(stored);
  const { width, height } = bounds;
  const toPixels = ({ x, y }: Point) => [x * width, (1 - y) * height] as const;

  const positionOf = (e: ReactPointerEvent<SVGSVGElement>) => pointIn(e.currentTarget, width, e.clientX, e.clientY);

  const nearest = (at: { x: number; y: number }) => {
    let found: number | null = null;
    let closest = HIT_DISTANCE;
    points.forEach((point, i) => {
      const [px, py] = toPixels(point);
      const distance = Math.hypot(at.x - px, at.y - py);
      if (distance < closest) {
        closest = distance;
        found = i;
      }
    });
    return found;
  };

  const save = (next: Point[]) => backend.setCurve(lfo, next);

  const curve = useMemo(() => curvePath(RESOLUTION, width, height, (x) => evaluate(points, tension, x)), [points, tension, width, height]);
  const phase = live?.lfoPhase[lfo];

  return (
    <svg
      className="absolute cursor-pointer touch-none"
      style={{ left: bounds.left, top: bounds.top }}
      width={width}
      height={height}
      aria-label={`LFO ${lfo + 1} shape`}
      onPointerDown={(e) => {
        if (e.button !== 0) return;
        const at = positionOf(e);
        const index = nearest(at);

        if (e.detail === 2) {
          if (index !== null && index > 0 && index < points.length - 1) save(points.filter((_, i) => i !== index));
          else if (index === null && points.length < MAX_CURVE_POINTS) {
            const added = { x: clamp01(at.x / width), y: clamp01(1 - at.y / height) };
            save([...points.filter((p) => p.x < added.x), added, ...points.filter((p) => p.x >= added.x)]);
          }
          return;
        }

        dragged.current = index;
        if (index !== null) e.currentTarget.setPointerCapture(e.pointerId);
      }}
      onPointerMove={(e) => {
        const at = positionOf(e);
        const index = dragged.current;

        if (index === null) {
          setHovered(nearest(at));
          return;
        }

        const y = clamp01(1 - at.y / height);
        const next = points.map((p) => ({ ...p }));
        const last = next.length - 1;

        // The ends stay at 0 and 1 and share a height, so the loop is seamless
        if (index === 0) next[last]!.y = y;
        if (index === last) next[0]!.y = y;
        const x = index === 0 ? 0 : index === last ? 1 : Math.min(next[index + 1]!.x - MIN_GAP, Math.max(next[index - 1]!.x + MIN_GAP, at.x / width));

        next[index] = { x, y };
        save(next);
      }}
      onPointerUp={() => (dragged.current = null)}
      onPointerLeave={() => dragged.current === null && setHovered(null)}
    >
      <CurveBackground width={width} height={height} centreLine />
      <path d={`${curve} L ${width} ${height} L 0 ${height} Z`} fill="var(--color-accent)" fillOpacity={0.2} />
      <path d={curve} stroke="var(--color-accent)" strokeWidth={2} fill="none" />
      {phase !== undefined && <line x1={phase * width} x2={phase * width} y1={0} y2={height} stroke={PLAYHEAD_COLOUR} strokeWidth={2} />}
      {points.map((point, i) => {
        const [x, y] = toPixels(point);
        return <ControlPoint key={i} x={x} y={y} highlighted={i === hovered || i === dragged.current} />;
      })}
    </svg>
  );
}

function LfoContent({ backend, lfo }: { backend: ModulateBackend; lfo: number }) {
  const id = (name: string) => `lfo${lfo + 1}_${name}`;
  const synced = useParam(backend, id('sync')).value >= 0.5;
  const tension = useParam(backend, id('tension'));

  const applyShape = (index: number) => {
    const shape = SHAPES[index]!;
    tension.commit(shape.tension);
    backend.setCurve(lfo, shape.points.map((p) => ({ ...p })));
  };

  return (
    <>
      <CurveEditor backend={backend} lfo={lfo} bounds={{ left: 10, top: 10, width: 330, height: 137 }} />

      {/* Behaves as a menu: the curve stays editable, so no name stays showing */}
      <ComboBox
        bounds={{ left: 15, top: 162, width: 155, height: 20 }}
        text="Shape"
        items={SHAPES.map((shape, index) => ({ kind: 'item', label: shape.name, onSelect: () => applyShape(index) }))}
      />
      <Label bounds={{ left: 15, top: 186, width: 38, height: 20 }} text="Mode" colour="var(--color-accent)" align="left" />
      <ChoiceBox backend={backend} id={id('mode')} bounds={{ left: 53, top: 186, width: 117, height: 20 }} />

      <Knob
        key={synced ? 'sync' : 'free'}
        backend={backend}
        id={synced ? id('sync_rate') : id('rate')}
        bounds={{ left: 185, top: 147, width: 50, height: LABEL_HEIGHT + 50 }}
        label={{ text: 'Rate', colour: 'var(--color-accent)' }}
        modulatable
      />
      <PillToggle backend={backend} id={id('sync')} bounds={{ left: 245, top: 167, width: 35, height: 12 }} text="SYNC" />
      <Knob
        backend={backend}
        id={id('tension')}
        bounds={{ left: 290, top: 147, width: 50, height: LABEL_HEIGHT + 50 }}
        label={{ text: 'Tension', colour: 'var(--color-accent)' }}
      />
    </>
  );
}

export function LfoPanel({ backend, bounds }: { backend: ModulateBackend; bounds: Rect }) {
  const [active, setActive] = useState(0);
  const names = Array.from({ length: NUM_LFOS }, (_, i) => `LFO ${i + 1}`);

  return (
    <PanelBox bounds={bounds}>
      <SourceTabs source="lfo" names={names} width={bounds.width} active={active} onSelect={setActive}>
        <LfoContent key={active} backend={backend} lfo={active} />
      </SourceTabs>
    </PanelBox>
  );
}
