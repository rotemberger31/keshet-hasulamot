import type { Syllable } from '../../music/syllables';
import type { Voice } from './formant';

/** public/voices/manifest.json: the recorded MIDI notes per voice and syllable. */
export type VoiceManifest = Partial<Record<Voice, Partial<Record<Syllable, number[]>>>>;

export const MAX_STRETCH_SEMITONES = 1.5;

export const sampleUrl = (base: string, voice: Voice, syllable: Syllable, midi: number) =>
  `${base}voices/${voice}/${syllable}_${midi}.mp3`;

/** The closest recording. Notes past the edge of a voice's range bend further than usual. */
export function nearestSample(available: number[], midi: number): number | null {
  if (!available.length) return null;
  return available.reduce((best, m) => (Math.abs(m - midi) < Math.abs(best - midi) ? m : best));
}
