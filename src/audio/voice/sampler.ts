import type { Syllable } from '../../music/syllables';
import { SYLLABLES } from '../../music/syllables';
import type { Voice } from './formant';
import { nearestSample, sampleUrl, type VoiceManifest } from './manifest';

type Bank = Map<Syllable, Map<number, AudioBuffer>>;

const banks = new Map<Voice, Bank>();
let manifest: Promise<VoiceManifest | null> | null = null;
const loading = new Set<Voice>();
const BASE = import.meta.env.BASE_URL;

function loadManifest() {
  manifest ??= fetch(`${BASE}voices/manifest.json`)
    .then((res) => (res.ok ? (res.json() as Promise<VoiceManifest>) : null))
    .catch(() => null); // no recordings yet, or a dev server answering with HTML
  return manifest;
}

/** Decodes a voice in the background. Until a bank is complete the formant voice sings. */
export async function loadVoice(ctx: BaseAudioContext, voice: Voice) {
  if (banks.has(voice) || loading.has(voice)) return;
  loading.add(voice);
  try {
    const list = (await loadManifest())?.[voice];
    if (!list || !SYLLABLES.every((syllable) => list[syllable]?.length)) return;

    const bank: Bank = new Map();
    await Promise.all(
      SYLLABLES.map(async (syllable) => {
        const notes = new Map<number, AudioBuffer>();
        bank.set(syllable, notes);
        await Promise.all(
          list[syllable]!.map(async (midi) => {
            const res = await fetch(sampleUrl(BASE, voice, syllable, midi));
            notes.set(midi, await ctx.decodeAudioData(await res.arrayBuffer()));
          }),
        );
      }),
    );
    banks.set(voice, bank);
  } catch (error) {
    console.warn('voice samples unavailable, using synthesis', error);
  } finally {
    loading.delete(voice);
  }
}

export const hasSamples = (voice: Voice) => banks.has(voice);

export function singSample(ctx: BaseAudioContext, bus: AudioNode, time: number, midi: number, duration: number, syllable: Syllable, voice: Voice) {
  const notes = banks.get(voice)?.get(syllable);
  const nearest = notes && nearestSample([...notes.keys()], midi);
  if (nearest == null) return;
  const source = ctx.createBufferSource();
  source.buffer = notes!.get(nearest)!;
  source.playbackRate.value = 2 ** ((midi - nearest) / 12);
  const gain = ctx.createGain();
  const fade = Math.min(0.03, duration * 0.3);
  gain.gain.setValueAtTime(1, time);
  gain.gain.setValueAtTime(1, time + duration - fade);
  gain.gain.linearRampToValueAtTime(0, time + duration);
  source.connect(gain).connect(bus);
  source.start(time);
  source.stop(time + duration + 0.01);
}
