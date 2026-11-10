// Mirrors Source/ui/EnvelopePanel.h and EnvelopeEditor.h
import { useEffect, useState } from 'react';
import { EnvelopeStage, NUM_ENVELOPES, type ModulateBackend } from './backend.ts';
import { Knob, PanelBox, SourceTabs, type Rect } from './controls.tsx';
import { CurveBackground, ControlPoint, PLAYHEAD_COLOUR } from './curveDisplay.tsx';
import { juceHeight } from './fonts.ts';
import { LABEL_HEIGHT } from './layout.ts';
import { useLive, useParam } from './hooks.ts';
import { clamp01 } from './params.ts';

const SUSTAIN_DISPLAY_TIME = 0.3;
const STAGES = ['attack', 'decay', 'sustain', 'release'] as const;
const STAGE_NAMES = ['Attack', 'Decay', 'Sustain', 'Release'];

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
  const [attack, decay, sustain, release] = STAGES.map((stage) => useParam(backend, `env${envelope + 1}_${stage}`).value) as [number, number, number, number];
  // Envelope 1 always runs, for this playhead
  const shown = useEnvelopeAssigned(backend, envelope) || envelope === 0;
  const live = useLive(backend, shown);

  const { width, height } = bounds;
  const [a, d, s, r] = [Math.max(0.001, attack), Math.max(0.001, decay), clamp01(sustain), Math.max(0.001, release)];
  const total = a + d + SUSTAIN_DISPLAY_TIME + r;
  const attackX = (a / total) * width;
  const decayX = attackX + (d / total) * width;
  const sustainX = decayX + (SUSTAIN_DISPLAY_TIME / total) * width;
  const sustainY = height - s * height;
  const curve = `M 0 ${height} L ${attackX} 0 L ${decayX} ${sustainY} L ${sustainX} ${sustainY} L ${width} ${height}`;

  const stage = live?.envelopeStage[envelope] ?? EnvelopeStage.Idle;
  const progress = live?.envelopeProgress[envelope] ?? 0;
  const playheadX =
    stage === EnvelopeStage.Attack ? progress * attackX
    : stage === EnvelopeStage.Decay ? attackX + progress * (decayX - attackX)
    : stage === EnvelopeStage.Sustain ? decayX
    : stage === EnvelopeStage.Release ? sustainX + progress * (width - sustainX)
    : null;

  const labels: [string, number, number][] = [['A', 0, attackX], ['D', attackX, decayX], ['S', decayX, sustainX], ['R', sustainX, width]];

  return (
    <svg className="absolute" style={{ left: bounds.left, top: bounds.top }} width={width} height={height} aria-label={`Envelope ${envelope + 1}`}>
      <CurveBackground width={width} height={height} />
      <path d={`${curve} Z`} fill="var(--color-accent)" fillOpacity={0.2} />
      <path d={curve} stroke="var(--color-accent)" strokeWidth={2} fill="none" />
      <ControlPoint x={attackX} y={0} />
      <ControlPoint x={decayX} y={sustainY} />
      <ControlPoint x={sustainX} y={sustainY} />
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
