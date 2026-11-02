import { midiToFreq } from '../../music/notes';
import type { Syllable } from '../../music/syllables';

export type Voice = 'female' | 'male';
type Formants = [number, number, number, number];
type Vowel = 'a' | 'e' | 'i' | 'o';

const FORMANTS: Record<Voice, Record<string, Formants>> = {
  male: {
    a: [730, 1090, 2440, 3400], e: [440, 1900, 2550, 3350], i: [290, 2250, 2950, 3500], o: [470, 820, 2600, 3300],
    l: [350, 1150, 2600, 3300], m: [260, 1000, 2250, 3300], r: [420, 1400, 2450, 3300], d: [300, 1700, 2600, 3300],
    b: [280, 900, 2300, 3300],
  },
  female: {
    a: [860, 1230, 2820, 3900], e: [480, 2280, 2900, 3950], i: [330, 2700, 3250, 4100], o: [530, 930, 2800, 3800],
    l: [400, 1300, 2900, 3800], m: [300, 1150, 2550, 3800], r: [480, 1600, 2800, 3800], d: [340, 1950, 2900, 3800],
    b: [320, 1050, 2600, 3800],
  },
};
const FORMANT_Q = [7, 11, 15, 17];
const FORMANT_GAIN: Record<Vowel, number[]> = {
  a: [1, 0.6, 0.35, 0.15], e: [1, 0.75, 0.45, 0.2], i: [1, 0.8, 0.5, 0.22], o: [1, 0.55, 0.25, 0.1],
};

const SHAPES: Record<Syllable, { onset: string; vowel: Vowel; coda?: 'l' | 'z' }> = {
  do: { onset: 'd', vowel: 'o' },
  re: { onset: 'r', vowel: 'e' },
  mi: { onset: 'm', vowel: 'i' },
  fa: { onset: 'f', vowel: 'a' },
  sol: { onset: 's', vowel: 'o', coda: 'l' },
  la: { onset: 'l', vowel: 'a' },
  si: { onset: 's', vowel: 'i' },
  di: { onset: 'd', vowel: 'i' },
  ez: { onset: '', vowel: 'e', coda: 'z' },
  be: { onset: 'b', vowel: 'e' },
  mol: { onset: 'm', vowel: 'o', coda: 'l' },
};

interface Sources {
  glottal: PeriodicWave;
  noise: AudioBuffer;
}
const sources = new WeakMap<BaseAudioContext, Sources>();

function getSources(ctx: BaseAudioContext): Sources {
  const cached = sources.get(ctx);
  if (cached) return cached;
  const size = 60;
  const imag = new Float32Array(size);
  for (let k = 1; k < size; k++) imag[k] = 1 / k ** 1.2;
  const noise = ctx.createBuffer(1, ctx.sampleRate, ctx.sampleRate);
  const data = noise.getChannelData(0);
  for (let i = 0; i < data.length; i++) data[i] = Math.random() * 2 - 1;
  const made = { glottal: ctx.createPeriodicWave(new Float32Array(size), imag), noise };
  sources.set(ctx, made);
  return made;
}

function noiseBurst(ctx: BaseAudioContext, bus: AudioNode, time: number, duration: number, type: BiquadFilterType, freq: number, q: number, level: number) {
  const source = ctx.createBufferSource();
  source.buffer = getSources(ctx).noise;
  const filter = ctx.createBiquadFilter();
  filter.type = type;
  filter.frequency.value = freq;
  filter.Q.value = q;
  const gain = ctx.createGain();
  gain.gain.setValueAtTime(0, time);
  gain.gain.linearRampToValueAtTime(level, time + duration * 0.3);
  gain.gain.linearRampToValueAtTime(0, time + duration);
  source.connect(filter).connect(gain).connect(bus);
  source.start(time, Math.random() * 0.5);
  source.stop(time + duration + 0.02);
}

