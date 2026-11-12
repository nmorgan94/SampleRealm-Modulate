import { useCallback, useEffect, useRef, useState } from 'react';
import { createRoot } from 'react-dom/client';
import { DemoFrame } from '@samplerealm/modules/demo/DemoFrame.tsx';
import { loadEngine, startWorklet } from '@samplerealm/modules/demo/engine.ts';
import { qwertySemitone } from '@samplerealm/modules/demo/qwerty.ts';
import { EDITOR_HEIGHT, EDITOR_WIDTH } from '../layout.ts';
import { ModulateEditor } from '../ModulateEditor.tsx';
import { factoryPresets } from './factoryPresets.ts';
import { Keyboard } from './Keyboard.tsx';
import { workletBackend, type WorkletBackend } from './workletBackend.ts';
import './styles.css';

const ENGINE_URL = 'engine.wasm';
// Modulate's own, in public/demo/
const PROCESSOR_URL = 'processor.js';

// C3
const LOW_NOTE = 48;
const NUM_KEYS = 25;

const isTyping = (target: EventTarget | null) =>
  target instanceof HTMLElement && (target.isContentEditable || /^(INPUT|TEXTAREA|SELECT)$/.test(target.tagName));

async function startAudio(backend: WorkletBackend, wasm: ArrayBuffer, context: AudioContext) {
  const node = await startWorklet(context, PROCESSOR_URL, {
    numberOfInputs: 0,
    outputChannelCount: [2],
    processorOptions: { wasm },
  });

  node.connect(context.destination);
  backend.attach(node.port);
}

function Demo() {
  const [engine, setEngine] = useState<{ backend: WorkletBackend; wasm: ArrayBuffer } | null>(null);
  const [status, setStatus] = useState<'loading' | 'ready' | 'failed'>('loading');
  const [active, setActive] = useState<ReadonlySet<number>>(new Set());
  const [midi, setMidi] = useState<MIDIAccess | null>(null);
  const audio = useRef<AudioContext | null>(null);
  const starting = useRef<Promise<void> | null>(null);

  useEffect(() => {
    loadEngine(ENGINE_URL)
      .then(({ wasm, info }) => setEngine({ backend: workletBackend(info, { factoryPresets }), wasm }))
      .catch((error) => {
        console.error('Demo engine failed to load:', error);
        setStatus('failed');
      });
  }, []);

  // Browsers only start audio from a click or key press
  const start = useCallback(() => {
    if (!engine) return Promise.resolve();
    if (starting.current) {
      // Wakes audio a browser held back when it started without a click
      audio.current?.resume();
      return starting.current;
    }

    const context = new AudioContext();
    context.resume();
    audio.current = context;

    starting.current = startAudio(engine.backend, engine.wasm, context)
      .then(() => setStatus('ready'))
      .catch((error) => {
        console.error('Demo audio failed to start:', error);
        context.close();
        audio.current = null;
        starting.current = null;
        setStatus('failed');
      });

    return starting.current;
  }, [engine]);

  // The site opens the demo from a click, which allow="autoplay" passes on
  useEffect(() => {
    start();
  }, [start]);

  const noteOn = useCallback(
    (note: number) => {
      setActive((current) => new Set(current).add(note));
      start();
      engine?.backend.noteOn(note);
    },
    [engine, start]
  );

  const noteOff = useCallback(
    (note: number) => {
      setActive((current) => {
        const next = new Set(current);
        next.delete(note);
        return next;
      });
      engine?.backend.noteOff(note);
    },
    [engine]
  );

  useEffect(() => {
    const held = new Map<string, number>();

    const down = (e: KeyboardEvent) => {
      const semitone = qwertySemitone(e.code);
      if (semitone === undefined || e.metaKey || e.ctrlKey || e.altKey || isTyping(e.target)) return;
      // ' opens Firefox's quick find
      e.preventDefault();
      if (e.repeat || held.has(e.code)) return;
      held.set(e.code, LOW_NOTE + semitone);
      noteOn(LOW_NOTE + semitone);
    };

    const up = (e: KeyboardEvent) => {
      const note = held.get(e.code);
      if (note === undefined) return;
      held.delete(e.code);
      noteOff(note);
    };

    // No keyup arrives once the frame loses focus
    const releaseAll = () => {
      held.forEach((note) => noteOff(note));
      held.clear();
    };

    window.addEventListener('keydown', down);
    window.addEventListener('keyup', up);
    window.addEventListener('blur', releaseAll);
    return () => {
      window.removeEventListener('keydown', down);
      window.removeEventListener('keyup', up);
      window.removeEventListener('blur', releaseAll);
      releaseAll();
    };
  }, [noteOn, noteOff]);

  const connectMidi = async () => {
    if (!engine) return;
    try {
      await start();
      const access = await navigator.requestMIDIAccess();
      const listen = (input: MIDIInput) => (input.onmidimessage = (e) => e.data && engine.backend.midi(e.data));
      access.inputs.forEach(listen);
      access.onstatechange = (e) => {
        if (e.port instanceof MIDIInput) listen(e.port);
      };
      setMidi(access);
    } catch (error) {
      console.error('MIDI access refused:', error);
    }
  };

  useEffect(
    () => () => {
      if (!midi) return;
      midi.onstatechange = null;
      midi.inputs.forEach((input) => (input.onmidimessage = null));
    },
    [midi]
  );

  useEffect(() => () => void audio.current?.close(), []);

  return (
    <DemoFrame width={EDITOR_WIDTH} height={EDITOR_HEIGHT} status={status} editor={engine && <ModulateEditor backend={engine.backend} />}>
      <Keyboard lowNote={LOW_NOTE} numKeys={NUM_KEYS} active={active} onNoteOn={noteOn} onNoteOff={noteOff} />

      {status === 'ready' && !midi && 'requestMIDIAccess' in navigator && (
        <button
          type="button"
          className="mt-2.5 h-7 cursor-pointer rounded-sm border border-widget-border bg-widget px-2 font-display text-xs hover:text-accent"
          onClick={connectMidi}
        >
          Use a MIDI keyboard
        </button>
      )}
    </DemoFrame>
  );
}

const root = document.getElementById('root');
if (root) createRoot(root).render(<Demo />);
