import type { Spelled } from '../music/notes';
import type { SeqNote } from '../music/sequence';

export type VisualEvent =
  | { type: 'note'; index: number; note: SeqNote | null; beat: number }
  | { type: 'count'; beat: number }
  | { type: 'stop' }
  | { type: 'tap'; midi: number; spelled: Spelled; degreeLabel: string | null }
  | { type: 'hit'; index: number }
  | { type: 'mic'; midi: number | null };

type Listener = (event: VisualEvent) => void;
type Timed = { time: number; event: VisualEvent };

const listeners = new Set<Listener>();
let queue: Timed[] = [];
let looping = false;

export function subscribeVisual(listener: Listener) {
  listeners.add(listener);
  return () => void listeners.delete(listener);
}

export const emitVisual = (event: VisualEvent) => listeners.forEach((listener) => listener(event));

/** Shows an event when its sound reaches the speakers, not when it was booked. */
export function enqueueVisual(time: number, event: VisualEvent) {
  queue.push({ time, event });
}

export const clearVisualQueue = () => {
  queue = [];
};

export function startVisualLoop(getNow: () => number) {
  if (looping) return;
  looping = true;
  const frame = () => {
    const now = getNow();
    while (queue.length && queue[0].time <= now) emitVisual(queue.shift()!.event);
    requestAnimationFrame(frame);
  };
  requestAnimationFrame(frame);
}
