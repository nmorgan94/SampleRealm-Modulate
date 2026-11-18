// The shared worklet backend, with Modulate's modulation, curves and live state
import type { PresetStorage } from '@samplerealm/modules/demo/localPresets.ts';
import { workletBackend as sharedBackend, type WorkletBackend as SharedBackend } from '@samplerealm/modules/demo/workletBackend.ts';
import {
  listenerSet,
  MAX_CURVE_POINTS,
  NUM_LFOS,
  toRouting,
  type EnvelopeStage,
  type Live,
  type ModulateBackend,
  type Modulation,
  type Point,
  type UiInfo,
} from '../backend.ts';
import { parseExtraState, STORAGE_KEY, type ExtraState } from './presetState.ts';

export type WorkletBackend = ModulateBackend & SharedBackend;

// processor.js posts the engine's live() about this often, as its `frames`
export const LIVE_FRAMES = { name: 'live', size: 'liveSize', perSecond: 30 };
const LIVE_GROUPS = 5;

export function toLive(frame: Float32Array): Live {
  const values = Array.from(frame);
  const size = values.length / LIVE_GROUPS;
  const group = (index: number) => values.slice(index * size, (index + 1) * size);

  return {
    lfoPhase: group(0),
    lfoValue: group(1),
    envelopeLevel: group(2),
    envelopeProgress: group(3),
    envelopeStage: group(4) as EnvelopeStage[],
  };
}

interface Options {
  // The factory preset files' text
  factoryPresets?: string[];
  // The page's own unless given
  storage?: PresetStorage;
}

export function workletBackend(info: UiInfo, { factoryPresets, storage }: Options = {}): WorkletBackend {
  const indexOf = new Map(info.params.map(({ id }, index) => [id, index]));
  const modulations = new Map<string, Modulation>();
  const curves: Point[][] = Array.from({ length: NUM_LFOS }, () => []);
  const modulationListeners = listenerSet<[]>();
  const curveListeners = listenerSet<[lfo: number]>();
  const liveListeners = listenerSet<[live: Live]>();

  // Doesn't notify, so a preset load can notify once for all of them
  const storeModulation = (id: string, modulation: Modulation | null) => {
    const index = indexOf.get(id);
    if (index === undefined) return;

    if (modulation) modulations.set(id, modulation);
    else modulations.delete(id);

    const { source, sourceIndex, min, max } = toRouting(modulation);
    shared.post({ type: 'call', name: 'setModulation', args: [index, source, sourceIndex, min, max] });
  };

  const applyCurve = (lfo: number, points: Point[]) => {
    // The engine's points() holds no more
    const kept = points.slice(0, MAX_CURVE_POINTS);
    curves[lfo] = kept.map(({ x, y }) => ({ x, y }));
    shared.post({ type: 'call', name: 'setCurve', args: [lfo, kept.length], floats: { at: 'points', values: kept.flatMap(({ x, y }) => [x, y]) } });
    curveListeners.notify(lfo);
  };

  const shared = sharedBackend<ExtraState>(info, {
    storageKey: STORAGE_KEY,
    factoryPresets,
    storage,
    extraState: {
      parse: parseExtraState,
      capture: () => ({
        modulation: Object.fromEntries(modulations),
        curves: curves.map((points) => points.map((point) => ({ ...point }))),
      }),
      apply: ({ modulation = {}, curves: stored = [] }) => {
        for (const { id } of info.params) storeModulation(id, modulation[id] ?? null);
        modulationListeners.notify();
        for (let lfo = 0; lfo < NUM_LFOS; lfo++) applyCurve(lfo, stored[lfo] ?? []);
      },
    },
  });

  shared.onMessage((data) => {
    if (data.type === 'frame' && data.frame instanceof Float32Array) liveListeners.notify(toLive(data.frame));
  });

  return {
    ...shared,
    getModulation: (id) => modulations.get(id) ?? null,
    setModulation: (id, modulation) => {
      storeModulation(id, modulation);
      modulationListeners.notify();
      shared.markModified();
    },
    onModulation: modulationListeners.add,
    getCurve: (lfo) => curves[lfo] ?? [],
    setCurve: (lfo, points) => {
      applyCurve(lfo, points);
      shared.markModified();
    },
    onCurve: curveListeners.add,
    onLive: liveListeners.add,
  };
}
