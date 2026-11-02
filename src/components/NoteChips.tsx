import { useEffect, useRef } from 'react';
import { enName, heName, noteColor } from '../music/notes';
import { selectSequence, useStore } from '../state/store';
import { subscribeVisual } from '../state/visual';
import styles from './NoteChips.module.css';

export function NoteChips() {
  const sequence = useStore(selectSequence);
  const strip = useRef<HTMLDivElement>(null);

  useEffect(() => {
    let current = -1;
    const chip = (i: number) => strip.current?.children[i];
    const clearHits = () => {
      for (const el of strip.current?.children ?? []) el.classList.remove(styles.hit);
    };
    return subscribeVisual((event) => {
      if (event.type === 'note') {
        chip(current)?.classList.remove(styles.cur);
        chip(event.index)?.classList.add(styles.cur);
        if (event.index === 0) clearHits();
        current = event.index;
      } else if (event.type === 'hit') {
        chip(event.index)?.classList.add(styles.hit);
      } else if (event.type === 'stop') {
        chip(current)?.classList.remove(styles.cur);
        clearHits();
        current = -1;
      }
    });
  }, []);

  return (
    <div ref={strip} className={`${styles.strip} ${sequence.length > 24 ? styles.dense : ''}`}>
      {sequence.map((note, i) =>
        note ? (
          <div key={i} className={styles.chip} style={{ '--c': noteColor(note.spelled.pc) } as React.CSSProperties}>
            <b>{heName(note.spelled)}</b>
            <span dir="ltr">{enName(note.spelled)}</span>
          </div>
        ) : (
          <div key={i} className={`${styles.chip} ${styles.rest}`}>
            <b>·</b>
          </div>
        ),
      )}
    </div>
  );
}
