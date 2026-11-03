// Mirrors Source/ui/CustomLookAndFeel.h, ModulatableSlider, StepperDisplay, ValueLabelSlider and CustomLFOTabButton
import { createContext, useContext, useLayoutEffect, useRef, useState, type CSSProperties, type MouseEvent as ReactMouseEvent, type PointerEvent as ReactPointerEvent, type ReactNode } from 'react';
import type { ModulateBackend, Modulation, ModulationSource, ParamInfo } from './backend.ts';
import { pointIn, type Rect } from './editorRoot.ts';
import { juceHeight } from './fonts.ts';
import { useLive, useModulation, useParam } from './hooks.ts';
import { Menu, useMenu, type MenuItem } from './Menu.tsx';
import { applyDepthChange, modulatedValue, sourceValue } from './modulation.ts';
import { formatValue, fromProportion, snap, toProportion } from './params.ts';

export type { Rect };

export const at = ({ left, top, width, height }: Rect): CSSProperties => ({ position: 'absolute', left, top, width, height });

const DISABLED_ALPHA = 0.3;

// Dims and blocks a control, as the panels do when their section is off
export const dimmed = (enabled: boolean): CSSProperties => (enabled ? {} : { opacity: DISABLED_ALPHA, pointerEvents: 'none' });

//==============================================================================
// Angles run clockwise from 12 o'clock, as JUCE's do
const pointAt = (cx: number, cy: number, radius: number, angle: number) =>
  [cx + radius * Math.sin(angle), cy - radius * Math.cos(angle)] as const;

const arcPath = (cx: number, cy: number, radius: number, from: number, to: number) => {
  if (to - from < 1e-6) return '';
  const [x1, y1] = pointAt(cx, cy, radius, from);
  const [x2, y2] = pointAt(cx, cy, radius, to);
  return `M ${x1} ${y1} A ${radius} ${radius} 0 ${to - from > Math.PI ? 1 : 0} 1 ${x2} ${y2}`;
};

// drawRotarySlider's, which differ from the Slider's own that the modulation arc uses
const DRAWN_START = 1.25 * Math.PI;
const DRAWN_END = 2.75 * Math.PI;
const SLIDER_START = 1.2 * Math.PI;
const SLIDER_END = 2.8 * Math.PI;

const PIXELS_FOR_FULL_DRAG = 250;
const FINE_DRAG = 4;
const MOD_DRAG_PER_PIXEL = 0.003;
const STEPPER_PIXELS_PER_STEP = 3;

function KnobGraphic({ size, proportion, enabled }: { size: number; proportion: number; enabled: boolean }) {
  const centre = size / 2;
  const radius = size / 2;
  const arcRadius = radius - 5;
  const angle = DRAWN_START + proportion * (DRAWN_END - DRAWN_START);
  const notch = { width: 2.5, length: radius * 0.3, distance: radius * 0.55 };

  return (
    <>
      <circle cx={centre} cy={centre} r={radius - 2} fill="var(--color-widget)" />
      <path d={arcPath(centre, centre, arcRadius, DRAWN_START, DRAWN_END)} stroke="var(--color-widget-border)" strokeWidth={3} strokeLinecap="round" fill="none" />
      {proportion > 0 && (
        <path d={arcPath(centre, centre, arcRadius, DRAWN_START, angle)} stroke={enabled ? 'var(--color-accent)' : 'var(--color-dim)'} strokeWidth={3} strokeLinecap="round" fill="none" />
      )}
      <rect
        x={centre - notch.width / 2}
        y={centre - notch.distance}
        width={notch.width}
        height={notch.length}
        rx={1}
        fill={enabled ? 'var(--color-fg)' : '#888888'}
        transform={`rotate(${(angle * 180) / Math.PI} ${centre} ${centre})`}
      />
    </>
  );
}

//==============================================================================
// Dragging an LFO or envelope tab onto a control assigns it
export interface ModulationDrag {
  start(source: ModulationSource, index: number, e: ReactPointerEvent): void;
}

