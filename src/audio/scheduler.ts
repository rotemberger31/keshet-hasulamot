export const TICK_MS = 25;
export const LOOKAHEAD_S = 0.12;
const MAX_LAG_S = 0.2;
const COUNT_IN_BEATS = 4;

export interface Step {
  kind: 'count' | 'note';
  time: number;
  duration: number;
  index: number; // position in the sequence, or the count-in beat
  beat: number; // 0..3 on a beat, -1 between beats
}

export interface SchedulerDeps {
  now: () => number;
  setTimer: (fn: () => void, ms: number) => unknown;
  clearTimer: (id: unknown) => void;
  onStep: (step: Step) => void;
  onEnd: (time: number) => void;
}

export interface Pass {
  length: number;
  sub: number;
  bpm: number;
  loop: boolean;
  countIn: boolean;
}

/**
 * Lookahead clock: a coarse timer that books steps slightly ahead on the audio
 * clock. A step that throws is logged and skipped, and time always moves on.
 */
export function createScheduler(deps: SchedulerDeps) {
  let timer: unknown = null;
  let pass: Pass = { length: 0, sub: 1, bpm: 70, loop: true, countIn: false };
  let nextTime = 0;
  let position = 0;
  let countBeat = COUNT_IN_BEATS;

  const beatLength = () => 60 / pass.bpm;
  const stepLength = () => beatLength() / pass.sub;

  function emit(step: Step) {
    try {
      deps.onStep(step);
    } catch (error) {
      console.error('step failed', error);
    }
  }

  function stop() {
    if (timer !== null) deps.clearTimer(timer);
    timer = null;
  }

  function tick() {
    const now = deps.now();
    if (nextTime < now - MAX_LAG_S) nextTime = now + 0.05; // tab was asleep: don't burst

    while (nextTime < now + LOOKAHEAD_S) {
      if (countBeat < COUNT_IN_BEATS) {
        emit({ kind: 'count', time: nextTime, duration: beatLength(), index: countBeat, beat: countBeat });
        nextTime += beatLength();
        countBeat++;
        continue;
      }
      if (position >= pass.length) {
        if (!pass.loop || pass.length === 0) {
          stop();
          deps.onEnd(nextTime);
          return;
        }
        position = 0;
      }
      const onBeat = position % pass.sub === 0;
      const beat = onBeat ? Math.floor(position / pass.sub) % 4 : -1;
      emit({ kind: 'note', time: nextTime, duration: stepLength(), index: position, beat });
      nextTime += stepLength();
      position++;
    }
  }

  return {
    start(next: Pass) {
      stop();
      pass = next;
      position = 0;
      countBeat = next.countIn ? 0 : COUNT_IN_BEATS;
      nextTime = deps.now() + 0.08;
      timer = deps.setTimer(tick, TICK_MS);
      tick();
    },
    stop,

    /** Live changes keep the place in the bar. A new subdivision takes over on the next beat. */
    update(change: Partial<Pass>) {
      const sub = change.sub ?? pass.sub;
      if (sub !== pass.sub && countBeat >= COUNT_IN_BEATS) {
        const toBeat = (pass.sub - (position % pass.sub)) % pass.sub;
        nextTime += toBeat * stepLength();
        position = ((position + toBeat) / pass.sub) * sub;
      }
      pass = { ...pass, ...change };
      if (pass.length > 0 && position >= pass.length) position %= pass.length;
    },

    get running() {
      return timer !== null;
    },
  };
}
