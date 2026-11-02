import { PitchDetector } from 'pitchy';
import { getEngine } from './engine';

const WINDOW = 2048;
const MIN_CLARITY = 0.9;
const MIN_RMS = 0.01;
const MIN_HZ = 60;
const MAX_HZ = 1500;

export interface HeardPitch {
  midi: number;
  cents: number;
  pc: number;
}

export function toPitch(freq: number): HeardPitch {
  const exact = 69 + 12 * Math.log2(freq / 440);
  const midi = Math.round(exact);
  return { midi, cents: (exact - midi) * 100, pc: ((midi % 12) + 12) % 12 };
}

/** The pitch in a window of audio, or null when it's silent or not clearly pitched. */
export function detectPitch(buffer: Float32Array<ArrayBuffer>, sampleRate: number, detector = PitchDetector.forFloat32Array(buffer.length)): HeardPitch | null {
  let sum = 0;
  for (const sample of buffer) sum += sample * sample;
  if (Math.sqrt(sum / buffer.length) < MIN_RMS) return null;
  const [freq, clarity] = detector.findPitch(buffer, sampleRate);
  if (clarity < MIN_CLARITY || freq < MIN_HZ || freq > MAX_HZ) return null;
  return toPitch(freq);
}

export interface Mic {
  read: () => HeardPitch | null;
  close: () => void;
}

export async function openMic(): Promise<Mic> {
  const { ctx } = getEngine();
  const stream = await navigator.mediaDevices.getUserMedia({
    audio: { echoCancellation: true, noiseSuppression: false, autoGainControl: false },
  });
  const source = ctx.createMediaStreamSource(stream);
  const analyser = ctx.createAnalyser();
  analyser.fftSize = WINDOW;
  source.connect(analyser);
  const buffer = new Float32Array(WINDOW);
  const detector = PitchDetector.forFloat32Array(WINDOW);
  return {
    read() {
      analyser.getFloatTimeDomainData(buffer);
      return detectPitch(buffer, ctx.sampleRate, detector);
    },
    close() {
      source.disconnect();
      stream.getTracks().forEach((track) => track.stop());
    },
  };
}
