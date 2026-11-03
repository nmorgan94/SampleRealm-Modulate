// Index is semitones above the lowest C
const QWERTY: readonly (readonly [code: string, label: string])[] = [
  ['KeyA', 'A'], ['KeyW', 'W'], ['KeyS', 'S'], ['KeyE', 'E'], ['KeyD', 'D'], ['KeyF', 'F'],
  ['KeyT', 'T'], ['KeyG', 'G'], ['KeyY', 'Y'], ['KeyH', 'H'], ['KeyU', 'U'], ['KeyJ', 'J'],
  ['KeyK', 'K'], ['KeyO', 'O'], ['KeyL', 'L'], ['KeyP', 'P'], ['Semicolon', ';'], ['Quote', "'"],
];

export const qwertySemitone = (code: string): number | undefined => {
  const index = QWERTY.findIndex(([key]) => key === code);
  return index < 0 ? undefined : index;
};

export const qwertyLabel = (semitone: number): string | undefined => QWERTY[semitone]?.[1];
