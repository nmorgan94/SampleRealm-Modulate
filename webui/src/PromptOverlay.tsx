// Mirrors Source/ui/PromptOverlay.h
import { useCallback, useEffect, useLayoutEffect, useMemo, useRef, useState } from 'react';
import { TextButton } from './controls.tsx';
import { juceHeight } from './fonts.ts';
import { Modal } from './Modal.tsx';

export interface Prompt {
  askForText(title: string, initialText: string, okText: string, onOk: (text: string) => void): void;
  askToConfirm(title: string, message: string, okText: string, onOk: () => void): void;
  showMessage(title: string, message: string): void;
}

interface Shown {
  // Each prompt starts afresh, even one that replaces another
  id: number;
  title: string;
  message: string;
  okText: string;
  initialText: string | null;
  canCancel: boolean;
  onOk: ((text: string) => void) | null;
}

const PANEL = { width: 380, height: 160 };
const MESSAGE_FONT = 12;
const BUTTON = { width: 90, height: 28 };

function Overlay({ shown, dismiss }: { shown: Shown; dismiss(): void }) {
  const input = useRef<HTMLInputElement>(null);
  const message = useRef<HTMLDivElement>(null);
  const [extra, setExtra] = useState(0);

  // Closes first, so the callback can open another prompt
  const accept = () => {
    const text = input.current?.value ?? '';
    dismiss();
    shown.onOk?.(text);
  };

  useEffect(() => input.current?.select(), [shown]);

  // As PromptOverlay::show: grow by what the message overflows
  useLayoutEffect(() => {
    const box = message.current;
    setExtra(box ? box.scrollHeight - box.clientHeight : 0);
  }, [shown]);

  return (
    <Modal
      label={shown.title}
      titleInset={{ x: 20, y: 16 }}
      panel={{ ...PANEL, height: PANEL.height + extra, maxHeight: '100%' }}
      onClose={dismiss}
      // A focused button handles Enter itself
      onPanelKey={(e) => e.key === 'Enter' && !(e.target instanceof HTMLButtonElement) && accept()}
      focus={input}
    >
      {shown.initialText === null ? (
        <div
          ref={message}
          className="absolute overflow-hidden whitespace-pre-line"
          style={{ left: 20, top: 48, right: 20, bottom: 16 + BUTTON.height + 12, fontSize: juceHeight(MESSAGE_FONT), lineHeight: `${MESSAGE_FONT}px` }}
        >
          {shown.message}
        </div>
      ) : (
        <input
          ref={input}
          className="absolute border border-input-outline bg-input px-1 text-fg outline-none selection:bg-input-highlight focus:border-input-focus"
          style={{ left: 20, top: 48, width: PANEL.width - 40, height: 28, fontSize: juceHeight(14) }}
          defaultValue={shown.initialText}
        />
      )}
      {shown.canCancel && (
        <TextButton text="Cancel" style={{ right: 20 + BUTTON.width + 8, bottom: 16, ...BUTTON }} onClick={dismiss} />
      )}
      <TextButton text={shown.okText} style={{ right: 20, bottom: 16, ...BUTTON }} onClick={accept} />
    </Modal>
  );
}

export function usePrompt() {
  const [shown, setShown] = useState<Shown | null>(null);
  const count = useRef(0);
  const dismiss = useCallback(() => setShown(null), []);

  const prompt = useMemo<Prompt>(() => {
    const show = (prompt: Omit<Shown, 'id'>) => setShown({ ...prompt, id: ++count.current });

    return {
      askForText: (title, initialText, okText, onOk) => show({ title, message: '', okText, initialText, canCancel: true, onOk }),
      askToConfirm: (title, message, okText, onOk) => show({ title, message, okText, initialText: null, canCancel: true, onOk }),
      showMessage: (title, message) => show({ title, message, okText: 'OK', initialText: null, canCancel: false, onOk: null }),
    };
  }, []);

  const overlay = shown && <Overlay key={shown.id} shown={shown} dismiss={dismiss} />;
  return { prompt, overlay };
}
