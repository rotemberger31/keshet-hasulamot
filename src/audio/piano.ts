import { midiToFreq } from '../music/notes';

const PARTIALS: [number, number][] = [[1, 1], [2, 0.42], [3, 0.22], [4, 0.12], [5, 0.07], [6, 0.04]];

export function playPiano(ctx: BaseAudioContext, bus: AudioNode, time: number, midi: number, duration: number, velocity = 0.5) {
  const freq = midiToFreq(midi);
  const held = Math.max(duration, 0.25);
  const end = time + held + 0.7;

  const lowpass = ctx.createBiquadFilter();
  lowpass.type = 'lowpass';
  lowpass.frequency.setValueAtTime(Math.min(9000, freq * 9), time);
  lowpass.frequency.exponentialRampToValueAtTime(Math.max(freq * 2.5, 500), time + 1.4);

  const out = ctx.createGain();
  out.gain.setValueAtTime(0, time);
  out.gain.linearRampToValueAtTime(velocity, time + 0.004);
  out.gain.exponentialRampToValueAtTime(velocity * 0.35, time + 0.3);
  out.gain.setTargetAtTime(0.0001, time + held, 0.18);

  PARTIALS.forEach(([harmonic, amp], i) => {
    const osc = ctx.createOscillator();
    const gain = ctx.createGain();
    osc.frequency.value = freq * harmonic * (1 + 0.0007 * i); // slight inharmonicity
    gain.gain.value = amp * 0.3;
    osc.connect(gain).connect(lowpass);
    osc.start(time);
    osc.stop(end);
  });
  lowpass.connect(out).connect(bus);
}
