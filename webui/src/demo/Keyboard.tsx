import { useRef } from 'react';
import { qwertyLabel } from '@samplerealm/modules/demo/qwerty.ts';

const BLACK_SEMITONES = new Set([1, 3, 6, 8, 10]);
const isBlack = (note: number) => BLACK_SEMITONES.has(note % 12);

const KEY = 'absolute top-0 flex items-end justify-center rounded-b-md border text-2xs text-dim';

const noteAt = (x: number, y: number) => {
  const note = (document.elementFromPoint(x, y) as HTMLElement | null)?.dataset.note;
  return note === undefined ? undefined : Number(note);
};

interface KeyboardProps {
  lowNote: number;
  numKeys: number;
  active: ReadonlySet<number>;
  onNoteOn: (note: number) => void;
  onNoteOff: (note: number) => void;
}

export function Keyboard({ lowNote, numKeys, active, onNoteOn, onNoteOff }: KeyboardProps) {
  const held = useRef(new Map<number, number>());

  const notes = Array.from({ length: numKeys }, (_, i) => lowNote + i);
  const whites = notes.filter((note) => !isBlack(note));
  const whiteWidth = 100 / whites.length;

  const release = (pointerId: number) => {
    const note = held.current.get(pointerId);
    if (note === undefined) return;
    held.current.delete(pointerId);
    onNoteOff(note);
  };

  return (
    <div
      className="relative mt-2.5 h-27.5 touch-none select-none"
      onPointerDown={(e) => {
        const note = noteAt(e.clientX, e.clientY);
        if (note === undefined) return;
        e.currentTarget.setPointerCapture(e.pointerId);
        held.current.set(e.pointerId, note);
        onNoteOn(note);
      }}
      onPointerMove={(e) => {
        const from = held.current.get(e.pointerId);
        const to = noteAt(e.clientX, e.clientY);
        if (from === undefined || to === undefined || to === from) return;
        // Note on before note off, so it glides
        held.current.set(e.pointerId, to);
        onNoteOn(to);
        onNoteOff(from);
      }}
      onPointerUp={(e) => release(e.pointerId)}
      onPointerCancel={(e) => release(e.pointerId)}
    >
      {whites.map((note, i) => (
        <div
          key={note}
          data-note={note}
          className={`${KEY} bottom-0 border-background pb-1.5 ${active.has(note) ? 'bg-accent' : 'bg-fg'}`}
          style={{ left: `${i * whiteWidth}%`, width: `${whiteWidth}%` }}
        >
          <span className="pointer-events-none max-[480px]:hidden">{qwertyLabel(note - lowNote)}</span>
        </div>
      ))}
      {notes.filter(isBlack).map((note) => {
        const whitesBefore = whites.filter((white) => white < note).length;
        return (
          <div
            key={note}
            data-note={note}
            className={`${KEY} z-1 h-3/5 border-widget-border pb-1 ${active.has(note) ? 'bg-accent' : 'bg-panel'}`}
            style={{ left: `${(whitesBefore - 0.3) * whiteWidth}%`, width: `${whiteWidth * 0.6}%` }}
          >
            <span className="pointer-events-none max-[480px]:hidden">{qwertyLabel(note - lowNote)}</span>
          </div>
        );
      })}
    </div>
  );
}
