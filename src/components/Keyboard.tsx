import { useEffect, useRef } from 'react';
import { tapNote } from '../audio/player';
import { enName, heName, LETTER_PC, LETTERS, mod, noteColor, type Spelled } from '../music/notes';
import { useStore } from '../state/store';
import { subscribeVisual } from '../state/visual';
import styles from './Keyboard.module.css';
import { useMediaQuery } from './useMediaQuery';
import { useScaleContext } from './useCurrentRoot';

const BLACK = new Set([1, 3, 6, 8, 10]);

export function Keyboard() {
  const { rootPc, notes } = useScaleContext();
  const singer = useStore((s) => s.singer);
  const compact = useMediaQuery('(max-width: 900px)');
  const octaves = compact ? 1 : 2;
  const base = singer === 'male' ? 48 : 60;
  const top = base + 12 * octaves;
  const board = useRef<HTMLDivElement>(null);

  useEffect(() => {
    const mark = (className: string, midi: number | null) => {
      board.current?.querySelector(`.${className}`)?.classList.remove(className);
      if (midi === null) return;
      const fitted = base + mod(midi - base, top - base + 1 > 13 ? 24 : 12);
      board.current?.querySelector(`[data-midi="${fitted}"]`)?.classList.add(className);
    };
    return subscribeVisual((event) => {
      if (event.type === 'note') mark(styles.cur, event.note?.midi ?? null);
      else if (event.type === 'tap') mark(styles.cur, event.midi);
      else if (event.type === 'stop') mark(styles.cur, null);
      else if (event.type === 'mic') mark(styles.mic, event.midi);
    });
  }, [base, top]);

  const inScale = new Map<number, Spelled>(notes.map((s) => [s.pc, s]));
  const midis = Array.from({ length: top - base + 1 }, (_, i) => base + i);
  const whiteCount = midis.filter((m) => !BLACK.has(m % 12)).length;
  const width = 100 / whiteCount;
  let whites = 0;

  return (
    <div ref={board} className={styles.keyboard} dir="ltr">
      {midis.map((midi) => {
        const pc = midi % 12;
        const black = BLACK.has(pc);
        const left = black ? whites * width - width * 0.32 : whites * width;
        if (!black) whites++;
        const spelled = inScale.get(pc);
        const classes = [styles.key, black ? styles.black : styles.white, spelled && styles.in, pc === rootPc && styles.root];
        return (
          <button
            key={midi}
            type="button"
            data-midi={midi}
            className={classes.filter(Boolean).join(' ')}
            style={{ left: `${left}%`, width: `${black ? width * 0.64 : width}%`, '--c': noteColor(pc) } as React.CSSProperties}
            aria-label={spelled ? heName(spelled) : LETTERS[(LETTER_PC as readonly number[]).indexOf(pc)] ?? 'קליד'}
            onPointerDown={(event) => {
              event.preventDefault();
              tapNote(midi);
            }}
            onKeyDown={(event) => {
              if (event.key === 'Enter' || event.key === ' ') {
                event.preventDefault();
                tapNote(midi);
              }
            }}
          >
            {spelled ? (
              <>
                <span>{heName(spelled)}</span>
                {!black && <span>{enName(spelled)}</span>}
              </>
            ) : (
              !black && <span className={styles.faint}>{LETTERS[(LETTER_PC as readonly number[]).indexOf(pc)]}</span>
            )}
          </button>
        );
      })}
    </div>
  );
}
