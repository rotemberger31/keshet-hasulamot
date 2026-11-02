import { describe, expect, it } from 'vitest';
import { enName, heName, LETTERS } from './notes';
import { relatedScales, scaleSteps } from './related';
import { getScale, SCALES } from './scales';
import { buildSequence, walkIndexes, VOICE_RANGE, type Direction, type Pattern, type Singer, type Subdivision } from './sequence';
import { spellDegree, spellRoot } from './spelling';
import { noteSyllables, SYLLABLES } from './syllables';

const C = 0, Db = 1, D = 2, Eb = 3, E = 4, Gb = 6, G = 7, Ab = 8, A = 9, Bb = 10, B = 11;

const spell = (root: number, id: string, flats = true) =>
  getScale(id).degrees.map((d) => enName(spellDegree(root, flats, d))).join(' ');

const CASES: [number, string, boolean, string][] = [
  [C, 'ionian', true, 'C D E F G A B'],
  [E, 'hijaz', true, 'E F G♯ A B C D'],
  [Eb, 'harmMinor', true, 'E♭ F G♭ A♭ B♭ C♭ D'],
  [D, 'dorian', true, 'D E F G A B C'],
  [Gb, 'ionian', true, 'F♯ G♯ A♯ B C♯ D♯ E♯'],
  [Gb, 'ionian', false, 'G♭ A♭ B♭ C♭ D♭ E♭ F'],
  [Db, 'lydian', false, 'C♯ D♯ E♯ F𝄪 G♯ A♯ B♯'],
  [Db, 'phrygian', true, 'D♭ E𝄫 F♭ G♭ A♭ B𝄫 C♭'],
  [B, 'locrian', true, 'B C D E F G A'],
  [A, 'hungMinor', true, 'A B C D♯ E F G♯'],
  // 5 notes
  [A, 'minPenta', true, 'A C D E G'],
  [C, 'majPenta', true, 'C D E G A'],
  [E, 'insen', true, 'E F A B D'],
  [D, 'hirajoshi', true, 'D E F A B♭'],
  // 6 notes
  [C, 'wholeTone', true, 'C D E F♯ G♯ B♭'],
  [A, 'blues', true, 'A C D E♭ E G'],
  // 8 notes
  [C, 'dimHW', true, 'C D♭ D♯ E F♯ G A B♭'],
  [C, 'dimWH', true, 'C D E♭ F G♭ A♭ A B'],
  [G, 'bebop', true, 'G A B C D E F F♯'],
];

describe('spelling', () => {
  it.each(CASES)('root %i %s (flats %s)', (root, id, flats, expected) => {
    expect(spell(root, id, flats)).toBe(expected);
  });

  it('gives black-key roots flats by default, except F♯', () => {
    expect([Db, Eb, Gb, Ab, Bb].map((pc) => enName(spellRoot(pc, true)))).toEqual(['D♭', 'E♭', 'F♯', 'A♭', 'B♭']);
    expect([Db, Eb, Gb, Ab, Bb].map((pc) => enName(spellRoot(pc, false)))).toEqual(['C♯', 'D♯', 'G♭', 'G♯', 'A♯']);
  });

  it('uses Hebrew names', () => {
    expect(heName(spellDegree(E, true, getScale('hijaz').degrees[2]))).toBe('סול♯');
  });
});

describe('scale data', () => {
  it('has 27 unique scales in five categories', () => {
    expect(SCALES).toHaveLength(27);
    expect(new Set(SCALES.map((s) => s.id)).size).toBe(27);
    expect(new Set(SCALES.map((s) => s.cat)).size).toBe(5);
  });

  it('steps always add up to an octave', () => {
    for (const scale of SCALES) {
      const steps = scaleSteps(scale);
      expect(steps.reduce((a, b) => a + b, 0), scale.id).toBe(12);
      expect(Math.min(...steps), scale.id).toBeGreaterThan(0);
    }
  });

  it('finds one-note neighbours', () => {
    const related = relatedScales(getScale('ionian'));
    expect(related.find((r) => r.scale.id === 'lydian')).toMatchObject({ from: '4', to: '♯4' });
    expect(related.find((r) => r.scale.id === 'mixolydian')).toMatchObject({ from: '7', to: '♭7' });
  });
});

