import { relatedScales } from '../music/related';
import { useStore } from '../state/store';
import { NoteChips } from './NoteChips';
import styles from './Practice.module.css';
import { Transport } from './Transport';
import ui from './ui.module.css';
import { useScaleContext } from './useCurrentRoot';

/** The exercise as a strip of notes, the tempo, and a nudge towards the next scale. */
export function Practice() {
  const { scale } = useScaleContext();
  const chooseScale = useStore((s) => s.chooseScale);
  const neighbours = relatedScales(scale).slice(0, 3);

  return (
    <section className={`${ui.panel} ${ui.stack}`} aria-label="התרגיל">
      <NoteChips />
      <Transport />
      {neighbours.length > 0 && (
        <div className={styles.next}>
          <span>נסה גם:</span>
          {neighbours.map((n) => (
            <button key={n.scale.id} type="button" onClick={() => chooseScale(n.scale.id)}>
              {n.scale.he}
            </button>
          ))}
        </div>
      )}
    </section>
  );
}
