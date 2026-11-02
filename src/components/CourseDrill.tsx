import { play } from '../audio/player';
import { HEBREW, LETTER_PC, noteColor } from '../music/notes';
import { getScale, WHITE_KEY_MODES } from '../music/scales';
import { useStore } from '../state/store';
import styles from './CourseDrill.module.css';
import ui from './ui.module.css';

export function CourseDrill() {
  const rootPc = useStore((s) => s.rootPc);
  const scaleId = useStore((s) => s.scaleId);
  const courseDrill = useStore((s) => s.courseDrill);

  const start = (letter: number) => {
    courseDrill(letter);
    if (!useStore.getState().playing) play();
  };

  return (
    <section className={`${ui.panel} ${ui.stack}`} aria-label="תרגיל הקורס">
      <div>
        <h2 className={ui.title}>רצף צלילים לפי טמפו</h2>
        <p className={ui.muted}>אמור צליל אחד בכל קליק עד שזה זורם ב-70BPM. כל שורה בתרגיל, על הקלידים הלבנים, היא בעצם מודוס אחר.</p>
      </div>
      <div className={styles.grid}>
        {HEBREW.map((name, letter) => {
          const pc = LETTER_PC[letter];
          const mode = getScale(WHITE_KEY_MODES[letter]);
          return (
            <button key={name} type="button" className={styles.drill} aria-pressed={rootPc === pc && scaleId === mode.id} onClick={() => start(letter)}>
              <i style={{ background: noteColor(pc) }} />
              <span>
                <b>{name} עד {name}</b>
                <small>{mode.he}</small>
              </span>
            </button>
          );
        })}
      </div>
    </section>
  );
}