export const ModulationDragContext = createContext<ModulationDrag | null>(null);

export const MOD_TARGET = 'data-mod-target';

const badgeText = ({ source, index }: Modulation) => `${source === 'lfo' ? 'L' : 'E'}${index + 1}`;

function Badge({ rect, modulation }: { rect: Rect; modulation: Modulation }) {
  return (
    <div className="flex items-center justify-center rounded-xs bg-widget text-accent" style={{ ...at(rect), fontSize: juceHeight(9) }}>
      {badgeText(modulation)}
    </div>
  );
}

// The badge drag and the Clear Assignment menu, which knobs and steppers share
function useModulationControls(backend: ModulateBackend, id: string, value: number, param: ParamInfo) {
  const modulation = useModulation(backend, id);
  const menu = useMenu();
  const depthDrag = useRef<number | null>(null);
  const base = toProportion(param, value);

  return {
    modulation,
    menu,
    // Moving the value keeps the range on it
    followValue: (next: number) => modulation && backend.setModulation(id, applyDepthChange(modulation, toProportion(param, next), 0)),
    badgeHandlers: {
      onPointerDown: (e: ReactPointerEvent) => {
        if (e.button !== 0 || !modulation) return;
        e.stopPropagation();
        e.currentTarget.setPointerCapture(e.pointerId);
        depthDrag.current = e.clientY;
      },
      // As ModulatableSlider, each move applies the whole distance from where the drag began
      onPointerMove: (e: ReactPointerEvent) => {
        const current = backend.getModulation(id);
        if (depthDrag.current === null || !current) return;
        backend.setModulation(id, applyDepthChange(current, base, -(e.clientY - depthDrag.current) * MOD_DRAG_PER_PIXEL));
      },
      onPointerUp: () => (depthDrag.current = null),
      onPointerCancel: () => (depthDrag.current = null),
    },
    onContextMenu: (e: ReactMouseEvent<HTMLElement>) => {
      e.preventDefault();
      if (!modulation) return;
      menu.open(e.currentTarget, pointIn(e.currentTarget, e.currentTarget.offsetWidth, e.clientX, e.clientY));
    },
    clear: () => backend.setModulation(id, null),
  };
}

//==============================================================================
type DragStart = { x: number; y: number; value: number };

function useValueDrag(backend: ModulateBackend, id: string, valueAt: (e: ReactPointerEvent, start: DragStart, param: ParamInfo) => number, onChange?: (value: number) => void) {
  const drag = useRef<DragStart | null>(null);
  const [dragging, setDragging] = useState(false);
  const state = useParam(backend, id);
  const { value, set, param } = state;

  const end = () => {
    if (!drag.current) return;
    drag.current = null;
    setDragging(false);
    backend.endGesture(id);
  };

  const handlers = {
    onPointerDown: (e: ReactPointerEvent) => {
      if (e.button !== 0) return;
      e.currentTarget.setPointerCapture(e.pointerId);
      drag.current = { x: e.clientX, y: e.clientY, value };
      setDragging(true);
      backend.beginGesture(id);
    },
    onPointerMove: (e: ReactPointerEvent) => {
      if (!drag.current) return;
      const next = valueAt(e, drag.current, param);
      if (next === value) return;
      set(next);
      onChange?.(next);
    },
    onPointerUp: end,
    onPointerCancel: end,
  };

  const aria = {
    'aria-label': param.name,
    'aria-valuemin': param.min,
    'aria-valuemax': param.max,
    'aria-valuenow': value,
    'aria-valuetext': formatValue(param, value),
  };

  return { ...state, dragging, handlers, aria };
}

const rotaryValueAt = (e: ReactPointerEvent, start: DragStart, param: ParamInfo) => {
  const moved = e.clientX - start.x - (e.clientY - start.y);
  const travel = moved / (PIXELS_FOR_FULL_DRAG * (e.shiftKey ? FINE_DRAG : 1));
  return fromProportion(param, toProportion(param, start.value) + travel);
};

