import { describe, expect, it, vi } from 'vitest';
import { createScheduler, type Step } from './scheduler';

function rig(onStep: (step: Step) => void = () => {}) {
  let time = 0;
  let tick: (() => void) | null = null;
  const steps: Step[] = [];
  const onEnd = vi.fn();
  const scheduler = createScheduler({
    now: () => time,
    setTimer: (fn) => ((tick = fn), 1),
    clearTimer: () => (tick = null),
    onStep: (step) => {
      steps.push(step);
      onStep(step);
    },
    onEnd,
  });
  const run = (seconds: number) => {
    for (let t = 0; t < seconds; t += 0.025) {
      time += 0.025;
      tick?.();
    }
  };
  return { scheduler, steps, onEnd, run };
}

const pass = { length: 8, sub: 1, bpm: 60, loop: false, countIn: false };

describe('scheduler', () => {
  it('plays every step exactly on the grid', () => {
    const { scheduler, steps, onEnd, run } = rig();
    scheduler.start(pass);
    run(10);
    expect(steps.map((s) => s.index)).toEqual([0, 1, 2, 3, 4, 5, 6, 7]);
    steps.forEach((step, i) => expect(step.time).toBeCloseTo(0.08 + i));
    expect(onEnd).toHaveBeenCalledOnce();
    expect(scheduler.running).toBe(false);
  });

  it('keeps advancing when a step throws', () => {
    const error = vi.spyOn(console, 'error').mockImplementation(() => {});
    const { scheduler, steps, run } = rig((step) => {
      if (step.index === 3) throw new Error('bad consonant');
    });
    scheduler.start(pass);
    run(10);
    expect(steps).toHaveLength(8);
    expect(error).toHaveBeenCalledOnce();
    error.mockRestore();
  });

  it('counts in one bar, then starts on the beat', () => {
    const { scheduler, steps, run } = rig();
    scheduler.start({ ...pass, countIn: true });
    run(14);
    expect(steps.slice(0, 5).map((s) => s.kind)).toEqual(['count', 'count', 'count', 'count', 'note']);
    expect(steps[4].time).toBeCloseTo(4.08);
  });

  it('loops without a gap', () => {
    const { scheduler, steps, run } = rig();
    scheduler.start({ ...pass, length: 4, loop: true });
    run(6);
    expect(steps[4].index).toBe(0);
    expect(steps[4].time - steps[3].time).toBeCloseTo(1);
    scheduler.stop();
  });

  it('marks beats and leaves subdivisions between them', () => {
    const { scheduler, steps, run } = rig();
    scheduler.start({ ...pass, sub: 2 });
    run(6);
    expect(steps.slice(0, 4).map((s) => s.beat)).toEqual([0, -1, 1, -1]);
  });

  it('swaps a shorter sequence live without restarting', () => {
    const { scheduler, steps, run } = rig();
    scheduler.start({ ...pass, loop: true });
    run(5);
    scheduler.update({ length: 4 });
    const before = steps.length;
    run(3);
    expect(steps.slice(before).every((s) => s.index < 4)).toBe(true);
    expect(steps[before].time - steps[before - 1].time).toBeCloseTo(1);
    scheduler.stop();
  });

  it('moves to a new subdivision on the next beat', () => {
    const { scheduler, steps, run } = rig();
    scheduler.start({ ...pass, sub: 2, length: 16, loop: true });
    run(1.6); // somewhere inside the sequence, maybe between beats
    scheduler.update({ sub: 4 });
    run(3);
    const beats = steps.filter((s) => s.beat >= 0);
    beats.slice(1).forEach((step, i) => expect(step.time - beats[i].time).toBeCloseTo(1));
    scheduler.stop();
  });

  it('follows a new tempo', () => {
    const { scheduler, steps, run } = rig();
    scheduler.start({ ...pass, length: 32, loop: true });
    run(2);
    scheduler.update({ bpm: 120 });
    const before = steps.length;
    run(2);
    expect(steps[before + 1].time - steps[before].time).toBeCloseTo(0.5);
    scheduler.stop();
  });

  it('stops booking steps after stop', () => {
    const { scheduler, steps, run } = rig();
    scheduler.start({ ...pass, loop: true });
    run(2);
    scheduler.stop();
    const before = steps.length;
    run(3);
    expect(steps).toHaveLength(before);
  });
});
