import { useEffect, useState } from 'react';
import { togglePlay } from '../audio/player';
import { enName, heName, noteColor, type Spelled } from '../music/notes';
import { useStore } from '../state/store';
import { subscribeVisual } from '../state/visual';
import styles from './NowCard.module.css';
import { useScaleContext } from './useCurrentRoot';

interface Shown {
  spelled: Spelled | null; // null during a rest
  caption: string;
  stamp: number;
}

const COUNT_WORDS = ['1', '2', '3', '4'];

export function NowCard() {
  const { scale, root } = useScaleContext();
  const playing = useStore((s) => s.playing);
  const [shown, setShown] = useState<Shown | null>(null);
  const [beat, setBeat] = useState(-1);
  const [count, setCount] = useState('');

  useEffect(
    () =>
      subscribeVisual((event) => {
        if (event.type === 'count') {
          setBeat(event.beat);
          setCount(COUNT_WORDS[event.beat]);
        } else if (event.type === 'note') {
          setCount('');
          if (event.beat >= 0) setBeat(event.beat);
          setShown((prev) => ({
            spelled: event.note?.spelled ?? null,
            caption: event.note ? `דרגה ${event.note.degreeLabel}` : 'נשימה',
            stamp: (prev?.stamp ?? 0) + 1,
          }));
        } else if (event.type === 'tap') {
          setShown((prev) => ({
            spelled: event.spelled,
            caption: event.degreeLabel ? `דרגה ${event.degreeLabel}` : 'לא בסולם',
            stamp: (prev?.stamp ?? 0) + 1,
          }));
        } else if (event.type === 'stop') {
          setShown(null);
          setBeat(-1);
          setCount('');
        }
      }),
    [],
  );

  const spelled = shown ? shown.spelled : root;
  const color = noteColor((spelled ?? root).pc);

  return (
    <section className={styles.now} style={{ '--nc': color } as React.CSSProperties} aria-label="הצליל הנוכחי">
      <div key={shown?.stamp} className={`${styles.big} ${shown ? styles.pop : ''}`}>
        {spelled ? heName(spelled) : ''}
      </div>
      <div className={styles.meta}>
        <div className={styles.scaleName}>
          {heName(root)} {scale.he}
          <small dir="ltr">
            {enName(root)} {scale.en}
          </small>
        </div>
        <div className={styles.line}>
          <span className={styles.letter} dir="ltr">
            {spelled ? enName(spelled) : ''}
          </span>
          <span className={styles.degree}>{shown ? shown.caption : 'צליל היסוד'}</span>
        </div>
        <div className={styles.beats} aria-hidden="true">
          {[0, 1, 2, 3].map((b) => (
            <i key={b} className={`${b === 0 ? styles.first : ''} ${b === beat ? styles.on : ''}`} />
          ))}
          <b>{count}</b>
        </div>
      </div>
      <button type="button" className={styles.play} onClick={togglePlay} aria-label={playing ? 'עצור' : 'נגן'}>
        <svg viewBox="0 0 24 24" fill="currentColor" aria-hidden="true">
          {playing ? <rect x="6" y="6" width="12" height="12" rx="2" /> : <path d="M18 12 7 18.5v-13z" />}
        </svg>
      </button>
    </section>
  );
}