const LABEL_STYLE: CSSProperties = { fontSize: juceHeight(15), lineHeight: 1 };

// A Label's border, inside which its text fits
const LABEL_INSET = 5;
const MIN_SQUEEZE = 0.7;

// As drawFittedText: squeezed to fit, as far as 70% wide, then smaller
export function FittedText({ text, width }: { text: string; width: number }) {
  const span = useRef<HTMLSpanElement>(null);
  const [fit, setFit] = useState({ squeeze: 1, shrink: 1 });

  useLayoutEffect(() => {
    const measure = () => {
      const natural = span.current?.scrollWidth ?? 0;
      const scale = natural > width ? width / natural : 1;
      setFit({ squeeze: Math.max(MIN_SQUEEZE, scale), shrink: Math.min(1, scale / MIN_SQUEEZE) });
    };
    measure();
    document.fonts?.ready.then(measure);
  }, [text, width]);

  return (
    <span ref={span} className="inline-block whitespace-nowrap" style={{ transform: `scaleX(${fit.squeeze})`, fontSize: fit.shrink === 1 ? undefined : `${fit.shrink}em` }}>
      {text}
    </span>
  );
}

function Bubble({ text }: { text: string }) {
  return (
    <div
      className="pointer-events-none absolute bottom-full left-1/2 z-30 mb-1 -translate-x-1/2 rounded-xs border border-accent bg-widget px-1.5 py-0.5 whitespace-nowrap text-accent"
      style={{ fontSize: juceHeight(12) }}
    >
      {text}
    </div>
  );
}

interface KnobProps {
  backend: ModulateBackend;
  id: string;
  bounds: Rect;
  // Shows the value while the knob turns. Without one, a bubble does
  label?: { bounds: Rect; text: string; colour?: string };
  modulatable?: boolean;
  enabled?: boolean;
}

export function Knob({ backend, id, bounds, label, modulatable = false, enabled = true }: KnobProps) {
  const controls = useModulationControlsFor(backend, id, modulatable);
  const { value, param, dragging, handlers, aria } = useValueDrag(backend, id, rotaryValueAt, controls?.followValue);
  const modulation = controls?.modulation ?? null;
  const live = useLive(backend, modulation !== null);

  const size = Math.min(bounds.width, bounds.height);
  const centre = size / 2;
  const trackWidth = (size / 2) * 0.15;
  const arcRadius = size / 2 - trackWidth / 2;
  const sweep = SLIDER_END - SLIDER_START;
  const badge = { left: size - 18, top: 2, width: 16, height: 12 };

  const heard = modulation && live ? toProportion(param, modulatedValue(param, value, modulation, sourceValue(live, modulation))) : null;
  const dot = heard === null ? null : pointAt(centre, centre, arcRadius, SLIDER_START + heard * sweep);

  return (
    <>
      {label && (
        <div className="flex items-center justify-center" style={{ ...at(label.bounds), ...LABEL_STYLE, color: label.colour, ...dimmed(enabled) }}>
          <FittedText text={dragging ? formatValue(param, value) : label.text} width={label.bounds.width - 2 * LABEL_INSET} />
        </div>
      )}
      <div
        className="cursor-ns-resize touch-none"
        role="slider"
        {...aria}
        {...{ [MOD_TARGET]: modulatable ? id : undefined }}
        style={{ ...at(bounds), ...dimmed(enabled) }}
        {...handlers}
        onContextMenu={controls?.onContextMenu}
      >
        <svg className="absolute" style={{ left: (bounds.width - size) / 2, top: (bounds.height - size) / 2 }} width={size} height={size} aria-hidden>
          <KnobGraphic size={size} proportion={toProportion(param, value)} enabled={enabled} />
          {modulation && (
            <path
              d={arcPath(centre, centre, arcRadius, SLIDER_START + modulation.min * sweep, SLIDER_START + modulation.max * sweep)}
              stroke="var(--color-accent)"
              strokeOpacity={0.5}
              strokeWidth={trackWidth}
              strokeLinecap="round"
              fill="none"
            />
          )}
          {dot && (
            <>
              <circle cx={dot[0]} cy={dot[1]} r={3} fill="var(--color-accent)" fillOpacity={0.7} />
              <circle cx={dot[0]} cy={dot[1]} r={1.5} fill="var(--color-fg)" fillOpacity={0.8} />
            </>
          )}
        </svg>
        {modulation && controls && (
          <div className="absolute" style={{ left: (bounds.width - size) / 2, top: (bounds.height - size) / 2 }}>
            <div {...controls.badgeHandlers} className="cursor-ns-resize">
              <Badge rect={badge} modulation={modulation} />
            </div>
          </div>
        )}
        {!label && dragging && <Bubble text={formatValue(param, value)} />}
      </div>
      {controls?.menu.anchor && <Menu anchor={controls.menu.anchor} onClose={controls.menu.close} items={[{ label: 'Clear Assignment', onSelect: controls.clear }]} />}
    </>
  );
}

