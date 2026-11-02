import { useEffect, useRef } from 'react';
import { tapNote } from '../audio/player';
import { heName, noteColor, type Spelled } from '../music/notes';
import { useStore } from '../state/store';
import { subscribeVisual } from '../state/visual';
import styles from './Fretboard.module.css';
import { useScaleContext } from './useCurrentRoot';

const TUNING = [64, 59, 55, 50, 45, 40]; // high E on top
const FRETS = 12;
const NUT = 3;
const MARKERS = [3, 5, 7, 9];
const stringY = (i: number) => 8 + i * 16.8;

/** Real fret spacing: each fret is 2^(−1/12) the width of the one before it. */
const fretX = (fret: number) => NUT + ((100 - NUT) * (1 - 2 ** (-fret / 12))) / (1 - 2 ** (-(FRETS + 1) / 12));
const noteX = (fret: number) => (fret === 0 ? 1.6 : (fretX(fret - 1) + fretX(fret)) / 2);

export function Fretboard() {
  const { rootPc, notes } = useScaleContext();
  const singer = useStore((s) => s.singer);
  const octaveUp = singer === 'male' ? 0 : 12;
  const board = useRef<HTMLDivElement>(null);

  useEffect(() => {
    const mark = (pc: number | null) => {
      board.current?.querySelectorAll(`.${styles.cur}`).forEach((el) => el.classList.remove(styles.cur));
      if (pc !== null) board.current?.querySelectorAll(`[data-pc="${pc}"]`).forEach((el) => el.classList.add(styles.cur));
    };
    return subscribeVisual((event) => {
      if (event.type === 'note') mark(event.note?.spelled.pc ?? null);
      else if (event.type === 'tap') mark(event.spelled.pc);
      else if (event.type === 'stop') mark(null);
    });
  }, []);

  const inScale = new Map<number, Spelled>(notes.map((s) => [s.pc, s]));

  return (
    <div className={styles.scroll} dir="ltr">
      <div ref={board} className={styles.neck}>
        <svg viewBox="0 0 100 100" preserveAspectRatio="none" aria-hidden="true">
          {Array.from({ length: FRETS + 1 }, (_, i) => i + 1).map((f) => (
            <line key={f} x1={fretX(f)} x2={fretX(f)} y1="5" y2="95" stroke="var(--fret)" strokeWidth={f === 12 ? 3 : 1.5} vectorEffect="non-scaling-stroke" />
          ))}
          <rect x="2.4" y="5" width="1.2" height="90" fill="var(--string)" />
          {MARKERS.map((f) => (
            <circle key={f} cx={noteX(f)} cy="50" r="1.1" fill="var(--fret)" />
          ))}
          {TUNING.map((_, i) => (
            <line key={i} x1="3" x2="100" y1={stringY(i)} y2={stringY(i)} stroke="var(--string)" strokeWidth={1 + i * 0.35} vectorEffect="non-scaling-stroke" />
          ))}
        </svg>
        {TUNING.flatMap((open, string) =>
          Array.from({ length: FRETS + 1 }, (_, fret) => {
            const midi = open + fret;
            const spelled = inScale.get(midi % 12);
            if (!spelled) return null;
            return (
              <button
                key={`${string}-${fret}`}
                type="button"
                data-pc={spelled.pc}
                className={`${styles.dot} ${spelled.pc === rootPc ? styles.root : ''}`}
                style={{ left: `${noteX(fret)}%`, top: `${stringY(string)}%`, '--c': noteColor(spelled.pc) } as React.CSSProperties}
                aria-label={heName(spelled)}
                onClick={() => tapNote(midi + octaveUp)}
              >
                {heName(spelled)}
              </button>
            );
          }),
        )}
      </div>
    </div>
  );
}
