import { useCallback, useEffect, useState } from 'react';
import type { Live, ModulateBackend, Modulation, Point } from './backend.ts';

export function useParam(backend: ModulateBackend, id: string) {
  const [value, setLocal] = useState(() => backend.getValue(id));

  useEffect(() => {
    setLocal(backend.getValue(id));
    return backend.subscribe(id, setLocal);
  }, [backend, id]);

  const set = useCallback(
    (next: number) => {
      setLocal(next);
      backend.setValue(id, next);
    },
    [backend, id]
  );

  const commit = useCallback(
    (next: number) => {
      backend.beginGesture(id);
      set(next);
      backend.endGesture(id);
    },
    [backend, id, set]
  );

  return { value, set, commit, param: backend.param(id) };
}

export function useModulation(backend: ModulateBackend, id: string): Modulation | null {
  const [modulation, setModulation] = useState(() => backend.getModulation(id));

  useEffect(() => {
    setModulation(backend.getModulation(id));
    return backend.onModulation(() => setModulation(backend.getModulation(id)));
  }, [backend, id]);

  return modulation;
}

export function useCurve(backend: ModulateBackend, lfo: number): Point[] {
  const [points, setPoints] = useState(() => backend.getCurve(lfo));

  useEffect(() => {
    setPoints(backend.getCurve(lfo));
    return backend.onCurve((changed) => changed === lfo && setPoints(backend.getCurve(lfo)));
  }, [backend, lfo]);

  return points;
}

// Only listens while enabled, so most controls don't redraw 30 times a second
export function useLive(backend: ModulateBackend, enabled = true): Live | null {
  const [live, setLive] = useState<Live | null>(null);

  useEffect(() => (enabled ? backend.onLive(setLive) : undefined), [backend, enabled]);

  return enabled ? live : null;
}