// Hooks can't be called conditionally, so a plain knob gets the controls and ignores them
function useModulationControlsFor(backend: ModulateBackend, id: string, modulatable: boolean) {
  const { value, param } = useParam(backend, id);
  const controls = useModulationControls(backend, id, value, param);
  return modulatable ? controls : null;
}

//==============================================================================
interface StepperProps {
  backend: ModulateBackend;
  id: string;
  bounds: Rect;
  label: string;
  showSign?: boolean;
  enabled?: boolean;
}

export function Stepper({ backend, id, bounds, label, showSign = true, enabled = true }: StepperProps) {
  const { value, param } = useParam(backend, id);
  const controls = useModulationControls(backend, id, value, param);
  const { commit, handlers, aria } = useValueDrag(
    backend,
    id,
    (e, start, param) => snap(param, start.value + Math.trunc((start.y - e.clientY) / STEPPER_PIXELS_PER_STEP)),
    controls.followValue
  );
  const { modulation } = controls;
  const live = useLive(backend, modulation !== null);

  const badge = { left: bounds.width - 16, top: -1, width: 15, height: 9 };
  const track = { left: 4, top: 1, width: Math.max(1, badge.left - 2 - 4), height: 3 };
  const heard = modulation && live ? toProportion(param, modulatedValue(param, value, modulation, sourceValue(live, modulation))) : null;
  const shown = Math.round(value);

  return (
    <>
      <div
        className="flex cursor-ns-resize touch-none rounded-sm border border-widget-border bg-widget font-bold"
        role="spinbutton"
        {...aria}
        {...{ [MOD_TARGET]: id }}
        style={{ ...at(bounds), ...dimmed(enabled), paddingTop: modulation ? 7 : 0 }}
        {...handlers}
        onDoubleClick={() => commit(snap(param, 0))}
        onContextMenu={controls.onContextMenu}
      >
        <span className="flex flex-1 items-center justify-center text-[#808080]" style={{ fontSize: juceHeight(10) }}>{label}</span>
        <span className="flex flex-1 items-center justify-center" style={{ fontSize: juceHeight(12) }}>{`${showSign && shown >= 0 ? '+' : ''}${shown}`}</span>
        {modulation && (
          <>
            <div className="absolute rounded-full bg-black/40" style={at(track)}>
              <div
                className="absolute h-full rounded-full bg-accent/50"
                style={{ left: modulation.min * track.width, width: Math.max(1, (modulation.max - modulation.min) * track.width) }}
              />
              {heard !== null && (
                <div className="absolute top-1/2 size-1.25 -translate-1/2 rounded-full bg-accent/90" style={{ left: heard * track.width }}>
                  <div className="absolute top-1/2 left-1/2 size-0.5 -translate-1/2 rounded-full bg-white/80" />
                </div>
              )}
            </div>
            <div {...controls.badgeHandlers} className="absolute" style={{ left: 0, top: 0 }}>
              <Badge rect={badge} modulation={modulation} />
            </div>
          </>
        )}
      </div>
      {controls.menu.anchor && <Menu anchor={controls.menu.anchor} onClose={controls.menu.close} items={[{ label: 'Clear Assignment', onSelect: controls.clear }]} />}
    </>
  );
}

