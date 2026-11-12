import type { ParamInfo } from '@samplerealm/modules/backend.ts';
import { parseValue } from '@samplerealm/modules/editor/params.ts';

// As LabeledKnob's typed values: a choice takes only one of its names
export const parseKnobValue = (param: ParamInfo, text: string): number | null => {
  if (param.kind !== 'choice') return parseValue(param, text);

  const typed = text.trim().toLowerCase();
  const index = param.choices.findIndex((choice) => choice.toLowerCase() === typed);
  return index < 0 ? null : index;
};
