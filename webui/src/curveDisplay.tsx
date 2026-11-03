// Mirrors Source/ui/BaseCurveDisplay.h
export const PLAYHEAD_COLOUR = '#55ff80';

export function CurveBackground({ width, height, centreLine = false }: { width: number; height: number; centreLine?: boolean }) {
  return (
    <>
      <rect width={width} height={height} rx={4} fill="var(--color-display)" />
      {[1, 2, 3, 4, 5, 6, 7].map((i) => (
        <line key={`v${i}`} x1={(width * i) / 8} x2={(width * i) / 8} y1={0} y2={height} stroke="var(--color-widget)" />
      ))}
      {[1, 2, 3].map((i) => (
        <line key={`h${i}`} x1={0} x2={width} y1={(height * i) / 4} y2={(height * i) / 4} stroke="var(--color-widget)" />
      ))}
      {centreLine && <line x1={0} x2={width} y1={height / 2} y2={height / 2} stroke="var(--color-widget-border)" strokeWidth={1.5} />}
    </>
  );
}

export function ControlPoint({ x, y, highlighted = false }: { x: number; y: number; highlighted?: boolean }) {
  return (
    <>
      <circle cx={x} cy={y} r={6} fill="var(--color-accent)" />
      <circle cx={x} cy={y} r={4} fill={highlighted ? 'var(--color-fg)' : 'var(--color-display)'} />
    </>
  );
}

// value(x) is 0..1 for x in 0..1, drawn with 1 at the top
export const curvePath = (resolution: number, width: number, height: number, value: (x: number) => number) =>
  Array.from({ length: resolution + 1 }, (_, i) => {
    const x = i / resolution;
    return `${i === 0 ? 'M' : 'L'} ${x * width} ${(1 - value(x)) * height}`;
  }).join(' ');