//==============================================================================
export function ComboBox({ bounds, text, items, enabled = true }: {
  bounds: Rect;
  text: string;
  // A function builds them as the menu opens
  items: MenuItem[] | (() => MenuItem[]);
  enabled?: boolean;
}) {
  const menu = useMenu();

  return (
    <>
      <button
        type="button"
        className="flex cursor-pointer items-center rounded-sm border border-widget-border bg-widget text-left"
        style={{ ...at(bounds), ...dimmed(enabled), fontSize: juceHeight(14) }}
        aria-haspopup="menu"
        onClick={(e) => menu.open(e.currentTarget)}
      >
        <span className="flex-1 truncate pl-1.5">{text}</span>
        <svg className="mx-1.25 flex-none" width="10" height="6" viewBox="0 0 10 6" aria-hidden>
          <path d="M 0 0 L 10 0 L 5 6 Z" fill={enabled ? 'var(--color-accent)' : 'var(--color-dim)'} />
        </svg>
      </button>
      {menu.anchor && <Menu anchor={menu.anchor} onClose={menu.close} items={typeof items === 'function' ? items() : items} />}
    </>
  );
}

// A ComboBox on a choice parameter
export function ChoiceBox({ backend, id, bounds, enabled = true }: { backend: ModulateBackend; id: string; bounds: Rect; enabled?: boolean }) {
  const { value, commit, param } = useParam(backend, id);

  return (
    <ComboBox
      bounds={bounds}
      enabled={enabled}
      text={formatValue(param, value)}
      items={param.choices.map((choice, index) => ({ label: choice, onSelect: () => commit(index) }))}
    />
  );
}

//==============================================================================
export function Toggle({ backend, id, bounds }: { backend: ModulateBackend; id: string; bounds: Rect }) {
  const { value, commit, param } = useParam(backend, id);
  const on = value >= 0.5;
  const size = Math.min(bounds.width, bounds.height) - 4;
  const centre = Math.min(bounds.width, bounds.height) / 2;

  return (
    <button type="button" className="cursor-pointer" style={at(bounds)} aria-label={param.name} aria-pressed={on} onClick={() => commit(on ? 0 : 1)}>
      <svg className="absolute" style={{ left: (bounds.width - centre * 2) / 2, top: (bounds.height - centre * 2) / 2 }} width={centre * 2} height={centre * 2} aria-hidden>
        <circle cx={centre} cy={centre} r={size / 2 - 1} fill="none" stroke={on ? 'var(--color-accent)' : 'var(--color-widget)'} strokeWidth={2} />
        {on && (
          <>
            <circle cx={centre} cy={centre} r={size / 2 - 3} fill="#00aa2e" fillOpacity={0.3} />
            <circle cx={centre} cy={centre} r={size * 0.15 + 2} fill="var(--color-accent)" fillOpacity={0.4} />
            <circle cx={centre} cy={centre} r={size * 0.15} fill="var(--color-accent)" />
          </>
        )}
      </svg>
    </button>
  );
}

export function PillToggle({ backend, id, bounds, text, enabled = true }: { backend: ModulateBackend; id: string; bounds: Rect; text: string; enabled?: boolean }) {
  const { value, commit, param } = useParam(backend, id);
  const on = value >= 0.5;

  return (
    <button
      type="button"
      className="cursor-pointer p-0.5"
      style={{ ...at(bounds), ...dimmed(enabled) }}
      aria-label={param.name}
      aria-pressed={on}
      onClick={() => commit(on ? 0 : 1)}
    >
      <span
        className="flex size-full items-center justify-center rounded-[3px] border-[1.5px]"
        style={{ borderColor: on ? 'var(--color-accent)' : 'var(--color-widget-border)', color: on ? 'var(--color-accent)' : 'var(--color-dim)', fontSize: juceHeight(10) }}
      >
        {text}
      </span>
    </button>
  );
}

