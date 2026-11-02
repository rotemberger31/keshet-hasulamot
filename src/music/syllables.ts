import type { Spelled } from './notes';

export type Syllable = 'do' | 're' | 'mi' | 'fa' | 'sol' | 'la' | 'si' | 'di' | 'ez' | 'be' | 'mol';

export const SYLLABLES: Syllable[] = ['do', 're', 'mi', 'fa', 'sol', 'la', 'si', 'di', 'ez', 'be', 'mol'];

const LETTER_SYLLABLE: Syllable[] = ['do', 're', 'mi', 'fa', 'sol', 'la', 'si'];
const DIEZ: Syllable[] = ['di', 'ez'];
const BEMOL: Syllable[] = ['be', 'mol'];

export interface TimedSyllable {
  syllable: Syllable;
  start: number; // fraction of the note
  length: number; // fraction of the note
}

const BASE_SHARE = 0.4;

/** The full sung name, split across the note: "רה במול" is re + be + mol. */
export function noteSyllables(spelled: Spelled): TimedSyllable[] {
  const base = LETTER_SYLLABLE[spelled.letter];
  if (!spelled.acc) return [{ syllable: base, start: 0, length: 1 }];

  const word = spelled.acc > 0 ? DIEZ : BEMOL;
  const accidental = Array.from({ length: Math.abs(spelled.acc) }, () => word).flat();
  const share = (1 - BASE_SHARE) / accidental.length;
  return [
    { syllable: base, start: 0, length: BASE_SHARE },
    ...accidental.map((syllable, i) => ({ syllable, start: BASE_SHARE + share * i, length: share })),
  ];
}
