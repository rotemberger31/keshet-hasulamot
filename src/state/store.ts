import { create } from 'zustand';
import type { CategoryId } from '../music/scales.data';
import { getScale, hasScale, SCALES, WHITE_KEY_MODES } from '../music/scales';
import { buildSequence, type Direction, type Pattern, type SeqNote, type Singer, type Subdivision } from '../music/sequence';
import { LETTER_PC } from '../music/notes';

export type Instrument = 'piano' | 'guitar';
export type Theme = 'auto' | 'light' | 'dark';

export interface Settings {
  rootPc: number;
  preferFlats: boolean;
  scaleId: string;
  category: CategoryId;
  direction: Direction;
  pattern: Pattern;
  sub: Subdivision;
  singer: Singer;
  pianoUnder: boolean;
  bpm: number;
  countIn: boolean;
  loop: boolean;
  instrument: Instrument;
  clickVolume: number;
  noteVolume: number;
  theme: Theme;
}

const DEFAULTS: Settings = {
  rootPc: 0,
  preferFlats: true,
  scaleId: 'ionian',
  category: 'modes',
  direction: 'up',
  pattern: 'scale',
  sub: 1,
  singer: 'female',
  pianoUnder: false,
  bpm: 70,
  countIn: true,
  loop: true,
  instrument: 'piano',
  clickVolume: 0.7,
  noteVolume: 0.8,
  theme: 'auto',
};

const STORAGE_KEY = 'keshet-settings-v1';

function loadSettings(): Settings {
  try {
    const raw = localStorage.getItem(STORAGE_KEY);
    const saved = raw ? { ...DEFAULTS, ...JSON.parse(raw) } : DEFAULTS;
    return hasScale(saved.scaleId) ? saved : { ...saved, scaleId: DEFAULTS.scaleId };
  } catch {
    return DEFAULTS;
  }
}

interface AppState extends Settings {
  playing: boolean;
  set: (change: Partial<Settings>) => void;
  setPlaying: (playing: boolean) => void;
  chooseScale: (id: string) => void;
  setBpm: (bpm: number) => void;
  surprise: () => void;
  courseDrill: (letter: number) => void;
}

export const useStore = create<AppState>((set, get) => ({
  ...loadSettings(),
  playing: false,
  set: (change) => set(change),
  setPlaying: (playing) => set({ playing }),
  chooseScale: (id) => set({ scaleId: id, category: getScale(id).cat }),
  setBpm: (bpm) => set({ bpm: Math.max(30, Math.min(200, Math.round(bpm))) }),
  surprise: () => {
    let scale = SCALES[Math.floor(Math.random() * SCALES.length)];
    while (scale.id === get().scaleId) scale = SCALES[Math.floor(Math.random() * SCALES.length)];
    set({ scaleId: scale.id, category: scale.cat, rootPc: Math.floor(Math.random() * 12) });
  },
  courseDrill: (letter) =>
    set({ rootPc: LETTER_PC[letter], scaleId: WHITE_KEY_MODES[letter], category: 'modes', pattern: 'scale', sub: 1, bpm: 70 }),
}));

useStore.subscribe((state) => {
  try {
    const { playing: _playing, set: _set, setPlaying: _sp, chooseScale: _cs, setBpm: _sb, surprise: _s, courseDrill: _c, ...settings } = state;
    localStorage.setItem(STORAGE_KEY, JSON.stringify(settings));
  } catch {
    // private mode: settings just won't survive a reload
  }
});

let lastKey = '';
let lastSequence: (SeqNote | null)[] = [];

/** Memoized, so a new array always means the music really changed. */
export function selectSequence(state: Settings) {
  const { scaleId, rootPc, preferFlats, direction, pattern, sub, singer } = state;
  const key = [scaleId, rootPc, preferFlats, direction, pattern, sub, singer].join('|');
  if (key !== lastKey) {
    lastKey = key;
    lastSequence = buildSequence({ scale: getScale(scaleId), rootPc, preferFlats, direction, pattern, sub, singer });
  }
  return lastSequence;
}
