import { mod, type Spelled } from './notes';
import type { Scale } from './scales';
import { spellDegree } from './spelling';

export type Direction = 'up' | 'down' | 'updown';
export type Pattern = 'scale' | 'thirds' | 'arp';
export type Subdivision = 1 | 2 | 3 | 4;
export type Singer = 'female' | 'male' | 'piano' | 'silent';

export interface SeqNote {
  midi: number;
  degreeLabel: string;
  spelled: Spelled;
}

export interface SequenceOptions {
  scale: Scale;
  rootPc: number;
  preferFlats: boolean;
  direction: Direction;
  pattern: Pattern;
  sub: Subdivision;
  singer: Singer;
}

export const BEATS_PER_BAR = 4;

const FEMALE_RANGE = [57, 81] as const; // A3–A5
export const VOICE_RANGE: Record<Singer, readonly [number, number]> = {
  male: [45, 69], // A2–A4
  female: FEMALE_RANGE,
  piano: FEMALE_RANGE,
  silent: FEMALE_RANGE,
};

/** Ascending walk, as scale-step indexes. Index n is the root an octave up. */
function ascending(n: number, pattern: Pattern): number[] {
  const degrees = [...Array(n).keys()];
  const end = [n];
  if (pattern === 'thirds') return [...degrees.flatMap((i) => [i, i + 2]), ...end];
  if (pattern === 'arp') return [...degrees.flatMap((i) => [i, i + 2, i + 4]), ...end];
  return [...degrees, ...end];
}

/** The descent is the ascent reversed, so every pattern stays above the root and ends on it. */
export function walkIndexes(n: number, direction: Direction, pattern: Pattern): number[] {
  const up = ascending(n, pattern);
  const down = [...up].reverse();
  if (direction === 'up') return up;
  return direction === 'down' ? down : [...up, ...down];
}

const semisAt = (scale: Scale, index: number) =>
  scale.degrees[mod(index, scale.degrees.length)].semi +
  12 * Math.floor(index / scale.degrees.length);

/** Picks the octave of the root that overflows the singer's range the least, lowest first. */
export function fitRootMidi(rootPc: number, singer: Singer, lowSemi: number, highSemi: number): number {
  const [low, high] = VOICE_RANGE[singer];
  let best = -1;
  let bestOverflow = Infinity;
  for (let midi = rootPc + 24; midi <= high; midi += 12) {
    const overflow = Math.max(0, low - (midi + lowSemi)) + Math.max(0, midi + highSemi - high);
    if (overflow < bestOverflow) {
      best = midi;
      bestOverflow = overflow;
    }
  }
  return best;
}

/** The notes of one pass. `null` is a rest that pads the last bar of 4/4. */
export function buildSequence(options: SequenceOptions): (SeqNote | null)[] {
  const { scale, rootPc, preferFlats, direction, pattern, sub, singer } = options;
  const indexes = walkIndexes(scale.degrees.length, direction, pattern);
  const semis = indexes.map((index) => semisAt(scale, index));
  const rootMidi = fitRootMidi(rootPc, singer, Math.min(...semis), Math.max(...semis));

  const notes: (SeqNote | null)[] = indexes.map((index, i) => {
    const degree = scale.degrees[mod(index, scale.degrees.length)];
    return {
      midi: rootMidi + semis[i],
      degreeLabel: degree.label,
      spelled: spellDegree(rootPc, preferFlats, degree),
    };
  });

  const bar = BEATS_PER_BAR * sub;
  while (notes.length % bar) notes.push(null);
  return notes;
}