//==============================================================================
const TAB_BAR_HEIGHT = 40;
const HANDLE_WIDTH = 18;
const HANDLE_DRAG_DISTANCE = 5;

export function Cross({ size = 8, thickness = 2 }: { size?: number; thickness?: number }) {
  return (
    <svg width={size} height={size} aria-hidden>
      <rect x={(size - thickness) / 2} y={0} width={thickness} height={size} fill="var(--color-accent)" />
      <rect x={0} y={(size - thickness) / 2} width={size} height={thickness} fill="var(--color-accent)" />
    </svg>
  );
}

// A TabbedComponent whose active tab's cross drags its source onto a control
export function SourceTabs({ source, names, width, active, onSelect, children }: {
  source: ModulationSource;
  names: string[];
  width: number;
  active: number;
  onSelect: (index: number) => void;
  children: ReactNode;
}) {
  const drag = useContext(ModulationDragContext);
  const pressed = useRef<{ x: number; y: number } | null>(null);
  const tabWidth = width / names.length;

  return (
    <>
      <div className="absolute inset-x-0 top-0 flex" style={{ height: TAB_BAR_HEIGHT }} role="tablist">
        {names.map((name, index) => {
          const isActive = index === active;
          return (
            <div
              key={name}
              className="relative flex cursor-pointer items-center justify-center font-bold"
              style={{ width: tabWidth, background: isActive ? 'var(--color-widget)' : 'var(--color-panel)', color: isActive ? 'var(--color-accent)' : 'var(--color-dim)', fontSize: juceHeight(14) }}
              role="tab"
              aria-selected={isActive}
              onClick={() => onSelect(index)}
            >
              {name}
              {isActive && <div className="absolute inset-x-0 bottom-0 h-0.75 bg-accent" />}
              {isActive && (
                <div
                  className="absolute inset-y-0 right-0 flex cursor-grab touch-none items-center justify-center"
                  style={{ width: HANDLE_WIDTH }}
                  aria-label={`Drag ${name} onto a control`}
                  onClick={(e) => e.stopPropagation()}
                  onPointerDown={(e) => {
                    e.stopPropagation();
                    // Moves keep coming once the pointer leaves the handle
                    e.currentTarget.setPointerCapture(e.pointerId);
                    pressed.current = { x: e.clientX, y: e.clientY };
                  }}
                  onPointerMove={(e) => {
                    if (!pressed.current || Math.hypot(e.clientX - pressed.current.x, e.clientY - pressed.current.y) <= HANDLE_DRAG_DISTANCE) return;
                    pressed.current = null;
                    drag?.start(source, index, e);
                  }}
                  onPointerUp={() => (pressed.current = null)}
                >
                  <Cross />
                </div>
              )}
            </div>
          );
        })}
      </div>
      <div className="absolute inset-x-0 bottom-0 bg-panel" style={{ top: TAB_BAR_HEIGHT }}>
        {children}
      </div>
    </>
  );
}

export function PanelBox({ bounds, children }: { bounds: Rect; children: ReactNode }) {
  return (
    <section className="rounded-lg bg-panel shadow-[inset_0_0_0_1px_var(--color-widget)]" style={at(bounds)}>
      {children}
    </section>
  );
}

export function Label({ bounds, text, bold = false, colour, align = 'center', size = 15, enabled = true }: {
  bounds: Rect;
  text: string;
  bold?: boolean;
  colour?: string;
  align?: 'left' | 'center';
  size?: number;
  enabled?: boolean;
}) {
  return (
    <div
      className={`flex items-center ${align === 'left' ? 'justify-start pl-1.25 [&>span]:origin-left' : 'justify-center'} ${bold ? 'font-bold' : ''}`}
      style={{ ...at(bounds), fontSize: juceHeight(size), color: colour, opacity: enabled ? 1 : DISABLED_ALPHA }}
    >
      <FittedText text={text} width={bounds.width - 2 * LABEL_INSET} />
    </div>
  );
}
