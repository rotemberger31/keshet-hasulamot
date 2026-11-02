import { describe, expect, it } from 'vitest';
import { detectPitch, toPitch } from './pitch';
import { nearestSample } from './voice/manifest';

function sine(freq: number, amp = 0.3, rate = 44100) {
  const buffer = new Float32Array(2048);
  for (let i = 0; i < buffer.length; i++) buffer[i] = amp * Math.sin((2 * Math.PI * freq * i) / rate);
  return buffer;
}

describe('pitch', () => {
  it('names a note and its cents', () => {
    expect(toPitch(440)).toMatchObject({ midi: 69, pc: 9 });
    expect(toPitch(440).cents).toBeCloseTo(0);
    expect(toPitch(261.63).pc).toBe(0);
    expect(toPitch(452).cents).toBeCloseTo(46.6, 0);
  });

  it('hears a clean tone', () => {
    const heard = detectPitch(sine(440), 44100);
    expect(heard?.midi).toBe(69);
    expect(Math.abs(heard!.cents)).toBeLessThan(5);
  });

  it('ignores silence', () => {
    expect(detectPitch(sine(440, 0.001), 44100)).toBeNull();
  });
});

describe('sampler choice', () => {
  it('takes the closest recording', () => {
    expect(nearestSample([57, 60, 63], 61)).toBe(60);
    expect(nearestSample([57, 60, 63], 62)).toBe(63);
    expect(nearestSample([], 62)).toBeNull();
  });
});
