// Mirrors Source/ui/EnvelopePanel.h and EnvelopeEditor.h
import { useEffect, useMemo, useState, type PointerEvent as ReactPointerEvent } from 'react';
import { EnvelopeStage, NUM_ENVELOPES, type ModulateBackend } from './backend.ts';
import { Knob, PanelBox, SourceTabs, type Rect } from './controls.tsx';
import { CurveBackground, ControlPoint, PLAYHEAD_COLOUR } from './curveDisplay.tsx';
import { pointIn } from './editorRoot.ts';
import { draggedCurve, envelopeShape } from './envelope.ts';
import { juceHeight } from './fonts.ts';
import { LABEL_HEIGHT } from './layout.ts';
import { useLive, useParam } from './hooks.ts';
import { snap } from './params.ts';

const STAGES = ['attack', 'decay', 'sustain', 'release'] as const;
const STAGE_NAMES = ['Attack', 'Decay', 'Sustain', 'Release'];
const CURVES = ['attack_curve', 'decay_curve', 'release_curve'] as const;
const HANDLE_RADIUS = 3;
const HIT_RADIUS = 8;

function useEnvelopeAssigned(backend: ModulateBackend, envelope: number) {
  const assigned = () => backend.info.params.some(({ id }) => {
    const modulation = backend.getModulation(id);
    return modulation?.source === 'env' && modulation.index === envelope;
  });
  const [isAssigned, setAssigned] = useState(assigned);

  useEffect(() => {
    setAssigned(assigned());
    return backend.onModulation(() => setAssigned(assigned()));
  }, [backend, envelope]);

  return isAssigned;
}