describe('walks', () => {
  it('walks up, down and both ways', () => {
    expect(walkIndexes(7, 'up', 'scale')).toEqual([0, 1, 2, 3, 4, 5, 6, 7]);
    expect(walkIndexes(7, 'down', 'scale')).toEqual([7, 6, 5, 4, 3, 2, 1, 0]);
    expect(walkIndexes(7, 'updown', 'scale')).toHaveLength(16);
  });

  it('builds real triads on each degree of 7-note scales', () => {
    const walk = walkIndexes(7, 'up', 'arp');
    expect(walk.slice(0, 6)).toEqual([0, 2, 4, 1, 3, 5]);
    expect(walk.at(-1)).toBe(7);
  });

  it('D to D is eight rising notes', () => {
    const notes = buildSequence({ scale: getScale('dorian'), rootPc: D, preferFlats: true, direction: 'up', pattern: 'scale', sub: 1, singer: 'female' });
    expect(notes.map((n) => n && enName(n.spelled)).join(' ')).toBe('D E F G A B C D');
    expect(notes[7]!.midi - notes[0]!.midi).toBe(12);
  });

  it('pads an incomplete bar with rests', () => {
    const notes = buildSequence({ scale: getScale('majPenta'), rootPc: C, preferFlats: true, direction: 'up', pattern: 'scale', sub: 1, singer: 'female' });
    expect(notes).toHaveLength(8);
    expect(notes.slice(6)).toEqual([null, null]);
  });
});

describe('every scale × root × spelling × direction × pattern × subdivision × singer', () => {
  const singers: Singer[] = ['female', 'male', 'piano', 'silent'];
  const directions: Direction[] = ['up', 'down', 'updown'];
  const patterns: Pattern[] = ['scale', 'thirds', 'arp'];
  const subs: Subdivision[] = [1, 2, 3, 4];
  const syllables = new Set(SYLLABLES);

  it('is well-formed', () => {
    let combos = 0;
    for (const scale of SCALES)
      for (let rootPc = 0; rootPc < 12; rootPc++)
        for (const preferFlats of [true, false])
          for (const direction of directions)
            for (const pattern of patterns)
              for (const sub of subs)
                for (const singer of singers) {
                  const seq = buildSequence({ scale, rootPc, preferFlats, direction, pattern, sub, singer });
                  const where = `${rootPc} ${scale.id} ${direction} ${pattern} ${sub} ${singer}`;
                  expect(seq.length % (4 * sub), where).toBe(0);
                  const [low, high] = VOICE_RANGE[singer];
                  for (const note of seq) {
                    if (!note) continue;
                    const { acc, pc, letter } = note.spelled;
                    if (Math.abs(acc) > 2) throw new Error(`triple accidental: ${where}`);
                    if (note.midi % 12 !== pc) throw new Error(`pitch mismatch: ${where}`);
                    if (LETTERS[letter] === undefined) throw new Error(`letter: ${where}`);
                    if (note.midi < low - 3 || note.midi > high + 3) throw new Error(`range ${note.midi}: ${where}`);
                    for (const part of noteSyllables(note.spelled))
                      if (!syllables.has(part.syllable)) throw new Error(`syllable: ${where}`);
                  }
                  combos++;
                }
    expect(combos).toBe(27 * 12 * 2 * 3 * 3 * 4 * 4);
  });

  it('stays strictly inside the range for plain scales', () => {
    for (const scale of SCALES)
      for (let rootPc = 0; rootPc < 12; rootPc++)
        for (const singer of singers) {
          const seq = buildSequence({ scale, rootPc, preferFlats: true, direction: 'updown', pattern: 'scale', sub: 1, singer });
          const [low, high] = VOICE_RANGE[singer];
          for (const note of seq) if (note) expect(note.midi).toBeGreaterThanOrEqual(low), expect(note.midi).toBeLessThanOrEqual(high);
        }
  });
});

describe('sung names', () => {
  const names = (letter: number, acc: number) => noteSyllables({ letter, acc, pc: 0 }).map((s) => s.syllable).join(' ');

  it('says the full name', () => {
    expect(names(0, 0)).toBe('do');
    expect(names(0, 1)).toBe('do di ez');
    expect(names(1, -1)).toBe('re be mol');
    expect(names(1, -2)).toBe('re be mol be mol');
    expect(names(3, 2)).toBe('fa di ez di ez');
  });

  it('fills the note exactly, base syllable first at 40%', () => {
    const parts = noteSyllables({ letter: 0, acc: 1, pc: 1 });
    expect(parts[0].length).toBeCloseTo(0.4);
    const end = parts.at(-1)!;
    expect(end.start + end.length).toBeCloseTo(1);
  });
});
