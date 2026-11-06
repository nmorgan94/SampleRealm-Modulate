// The C++ side of these names is Source/webview/WebViewEditor.cpp
import { getNativeFunction, getSliderState } from '@juce-framework/webview';
import {
  listenerSet,
  NUM_LFOS,
  paramLookup,
  toRouting,
  type Level,
  type Live,
  type ModulateBackend,
  type Modulation,
  type Point,
  type PresetBackend,
  type PresetEntry,
  type PresetStatus,
  type UiInfo,
} from '../backend.ts';
import { toProportion } from '../params.ts';

const call = async <T>(name: string, ...args: unknown[]) => (await getNativeFunction(name)(...args)) as T;

const listen = <T>(event: string, listener: (payload: T) => void) => {
  const handle = window.__JUCE__.backend.addEventListener(event, (payload) => listener(payload as T));
  return () => window.__JUCE__.backend.removeEventListener(handle);
};

// JUCE's type lists only its own initialisation data, and wraps each item in an array
const initial = <T>(name: string) => (window.__JUCE__.initialisationData as unknown as Record<string, T[]>)[name]![0]!;

function pluginPresets(): PresetBackend {
  let status = JSON.parse(initial<string>('presetStatus')) as PresetStatus;
  const listeners = listenerSet<[status: PresetStatus]>();

  const update = (next: PresetStatus) => {
    status = next;
    listeners.notify(status);
  };

  // Asked again once listening, in case a change came first
  listen<PresetStatus>('presetStatus', update);
  call<PresetStatus>('presetStatus').then(update);
  const error = async (result: Promise<string>) => (await result) || null;

  return {
    status: () => status,
    onStatus: listeners.add,
    list: () => call<PresetEntry[]>('presetList'),
    load: async (preset) => void (await call('presetLoad', preset)),
    loadInit: async () => void (await call('presetInit')),
    loadNext: async () => void (await call('presetStep', 1)),
    loadPrevious: async () => void (await call('presetStep', -1)),
    exists: (name) => call<boolean>('presetExists', name),
    save: (name, category) => error(call<string>('presetSave', name, category)),
    rename: (preset, newName) => error(call<string>('presetRename', preset, newName)),
    remove: (preset) => error(call<string>('presetDelete', preset)),
    showFolder: () => void call('presetShowFolder'),
  };
}

export function pluginBackend(): ModulateBackend {
  const info = JSON.parse(initial<string>('uiInfo')) as UiInfo;
  const entry = paramLookup(new Map(info.params.map((param) => [param.id, { param, slider: getSliderState(param.id) }])));
  let modulations = new Map(Object.entries(initial<Record<string, Modulation>>('modulation')));
  let curves = initial<Point[][]>('curves');
  const modulationListeners = listenerSet<[]>();
  const curveListeners = listenerSet<[lfo: number]>();

  const param = (id: string) => entry(id).param;
  const slider = (id: string) => entry(id).slider;

  // Whatever changed them: this page, a preset, or the host restoring a session
  listen<Record<string, Modulation>>('modulation', (byId) => {
    modulations = new Map(Object.entries(byId));
    modulationListeners.notify();
  });
  listen<Point[][]>('curves', (next) => {
    curves = next;
    for (let lfo = 0; lfo < NUM_LFOS; lfo++) curveListeners.notify(lfo);
  });

  return {
    info,
    about: { version: info.version, commit: info.commit, build: info.build, running: initial<string>('running') },
    presets: pluginPresets(),
    param,
    getValue: (id) => slider(id).getScaledValue(),
    setValue: (id, value) => slider(id).setNormalisedValue(toProportion(param(id), value)),
    beginGesture: (id) => slider(id).sliderDragStarted(),
    endGesture: (id) => slider(id).sliderDragEnded(),
    subscribe: (id, listener) => {
      const state = slider(id);
      const handle = state.valueChangedEvent.addListener(() => listener(state.getScaledValue()));
      return () => state.valueChangedEvent.removeListener(handle);
    },
    getModulation: (id) => modulations.get(id) ?? null,
    setModulation: (id, modulation) => {
      if (modulation) modulations.set(id, modulation);
      else modulations.delete(id);
      modulationListeners.notify();
      const { source, sourceIndex, min, max } = toRouting(modulation);
      call('setModulation', id, source, sourceIndex, min, max);
    },
    onModulation: modulationListeners.add,
    getCurve: (lfo) => curves[lfo] ?? [],
    setCurve: (lfo, points) => {
      curves = curves.map((existing, i) => (i === lfo ? points : existing));
      curveListeners.notify(lfo);
      call('setCurve', lfo, points.flatMap(({ x, y }) => [x, y]));
    },
    onCurve: curveListeners.add,
    onLevel: (listener) => listen<Level>('level', listener),
    onLive: (listener) => listen<Live>('live', listener),
  };
}
