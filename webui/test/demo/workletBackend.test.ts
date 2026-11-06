import assert from 'node:assert/strict';
import { test } from 'node:test';
import { MAX_CURVE_POINTS, type Level, type Live } from '../../src/backend.ts';
import { workletBackend, type EnginePort } from '../../src/demo/workletBackend.ts';
import { indexOf, info } from '../engine.ts';

const fakePort = () => {
  const sent: { type: string }[] = [];
  const port: EnginePort = { postMessage: (message) => sent.push(message), onmessage: null };
  const receive = (data: unknown) => port.onmessage?.({ data } as MessageEvent);
  return { port, sent, receive };
};

test('changes made before the worklet starts reach it in order, after the demo tempo', () => {
  const backend = workletBackend(info);
  backend.setValue('filter_cutoff', 2500);
  backend.setModulation('filter_cutoff', { source: 'env', index: 1, min: 0.2, max: 0.8 });
  backend.setCurve(2, [{ x: 0, y: 0.1 }, { x: 1, y: 0.1 }]);
  backend.noteOn(60);

  const { port, sent } = fakePort();
  backend.attach(port);

  assert.deepEqual(sent, [
    { type: 'tempo', bpm: 120 },
    { type: 'param', index: indexOf('filter_cutoff'), value: 2500 },
    { type: 'modulation', index: indexOf('filter_cutoff'), source: 2, sourceIndex: 1, min: 0.2, max: 0.8 },
    { type: 'curve', lfo: 2, points: [0, 0.1, 1, 0.1] },
    { type: 'midi', bytes: [0x90, 60, 100] },
  ]);

  backend.setModulation('filter_cutoff', null);
  assert.deepEqual(sent.at(-1), { type: 'modulation', index: indexOf('filter_cutoff'), source: 0, sourceIndex: 0, min: 0, max: 0 });
});

test("curves are cut to what the engine's points() holds", () => {
  const backend = workletBackend(info);
  const { port, sent } = fakePort();
  backend.attach(port);

  const long = Array.from({ length: MAX_CURVE_POINTS + 8 }, (_, i) => ({ x: i / (MAX_CURVE_POINTS + 7), y: 0.5 }));
  const kept = long.slice(0, MAX_CURVE_POINTS);
  backend.setCurve(0, long);

  assert.deepEqual(backend.getCurve(0), kept);
  assert.deepEqual(sent.at(-1), { type: 'curve', lfo: 0, points: kept.flatMap(({ x, y }) => [x, y]) });
});

test("the worklet's levels and live state reach their listeners", () => {
  const backend = workletBackend(info);
  const levels: Level[] = [];
  const lives: Live[] = [];
  backend.onLevel((level) => levels.push(level));
  backend.onLive((live) => lives.push(live));

  const { port, receive } = fakePort();
  backend.attach(port);

  const live: Live = { lfoPhase: [0.5, 0, 0, 0], lfoValue: [1, 0, 0, 0], envelopeLevel: [0.7, 0, 0, 0], envelopeProgress: [0, 0, 0, 0], envelopeStage: [3, 0, 0, 0] };
  receive({ type: 'level', peaks: [0.25, 0.5] });
  receive({ type: 'live', live });

  assert.deepEqual(levels, [{ peaks: [0.25, 0.5] }]);
  assert.deepEqual(lives, [live]);
});
