// The C++ side of these names is Source/webview/ModulateWebView.cpp
import { getNativeFunction } from '@juce-framework/webview';
import { initialData, onEvent, pluginBackend } from '@samplerealm/modules/plugin/pluginBackend.ts';
import { listenerSet, NUM_LFOS, toRouting, type Live, type ModulateBackend, type Modulation, type Point } from '../backend.ts';

export function modulateBackend(): ModulateBackend {
  let modulations = new Map(Object.entries(initialData<Record<string, Modulation>>('modulation')));
  let curves = initialData<Point[][]>('curves');
  const modulationListeners = listenerSet<[]>();
  const curveListeners = listenerSet<[lfo: number]>();

  // Whatever changed them: this page, a preset, or the host restoring a session
  onEvent<Record<string, Modulation>>('modulation', (byId) => {
    modulations = new Map(Object.entries(byId));
    modulationListeners.notify();
  });
  onEvent<Point[][]>('curves', (next) => {
    curves = next;
    for (let lfo = 0; lfo < NUM_LFOS; lfo++) curveListeners.notify(lfo);
  });

  return {
    ...pluginBackend(),
    getModulation: (id) => modulations.get(id) ?? null,
    setModulation: (id, modulation) => {
      if (modulation) modulations.set(id, modulation);
      else modulations.delete(id);
      modulationListeners.notify();
      const { source, sourceIndex, min, max } = toRouting(modulation);
      void getNativeFunction('setModulation')(id, source, sourceIndex, min, max);
    },
    onModulation: modulationListeners.add,
    getCurve: (lfo) => curves[lfo] ?? [],
    setCurve: (lfo, points) => {
      curves = curves.map((existing, i) => (i === lfo ? points : existing));
      curveListeners.notify(lfo);
      void getNativeFunction('setCurve')(lfo, points.flatMap(({ x, y }) => [x, y]));
    },
    onCurve: curveListeners.add,
    onLive: (listener) => onEvent<Live>('live', listener),
  };
}
