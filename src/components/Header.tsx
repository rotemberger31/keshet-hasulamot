import { LETTER_PC, noteColor } from '../music/notes';
import { useStore, type Theme } from '../state/store';
import styles from './Header.module.css';
import ui from './ui.module.css';

const NEXT_THEME: Record<Theme, Theme> = { auto: 'dark', dark: 'light', light: 'auto' };
const THEME_LABEL: Record<Theme, string> = { auto: 'מצב: אוטומטי', dark: 'מצב: כהה', light: 'מצב: בהיר' };

export function Header() {
  const theme = useStore((s) => s.theme);
  const set = useStore((s) => s.set);
  const surprise = useStore((s) => s.surprise);

  return (
    <header className={styles.header}>
      <div>
        <h1 className={styles.name}>
          קשת הסולמות
          <span className={styles.bow} aria-hidden="true">
            {LETTER_PC.map((pc) => (
              <i key={pc} style={{ background: noteColor(pc) }} />
            ))}
          </span>
        </h1>
        <p className={styles.tag}>בחר צליל יסוד וסולם, לחץ נגן, ואמור את שם הצליל על כל פעמה.</p>
      </div>
      <div className={styles.actions}>
        <button type="button" className={ui.mini} onClick={() => set({ theme: NEXT_THEME[theme] })}>
          {THEME_LABEL[theme]}
        </button>
        <button type="button" className={ui.mini} onClick={surprise}>
          הפתע אותי
        </button>
      </div>
    </header>
  );
}
