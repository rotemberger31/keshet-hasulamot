export function playClick(ctx: BaseAudioContext, bus: AudioNode, time: number, accent: boolean) {
  const osc = ctx.createOscillator();
  const gain = ctx.createGain();
  const highpass = ctx.createBiquadFilter();
  highpass.type = 'highpass';
  highpass.frequency.value = 600;
  osc.type = 'square';
  osc.frequency.value = accent ? 2000 : 1350;
  gain.gain.setValueAtTime(0, time);
  gain.gain.linearRampToValueAtTime(accent ? 0.35 : 0.22, time + 0.002);
  gain.gain.exponentialRampToValueAtTime(0.001, time + 0.05);
  osc.connect(highpass).connect(gain).connect(bus);
  osc.start(time);
  osc.stop(time + 0.06);
}
