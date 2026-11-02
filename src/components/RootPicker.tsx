import { enName, heName, noteColor } from '../music/notes';
import { spellRoot } from '../music/spelling';
import { useStore } from '../state/store';
import styles from './RootPicker.module.css';
import ui from './ui.module.css';

export function RootPicker() {
  const rootPc = useStore((s) => s.rootPc);
  const preferFlats = useStore((s) => s.preferFlats);
  const set = useStore((s) => s.set);

  return (
    <section className={ui.panel} aria-label="צליל יסוד">
      <div className={ui.row}>
        <p className={ui.label}>צליל יסוד</p>
        <button type="button" className={ui.mini} onClick={() => set({ preferFlats: !preferFlats })} aria-label="החלף בין דיאז לבמול">
          ♯ / ♭
        </button>
      </div>
      <div className={styles.roots}>
        {Array.from({ length: 12 }, (_, pc) => {
          const spelled = spellRoot(pc, preferFlats);
          return (
            <button
              key={pc}
              type="button"
              className={styles.root}
              style={{ '--c': noteColor(pc) } as React.CSSProperties}
              aria-pressed={pc === rootPc}
              onClick={() => set({ rootPc: pc })}
            >
              {heName(spelled)}
              <small dir="ltr">{enName(spelled)}</small>
            </button>
          );
        })}
      </div>
    </section>
  );
}
