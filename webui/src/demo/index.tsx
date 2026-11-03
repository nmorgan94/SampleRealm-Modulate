import { useCallback, useEffect, useRef, useState } from 'react';
import { createRoot } from 'react-dom/client';
import { Keyboard } from './Keyboard.tsx';
import { qwertySemitone } from './qwerty.ts';
import { EDITOR_HEIGHT, EDITOR_WIDTH, ModulateEditor } from '../ModulateEditor.tsx';
import { loadEngine, workletBackend, type WorkletBackend } from './workletBackend.ts';
import './styles.css';

const ENGINE_URL = 'engine.wasm';
const PROCESSOR_URL = 'processor.js';

// C3
const LOW_NOTE = 48;
const NUM_KEYS = 25;

const SIZE_MESSAGE = 'samplerealm-demo-size';

const isTyping = (target: EventTarget | null) =>
  target instanceof HTMLElement && (target.isContentEditable || /^(INPUT|TEXTAREA|SELECT)$/.test(target.tagName));

async function startAudio(backend: WorkletBackend, wasm: ArrayBuffer, audio: AudioContext) {
  await audio.audioWorklet.addModule(PROCESSOR_URL);

  const node = new AudioWorkletNode(audio, 'modulate-processor', {
    numberOfInputs: 0,
    outputChannelCount: [2],
    processorOptions: { wasm },
  });

  await new Promise<void>((resolve, reject) => {
    node.port.onmessage = ({ data }) => {
      if (data.type === 'ready') resolve();
      else if (data.type === 'error') reject(new Error(data.message));
    };
  });

  node.connect(audio.destination);
  backend.attach(node.port);
}

function useScale() {
  const [scale, setScale] = useState(1);
  const ref = useRef<HTMLDivElement>(null);

  useEffect(() => {
    const element = ref.current;
    if (!element) return;
    const observer = new ResizeObserver(() => setScale(Math.min(1, element.clientWidth / EDITOR_WIDTH)));
    observer.observe(element);
    return () => observer.disconnect();
  }, []);

  return { ref, scale };
}

function Demo() {
  const [engine, setEngine] = useState<{ backend: WorkletBackend; wasm: ArrayBuffer } | null>(null);
  const [status, setStatus] = useState<'loading' | 'on' | 'failed'>('loading');
  const [active, setActive] = useState<ReadonlySet<number>>(new Set());
  const [midi, setMidi] = useState<MIDIAccess | null>(null);
  const audio = useRef<AudioContext | null>(null);
  const starting = useRef<Promise<void> | null>(null);
  const { ref, scale } = useScale();
  const editor = useRef<HTMLDivElement>(null);

  // Not the document's height, which never shrinks below the frame. The editor's box is measured,
  // as its scale catches up with the frame's width a render late
  useEffect(() => {
    const content = ref.current;
    const box = editor.current;
    if (window.parent === window || !content || !box) return;

    const report = () => {
      const below = Math.round(content.getBoundingClientRect().height - box.getBoundingClientRect().height);
      window.parent.postMessage({ type: SIZE_MESSAGE, editorWidth: EDITOR_WIDTH, editorHeight: EDITOR_HEIGHT, below }, '*');
    };
    const observer = new ResizeObserver(report);
    observer.observe(content);
    return () => observer.disconnect();
  }, [ref]);

  useEffect(() => {
    loadEngine(ENGINE_URL)
      .then(({ wasm, info }) => setEngine({ backend: workletBackend(info), wasm }))
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
      .then(() => setStatus('on'))
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
    <div className="mx-auto max-w-225" ref={ref}>
      <div className="relative overflow-hidden rounded-xl" ref={editor} style={{ width: EDITOR_WIDTH * scale, height: EDITOR_HEIGHT * scale }}>
        <div className="origin-top-left" style={{ width: EDITOR_WIDTH, height: EDITOR_HEIGHT, transform: `scale(${scale})` }}>
          {engine && <ModulateEditor backend={engine.backend} />}
        </div>
        {status !== 'on' && (
          <div className="absolute inset-0 z-60 flex items-center justify-center bg-background/55">
            {status === 'failed' ? (
              <p className="text-sm text-red-400">The demo couldn't start. Please try again later.</p>
            ) : (
              <p className="font-display text-sm font-bold text-accent">Loading…</p>
            )}
          </div>
        )}
      </div>

      <Keyboard lowNote={LOW_NOTE} numKeys={NUM_KEYS} active={active} onNoteOn={noteOn} onNoteOff={noteOff} />

      {status === 'on' && !midi && 'requestMIDIAccess' in navigator && (
        <button
          type="button"
          className="mt-2.5 h-7 cursor-pointer rounded-sm border border-widget-border bg-widget px-2 font-display text-xs hover:text-accent"
          onClick={connectMidi}
        >
          Use a MIDI keyboard
        </button>
      )}
    </div>
  );
}

const root = document.getElementById('root');
if (root) createRoot(root).render(<Demo />);