/** Shapes the amplitude for the onset. Returns when the vowel formants are reached. An unknown onset is just the vowel. */
function shapeOnset(ctx: BaseAudioContext, bus: AudioNode, env: AudioParam, onset: string, t: number, k: number): number {
  const glide = 0.05 * k;
  switch (onset) {
    case 'd':
    case 'b':
      env.setValueAtTime(0, t + 0.025 * k);
      env.linearRampToValueAtTime(1, t + 0.05 * k);
      noiseBurst(ctx, bus, t + 0.02 * k, 0.018, 'bandpass', onset === 'd' ? 3200 : 1200, 1, onset === 'd' ? 0.12 : 0.07);
      return t + 0.05 * k + glide;
    case 'r':
      env.linearRampToValueAtTime(0.7, t + 0.015);
      env.linearRampToValueAtTime(0.12, t + 0.035 * k + 0.01);
      env.linearRampToValueAtTime(1, t + 0.06 * k + 0.015);
      return t + 0.06 * k + glide;
    case 'm':
      env.linearRampToValueAtTime(0.45, t + 0.015);
      env.setValueAtTime(0.45, t + 0.07 * k);
      env.linearRampToValueAtTime(1, t + 0.1 * k);
      return t + 0.08 * k + glide;
    case 'l':
      env.linearRampToValueAtTime(0.6, t + 0.02);
      env.linearRampToValueAtTime(1, t + 0.08 * k);
      return t + 0.06 * k + glide;
    case 'f':
      env.setValueAtTime(0, t + 0.07 * k);
      env.linearRampToValueAtTime(1, t + 0.1 * k);
      noiseBurst(ctx, bus, t, 0.09 * k, 'highpass', 1600, 0.7, 0.09);
      return t + 0.08 * k + glide * 0.6;
    case 's':
      env.setValueAtTime(0, t + 0.09 * k);
      env.linearRampToValueAtTime(1, t + 0.12 * k);
      noiseBurst(ctx, bus, t, 0.11 * k, 'bandpass', 6200, 1.6, 0.22);
      return t + 0.1 * k + glide * 0.6;
    default:
      env.linearRampToValueAtTime(1, t + 0.025);
      return t + 0.01;
  }
}

export function singFormant(ctx: BaseAudioContext, bus: AudioNode, time: number, midi: number, rawDuration: number, syllable: Syllable, voice: Voice, vibrato: boolean) {
  const shape = SHAPES[syllable];
  const table = FORMANTS[voice];
  const duration = Math.max(0.06, rawDuration);
  const k = Math.min(1, duration / 0.34); // squeezes consonants on short notes
  const f0 = midiToFreq(midi);

  const src = ctx.createOscillator();
  src.setPeriodicWave(getSources(ctx).glottal);
  src.frequency.setValueAtTime(f0 * 0.99, time);
  src.frequency.linearRampToValueAtTime(f0, time + 0.04 * k + 0.01);
  if (vibrato && duration > 0.35) {
    const lfo = ctx.createOscillator();
    const depth = ctx.createGain();
    lfo.frequency.value = 5.4;
    depth.gain.setValueAtTime(0, time + 0.18);
    depth.gain.linearRampToValueAtTime(f0 * 0.011, time + 0.4);
    lfo.connect(depth).connect(src.frequency);
    lfo.start(time);
    lfo.stop(time + duration + 0.05);
  }

  const env = ctx.createGain();
  env.gain.setValueAtTime(0, time);
  src.connect(env);
  const out = ctx.createGain();
  out.gain.value = voice === 'male' ? 1.1 : 0.85;
  out.connect(bus);

  const vowelFormants = table[shape.vowel];
  const startFormants = table[shape.onset] ?? vowelFormants;
  const vowelAt = shapeOnset(ctx, bus, env.gain, shape.onset, time, k);

  const release = Math.min(0.07, duration * 0.25);
  const end = time + duration;
  const codaAt = end - 0.1 * k - release;
  if (shape.coda === 'l') {
    env.gain.setValueAtTime(1, codaAt);
    env.gain.linearRampToValueAtTime(0.55, end - release);
  } else if (shape.coda === 'z') {
    env.gain.setValueAtTime(1, codaAt);
    env.gain.linearRampToValueAtTime(0.35, end - release);
    noiseBurst(ctx, bus, codaAt, 0.1 * k + release, 'bandpass', 5200, 1.4, 0.16);
  } else {
    env.gain.setValueAtTime(1, end - release);
  }
  env.gain.linearRampToValueAtTime(0, end);

  vowelFormants.forEach((freq, j) => {
    const band = ctx.createBiquadFilter();
    band.type = 'bandpass';
    band.Q.value = FORMANT_Q[j];
    band.frequency.setValueAtTime(startFormants[j], time);
    band.frequency.linearRampToValueAtTime(freq, Math.max(vowelAt, time + 0.005));
    if (shape.coda === 'l') {
      band.frequency.setValueAtTime(freq, codaAt);
      band.frequency.linearRampToValueAtTime(table.l[j], end - release);
    }
    const gain = ctx.createGain();
    gain.gain.value = FORMANT_GAIN[shape.vowel][j];
    env.connect(band).connect(gain).connect(out);
  });

  if (shape.onset === 'm') {
    const nasal = ctx.createBiquadFilter();
    nasal.type = 'lowpass';
    nasal.frequency.value = 380;
    const gain = ctx.createGain();
    gain.gain.setValueAtTime(0.5, time);
    gain.gain.linearRampToValueAtTime(0, time + 0.1 * k);
    env.connect(nasal).connect(gain).connect(out);
  }

  src.start(time);
  src.stop(end + 0.05);
}
