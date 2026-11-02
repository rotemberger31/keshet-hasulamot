import { mod, noteColor } from '../music/notes';
import { SCALES } from '../music/scales';
import { CATEGORIES } from '../music/scales.data';
import { useStore } from '../state/store';
import styles from './ScalePicker.module.css';
import { Seg } from './Seg';
import ui from './ui.module.css';

const OPTIONS = CATEGORIES.map((c) => ({ value: c.id, label: c.he }));

export function ScalePicker() {
  const category = useStore((s) => s.category);
  const scaleId = useStore((s) => s.scaleId);
  const rootPc = useStore((s) => s.rootPc);
  const set = useStore((s) => s.set);

  return (
    <section className={`${ui.panel} ${ui.stack}`} aria-label="סולם">
      <p className={ui.label} style={{ margin: 0 }}>סולם</p>
      <Seg label="קטגוריה" value={category} options={OPTIONS} onChange={(value) => set({ category: value })} />
      <div className={styles.list}>
        {SCALES.filter((s) => s.cat === category).map((scale) => (
          <button key={scale.id} type="button" className={styles.scale} aria-pressed={scale.id === scaleId} onClick={() => set({ scaleId: scale.id })}>
            <span>
              <b>{scale.he}</b>
              <small dir="ltr">{scale.en}</small>
            </span>
            <span className={styles.dots} dir="ltr" aria-hidden="true">
              {scale.pcs.map((pc) => (
                <i key={pc} style={{ background: noteColor(mod(rootPc + pc, 12)) }} />
              ))}
            </span>
          </button>
        ))}
      </div>
    </section>
  );
}