function EnvelopeEditor({ backend, envelope, bounds }: { backend: ModulateBackend; envelope: number; bounds: Rect }) {
  const id = (name: string) => `env${envelope + 1}_${name}`;
  const [attack, decay, sustain, release] = STAGES.map((stage) => useParam(backend, id(stage)).value) as [number, number, number, number];
  const curves = CURVES.map((curve) => useParam(backend, id(curve)));
  const [attackCurve, decayCurve, releaseCurve] = curves.map(({ value }) => value) as [number, number, number];
  // Envelope 1 always runs, for this playhead
  const shown = useEnvelopeAssigned(backend, envelope) || envelope === 0;
  const live = useLive(backend, shown);
  const [hovered, setHovered] = useState<number | null>(null);
  const [dragged, setDragged] = useState<{ index: number; startY: number; startCurve: number } | null>(null);

  const { width, height } = bounds;
  const { attackX, decayX, sustainX, sustainY, path, handles } = useMemo(
    () => envelopeShape({ attack, decay, sustain, release, curves: [attackCurve, decayCurve, releaseCurve] }, width, height),
    [attack, decay, sustain, release, attackCurve, decayCurve, releaseCurve, width, height]
  );

  const positionOf = (e: ReactPointerEvent<SVGSVGElement>) => pointIn(e.currentTarget, width, e.clientX, e.clientY);
  const nearest = (at: { x: number; y: number }) => {
    const distances = handles.map(({ x, y }) => Math.hypot(at.x - x, at.y - y));
    const closest = Math.min(...distances);
    return closest < HIT_RADIUS ? distances.indexOf(closest) : null;
  };

  const endDrag = (e: ReactPointerEvent<SVGSVGElement>) => {
    if (dragged === null) return;
    backend.endGesture(id(CURVES[dragged.index]));
    setDragged(null);
    const at = positionOf(e);
    const inside = at.x >= 0 && at.x <= width && at.y >= 0 && at.y <= height;
    setHovered(inside ? nearest(at) : null);
  };

  const stage = live?.envelopeStage[envelope] ?? EnvelopeStage.Idle;
  const progress = live?.envelopeProgress[envelope] ?? 0;
  const playheadX =
    stage === EnvelopeStage.Attack ? progress * attackX
    : stage === EnvelopeStage.Decay ? attackX + progress * (decayX - attackX)
    : stage === EnvelopeStage.Sustain ? decayX
    : stage === EnvelopeStage.Release ? sustainX + progress * (width - sustainX)
    : null;

  const labels: [string, number, number][] = [['A', 0, attackX], ['D', attackX, decayX], ['S', decayX, sustainX], ['R', sustainX, width]];
  const active = dragged?.index ?? hovered;

  return (
    <svg
      className="absolute touch-none"
      style={{ left: bounds.left, top: bounds.top, cursor: active === null ? undefined : 'ns-resize' }}
      width={width}
      height={height}
      aria-label={`Envelope ${envelope + 1}`}
      onPointerDown={(e) => {
        if (e.button !== 0) return;
        const at = positionOf(e);
        const index = nearest(at);
        if (index === null) return;

        if (e.detail === 2) {
          curves[index]!.commit(0);
          return;
        }

        backend.beginGesture(id(CURVES[index]));
        setDragged({ index, startY: at.y, startCurve: curves[index]!.value });
        e.currentTarget.setPointerCapture(e.pointerId);
      }}
      onPointerMove={(e) => {
        const at = positionOf(e);
        if (dragged === null) setHovered(nearest(at));
        else {
          const { set, param } = curves[dragged.index]!;
          set(snap(param, draggedCurve(dragged.index, dragged.startCurve, at.y - dragged.startY)));
        }
      }}
      onPointerUp={endDrag}
      onPointerCancel={endDrag}
      onPointerLeave={() => dragged === null && setHovered(null)}
    >
      <CurveBackground width={width} height={height} />
      <path d={`${path} Z`} fill="var(--color-accent)" fillOpacity={0.2} />
      <path d={path} stroke="var(--color-accent)" strokeWidth={2} fill="none" />
      <ControlPoint x={attackX} y={0} />
      <ControlPoint x={decayX} y={sustainY} />
      <ControlPoint x={sustainX} y={sustainY} />
      {handles.map(({ x, y }, i) => (
        <circle key={i} cx={x} cy={y} r={i === active ? HANDLE_RADIUS + 1.5 : HANDLE_RADIUS} fill="var(--color-accent)" fillOpacity={i === active ? 1 : 0.5} />
      ))}
      {playheadX !== null && <line x1={playheadX} x2={playheadX} y1={0} y2={height} stroke={PLAYHEAD_COLOUR} strokeWidth={2} />}
      {labels.map(([text, from, to]) => (
        <text key={text} x={(from + to) / 2} y={height - 9} fill="var(--color-accent)" fillOpacity={0.6} fontSize={juceHeight(10)} textAnchor="middle" dominantBaseline="middle">
          {text}
        </text>
      ))}
    </svg>
  );
}

function EnvelopeContent({ backend, envelope }: { backend: ModulateBackend; envelope: number }) {
  const knobSize = 50;
  const spacing = (330 - knobSize * 4) / 5;

  return (
    <>
      <EnvelopeEditor backend={backend} envelope={envelope} bounds={{ left: 10, top: 10, width: 330, height: 127 }} />
      {STAGES.map((stage, i) => {
        const left = 10 + spacing * (i + 1) + knobSize * i;
        return (
          <Knob
            key={stage}
            backend={backend}
            id={`env${envelope + 1}_${stage}`}
            bounds={{ left, top: 147, width: knobSize, height: LABEL_HEIGHT + knobSize }}
            label={{ text: STAGE_NAMES[i]!, colour: 'var(--color-accent)' }}
          />
        );
      })}
    </>
  );
}

export function EnvelopePanel({ backend, bounds }: { backend: ModulateBackend; bounds: Rect }) {
  const [active, setActive] = useState(0);
  const names = Array.from({ length: NUM_ENVELOPES }, (_, i) => `ENV ${i + 1}`);

  return (
    <PanelBox bounds={bounds}>
      <SourceTabs source="env" names={names} width={bounds.width} active={active} onSelect={setActive}>
        <EnvelopeContent key={active} backend={backend} envelope={active} />
      </SourceTabs>
    </PanelBox>
  );
}
