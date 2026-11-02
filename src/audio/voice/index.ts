import type { Spelled } from '../../music/notes';
import { noteSyllables } from '../../music/syllables';
import { singFormant, type Voice } from './formant';
import { hasSamples, singSample } from './sampler';

const MAX_NOTE_S = 1.4;

/** Sings the full note name on one pitch, from recordings when a voice has them. */
export function singNote(ctx: BaseAudioContext, bus: AudioNode, time: number, midi: number, duration: number, spelled: Spelled, voice: Voice) {
  const total = Math.min(duration, MAX_NOTE_S);
  const parts = noteSyllables(spelled);
  parts.forEach(({ syllable, start, length }, i) => {
    const at = time + start * total;
    const span = length * total;
    if (hasSamples(voice)) singSample(ctx, bus, at, midi, span, syllable, voice);
    else singFormant(ctx, bus, at, midi, span, syllable, voice, i === parts.length - 1);
  });
}

export { loadVoice } from './sampler';
export type { Voice } from './formant';
