// JUCE font heights are ascent + descent, CSS sizes are the em: Orbitron's is 1254 / 1000
export const juceHeight = (height: number) => (height * 1000) / 1254;
