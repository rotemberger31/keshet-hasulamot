export const LETTERS = ['C', 'D', 'E', 'F', 'G', 'A', 'B'] as const;
export const HEBREW = ['דו', 'רה', 'מי', 'פה', 'סול', 'לה', 'סי'] as const;
/** Pitch class of each natural letter. Also the major scale. */
export const LETTER_PC = [0, 2, 4, 5, 7, 9, 11] as const;
const ACC_SYMBOL: Record<number, string> = { [-2]: '𝄫', [-1]: '♭', 0: '', 1: '♯', 2: '𝄪' };

/** A note as written: a letter plus an accidental. */
export interface Spelled {
  letter: number; // index into LETTERS
  acc: number; // -2..2
  pc: number; // 0..11
}

export const mod = (n: number, m: number) => ((n % m) + m) % m;
export const accSymbol = (acc: number) => ACC_SYMBOL[acc] ?? '';
export const heName = (s: Spelled) => HEBREW[s.letter] + accSymbol(s.acc);
export const enName = (s: Spelled) => LETTERS[s.letter] + accSymbol(s.acc);
export const midiToFreq = (midi: number) => 440 * 2 ** ((midi - 69) / 12);

/** Chromatic wheel: C is red, then 30° for each semitone. */
export const noteHue = (pc: number) => (25 + pc * 30) % 360;
export const noteColor = (pc: number) => `oklch(var(--note-l) var(--note-c) ${noteHue(pc)})`;
