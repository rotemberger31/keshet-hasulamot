import { mod, type Spelled } from '../music/notes';
import { getScale } from '../music/scales';
import type { SeqNote, Singer } from '../music/sequence';
import { spellDegree, spellRoot } from '../music/spelling';
import { selectSequence, useStore } from '../state/store';
import { clearVisualQueue, emitVisual, enqueueVisual, startVisualLoop } from '../state/visual';
import { getEngine, newSession, onEngineReady, setVolumes, type Session } from './engine';
import { playClick } from './metronome';
import { playPiano } from './piano';
import { createScheduler, type Step } from './scheduler';
import { loadVoice, singNote } from './voice';

const NOTE_LENGTH = 0.92; // of a step, so repeated notes still articulate

let session: Session | null = null;
let sequence: (SeqNote | null)[] = [];

function sound(ctx: BaseAudioContext, bus: AudioNode, time: number, midi: number, duration: number, spelled: Spelled, singer: Singer, pianoUnder: boolean) {
  if (singer === 'female' || singer === 'male') {
    singNote(ctx, bus, time, midi, duration, spelled, singer);
    if (pianoUnder) playPiano(ctx, bus, time, midi, duration, 0.18);
  } else if (singer === 'piano') {
    playPiano(ctx, bus, time, midi, duration, 0.5);
  }
}

function onStep(step: Step) {
  if (!session) return;
  const { ctx } = getEngine();
  if (step.beat >= 0) playClick(ctx, session.click, step.time, step.beat === 0);
  if (step.kind === 'count') {
    enqueueVisual(step.time, { type: 'count', beat: step.beat });
    return;
  }
  const note = sequence[step.index] ?? null;
  enqueueVisual(step.time, { type: 'note', index: step.index, note, beat: step.beat });
  if (note) {
    const { singer, pianoUnder } = useStore.getState();
    sound(ctx, session.note, step.time, note.midi, step.duration * NOTE_LENGTH, note.spelled, singer, pianoUnder);
  }
}

const scheduler = createScheduler({
  now: () => getEngine().ctx.currentTime,
  setTimer: (fn, ms) => setInterval(fn, ms),
  clearTimer: (id) => clearInterval(id as ReturnType<typeof setInterval>),
  onStep,
  onEnd: (time) => {
    const wait = Math.max(0, time - getEngine().ctx.currentTime);
    setTimeout(stop, wait * 1000);
  },
});

function loadCurrentVoice() {
  const { singer } = useStore.getState();
  if (singer === 'female' || singer === 'male') void loadVoice(getEngine().ctx, singer);
}

export function play() {
  stop();
  const engine = getEngine();
  const state = useStore.getState();
  session = newSession();
  sequence = selectSequence(state);
  loadCurrentVoice();
  useStore.getState().setPlaying(true);
  void engine.ctx.resume().then(() => {
    if (!session) return;
    scheduler.start({ length: sequence.length, sub: state.sub, bpm: state.bpm, loop: state.loop, countIn: state.countIn });
  });
}

export function stop() {
  scheduler.stop();
  session?.kill();
  session = null;
  clearVisualQueue();
  if (useStore.getState().playing) useStore.getState().setPlaying(false);
  emitVisual({ type: 'stop' });
}

export const togglePlay = () => (useStore.getState().playing ? stop() : play());

/** Tapping a key or fret: says the note as this scale spells it. */
export function tapNote(midi: number) {
  const engine = getEngine();
  loadCurrentVoice();
  const { scaleId, rootPc, preferFlats, singer, pianoUnder, playing } = useStore.getState();
  const pc = mod(midi, 12);
  const degree = getScale(scaleId).degrees.find((d) => mod(rootPc + d.semi, 12) === pc);
  const spelled = degree ? spellDegree(rootPc, preferFlats, degree) : spellRoot(pc, preferFlats);
  sound(engine.ctx, engine.free.note, engine.ctx.currentTime + 0.01, midi, 0.75, spelled, singer === 'silent' ? 'piano' : singer, pianoUnder);
  if (!playing) emitVisual({ type: 'tap', midi, spelled, degreeLabel: degree?.label ?? null });
}

onEngineReady((ctx) => {
  startVisualLoop(() => ctx.currentTime - (ctx.outputLatency || 0));
});

useStore.subscribe((state, previous) => {
  if (state.clickVolume !== previous.clickVolume || state.noteVolume !== previous.noteVolume)
    setVolumes(state.clickVolume, state.noteVolume);
  if (!state.playing) return;
  const next = selectSequence(state);
  if (next !== sequence) {
    sequence = next;
    scheduler.update({ length: next.length, sub: state.sub });
    loadCurrentVoice();
  }
  if (state.bpm !== previous.bpm) scheduler.update({ bpm: state.bpm });
  if (state.loop !== previous.loop) scheduler.update({ loop: state.loop });
});

const initial = useStore.getState();
setVolumes(initial.clickVolume, initial.noteVolume);
