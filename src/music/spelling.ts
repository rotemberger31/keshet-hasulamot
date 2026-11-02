import { LETTER_PC, mod, type Spelled } from './notes';

export interface Degree {
  n: number; // 1..7
  acc: number;
  semi: number; // semitones above the root
  label: string; // "♭3"
}

export function parseFormula(formula: string): Degree[] {
  return formula.split(' ').map((token) => {
    const match = token.match(/^([b#]*)([1-7])$/);
    if (!match) throw new Error(`Bad degree: ${token}`);
    const acc = [...match[1]].reduce((sum, c) => sum + (c === '#' ? 1 : -1), 0);
    const n = Number(match[2]);
    return {
      n,
      acc,
      semi: LETTER_PC[n - 1] + acc,
      label: token.replace(/b/g, '♭').replace(/#/g, '♯'),
    };
  });
}

/** Black-key roots default to flats, except F♯. `preferFlats` false swaps all of them. */
export function spellRoot(pc: number, preferFlats: boolean): Spelled {
  const natural = (LETTER_PC as readonly number[]).indexOf(pc);
  if (natural >= 0) return { letter: natural, acc: 0, pc };
  const useFlat = pc === 6 ? !preferFlats : preferFlats;
  const letterPc = mod(pc + (useFlat ? 1 : -1), 12);
  return { letter: (LETTER_PC as readonly number[]).indexOf(letterPc), acc: useFlat ? -1 : 1, pc };
}

/** Degree n takes the letter root+n−1; the accidental is whatever closes the gap. */
export function spellDegree(rootPc: number, preferFlats: boolean, degree: Degree): Spelled {
  const letter = (spellRoot(rootPc, preferFlats).letter + degree.n - 1) % 7;
  const pc = mod(rootPc + degree.semi, 12);
  return { letter, acc: mod(pc - LETTER_PC[letter] + 6, 12) - 6, pc };
}
