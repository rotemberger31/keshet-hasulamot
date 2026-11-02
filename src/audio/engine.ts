export interface Session {
  click: GainNode;
  note: GainNode;
  /** Cuts everything booked through this session, including what is still ahead of the clock. */
  kill: () => void;
}

interface Engine {
  ctx: AudioContext;
  clickVolume: GainNode;
  noteVolume: GainNode;
  free: Session;
}

let engine: Engine | null = null;
let volumes = { click: 0.7, note: 0.8 };
const readyListeners: ((ctx: AudioContext) => void)[] = [];

function applyVolumes() {
  if (!engine) return;
  engine.clickVolume.gain.value = volumes.click * 0.9;
  engine.noteVolume.gain.value = volumes.note;
}

function makeSession(e: Pick<Engine, 'ctx' | 'clickVolume' | 'noteVolume'>): Session {
  const { ctx } = e;
  const click = ctx.createGain();
  const note = ctx.createGain();
  click.connect(e.clickVolume);
  note.connect(e.noteVolume);
  return {
    click,
    note,
    kill() {
      for (const bus of [click, note]) {
        bus.gain.cancelScheduledValues(ctx.currentTime);
        bus.gain.setTargetAtTime(0, ctx.currentTime, 0.004);
      }
      setTimeout(() => {
        click.disconnect();
        note.disconnect();
      }, 60);
    },
  };
}

/** Must first run inside a user gesture, or the browser keeps the context suspended. */
export function getEngine(): Engine {
  if (engine) {
    if (engine.ctx.state !== 'running') void engine.ctx.resume();
    return engine;
  }
  const AudioCtor = window.AudioContext ?? (window as unknown as { webkitAudioContext: typeof AudioContext }).webkitAudioContext;
  const ctx = new AudioCtor();
  const compressor = ctx.createDynamicsCompressor();
  compressor.threshold.value = -14;
  compressor.ratio.value = 4;
  compressor.connect(ctx.destination);

  const master = ctx.createGain();
  master.gain.value = 0.9;
  master.connect(compressor);
  const clickVolume = ctx.createGain();
  const noteVolume = ctx.createGain();
  clickVolume.connect(master);
  noteVolume.connect(master);

  const parts = { ctx, clickVolume, noteVolume };
  engine = { ...parts, free: makeSession(parts) };
  applyVolumes();
  readyListeners.forEach((listener) => listener(ctx));
  return engine;
}

export const newSession = () => makeSession(getEngine());
export const currentContext = () => engine?.ctx ?? null;

export function setVolumes(click: number, note: number) {
  volumes = { click, note };
  applyVolumes();
}

export function onEngineReady(listener: (ctx: AudioContext) => void) {
  if (engine) listener(engine.ctx);
  else readyListeners.push(listener);
}
