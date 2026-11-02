import { mod } from './notes';
import { SCALE_DATA, type ScaleData } from './scales.data';
import { parseFormula, type Degree } from './spelling';

export interface Scale extends ScaleData {
  degrees: Degree[];
  pcs: number[]; // semitones above the root, 0..11
}

export const SCALES: Scale[] = SCALE_DATA.map((data) => {
  const degrees = parseFormula(data.formula);
  return { ...data, degrees, pcs: degrees.map((d) => mod(d.semi, 12)) };
});

const BY_ID = new Map(SCALES.map((s) => [s.id, s]));
export const getScale = (id: string) => BY_ID.get(id) ?? SCALES[0];
export const hasScale = (id: string) => BY_ID.has(id);

/** Modes on the white keys, in letter order: C=Ionian, D=Dorian… */
export const WHITE_KEY_MODES = ['ionian', 'dorian', 'phrygian', 'lydian', 'mixolydian', 'aeolian', 'locrian'];
