import { useRef } from 'react';
import { useStore } from '../state/store';
import styles from './Transport.module.css';
import ui from './ui.module.css';

const TAP_RESET_MS = 2000;

export function Transport() {
  const bpm = useStore((s) => s.bpm);
  const countIn = useStore((s) => s.countIn);
  const loop = useStore((s) => s.loop);
  const setBpm = useStore((s) => s.setBpm);
  const set = useStore((s) => s.set);
  const taps = useRef<number[]>([]);

  const tap = () => {
    const now = performance.now();
    if (now - (taps.current.at(-1) ?? 0) > TAP_RESET_MS) taps.current = [];
    taps.current = [...taps.current, now].slice(-5);
    const count = taps.current.length;
    if (count > 1) setBpm((60000 * (count - 1)) / (now - taps.current[0]));
  };

  return (
    <div className={styles.transport}>
      <div className={styles.bpm}>
        <button type="button" onClick={() => setBpm(bpm - 1)} aria-label="האט">−</button>
        <div>
          <div className={styles.value}>{bpm}</div>
          <div className={styles.unit}>BPM</div>
        </div>
        <button type="button" onClick={() => setBpm(bpm + 1)} aria-label="האץ">+</button>
      </div>
      <button type="button" className={ui.mini} onClick={tap}>
        הקש קצב
      </button>
      <input className={styles.slider} type="range" min={30} max={200} value={bpm} onChange={(e) => setBpm(Number(e.target.value))} aria-label="מהירות" />
      <div className={styles.toggles}>
        <label className={ui.check}>
          <input type="checkbox" checked={countIn} onChange={(e) => set({ countIn: e.target.checked })} /> ספירה לפני
        </label>
        <label className={ui.check}>
          <input type="checkbox" checked={loop} onChange={(e) => set({ loop: e.target.checked })} /> בלופ
        </label>
      </div>
    </div>
  );
}
