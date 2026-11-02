import { useEffect, useRef, useState } from 'react';
import { openMic, type HeardPitch, type Mic } from '../audio/pitch';
import { heName, noteColor, type Spelled } from '../music/notes';
import type { SeqNote } from '../music/sequence';
import { selectSequence, useStore } from '../state/store';
import { emitVisual, subscribeVisual } from '../state/visual';
import { spellRoot } from '../music/spelling';
import styles from './MicPanel.module.css';
import ui from './ui.module.css';
import { useScaleContext } from './useCurrentRoot';

const HIT_CENTS = 45;
const READ_EVERY_FRAMES = 3;
const INTRO = 'שיר או נגן יחד עם הסולם. צליל שתפגע בו יקבל טבעת ירוקה. עדיף עם אוזניות, כדי שהמיקרופון לא ישמע את האפליקציה.';

function micErrorMessage(error: unknown) {
  if (!navigator.mediaDevices) return 'המיקרופון זמין רק בכתובת מאובטחת (https או localhost).';
  if (error instanceof DOMException && error.name === 'NotAllowedError') return 'אין הרשאה למיקרופון. אשר את הגישה בהגדרות הדפדפן ונסה שוב.';
  if (error instanceof DOMException && error.name === 'NotFoundError') return 'לא נמצא מיקרופון במכשיר.';
  return 'לא הצלחתי להפעיל את המיקרופון.';
}

export function MicPanel() {
  const { notes, preferFlats } = useScaleContext();
  const sequence = useStore(selectSequence);
  const total = sequence.filter(Boolean).length;
  const [mic, setMic] = useState<Mic | null>(null);
  const [message, setMessage] = useState(INTRO);
  const [heard, setHeard] = useState<HeardPitch | null>(null);
  const [score, setScore] = useState(0);
  const [roundActive, setRoundActive] = useState(false);
  const current = useRef<{ index: number; note: SeqNote | null } | null>(null);
  const hits = useRef(new Set<number>());

  useEffect(
    () =>
      subscribeVisual((event) => {
        if (event.type === 'note') {
          current.current = { index: event.index, note: event.note };
          setRoundActive(true);
          if (event.index === 0) {
            hits.current = new Set();
            setScore(0);
          }
        } else if (event.type === 'stop') {
          current.current = null;
          setRoundActive(false);
        }
      }),
    [],
  );

  useEffect(() => {
    if (!mic) return;
    let frame = 0;
    let id = 0;
    const loop = () => {
      id = requestAnimationFrame(loop);
      if (frame++ % READ_EVERY_FRAMES) return;
      const pitch = mic.read();
      setHeard(pitch);
      emitVisual({ type: 'mic', midi: pitch?.midi ?? null });
      const now = current.current;
      if (pitch && now?.note && now.note.spelled.pc === pitch.pc && Math.abs(pitch.cents) <= HIT_CENTS && !hits.current.has(now.index)) {
        hits.current.add(now.index);
        setScore(hits.current.size);
        emitVisual({ type: 'hit', index: now.index });
      }
    };
    id = requestAnimationFrame(loop);
    return () => {
      cancelAnimationFrame(id);
      mic.close();
      emitVisual({ type: 'mic', midi: null });
    };
  }, [mic]);

  const toggle = async () => {
    if (mic) {
      setMic(null);
      setHeard(null);
      return;
    }
    try {
      setMic(await openMic());
      setMessage(INTRO);
    } catch (error) {
      setMessage(micErrorMessage(error));
    }
  };

  let shown: Spelled | null = null;
  if (heard) shown = notes.find((s) => s.pc === heard.pc) ?? spellRoot(heard.pc, preferFlats);


  return (
    <section className={ui.panel} aria-label="מיקרופון">
      <div className={ui.row}>
        <p className={ui.label}>מיקרופון</p>
        {mic && roundActive && <span className={styles.score}>{score} / {total}</span>}
      </div>
      <button type="button" className={ui.mini} onClick={toggle}>
        {mic ? 'כבה מיקרופון' : 'הפעל מיקרופון'}
      </button>
      {mic && (
        <div className={styles.read}>
          <div className={styles.note} style={shown ? { background: noteColor(shown.pc) } : undefined}>
            {shown ? heName(shown) : '–'}
          </div>
          <div className={styles.cents} dir="ltr" role="img" aria-label="סטייה בסנטים">
            <i style={{ left: `${50 + (heard?.cents ?? 0)}%` }} />
          </div>
        </div>
      )}
      <p className={styles.message}>{message}</p>
    </section>
  );
}
