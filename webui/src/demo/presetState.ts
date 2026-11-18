// What a .modulate preset holds beside its parameters, for the shared workletBackend's extraState
import { xmlTags } from '@samplerealm/modules/demo/localPresets.ts';
import { NUM_LFOS, SOURCE_NUMBERS, type Modulation, type ModulationSource, type Point } from '../backend.ts';

const SOURCES = Object.keys(SOURCE_NUMBERS) as ModulationSource[];

// Each plugin's demo shares the site's storage
export const STORAGE_KEY = 'samplerealm-modulate-presets';

export interface ExtraState {
  modulation: Record<string, Modulation>;
  curves: Point[][];
}

// The ModMatrix and LFOCurves children PresetManager writes beside the parameters
export function parseExtraState(xml: string): ExtraState {
  const modulation: Record<string, Modulation> = {};
  for (const { parameterID, sourceType, sourceIndex, minValue, maxValue } of xmlTags(xml, 'Assignment')) {
    const source = SOURCES.find((candidate) => SOURCE_NUMBERS[candidate] === Number(sourceType));
    if (parameterID && source) modulation[parameterID] = { source, index: Number(sourceIndex), min: Number(minValue), max: Number(maxValue) };
  }

  const curves: Point[][] = Array.from({ length: NUM_LFOS }, () => []);
  // An LFO with no points is written self-closing
  for (const [element, body = ''] of xml.matchAll(/<LFOCurve\s[^>]*?(?:\/>|>([\s\S]*?)<\/LFOCurve>)/g)) {
    const lfo = Number(xmlTags(element, 'LFOCurve')[0]?.index);
    if (lfo >= 0 && lfo < NUM_LFOS) curves[lfo] = xmlTags(body, 'Point').map(({ x, y }) => ({ x: Number(x), y: Number(y) }));
  }

  return { modulation, curves };
}
