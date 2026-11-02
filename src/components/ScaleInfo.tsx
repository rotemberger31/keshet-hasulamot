import { relatedScales, scaleSteps } from '../music/related';
import { useStore } from '../state/store';
import styles from './ScaleInfo.module.css';
import ui from './ui.module.css';
import { useScaleContext } from './useCurrentRoot';

const STEP_LABEL: Record<number, string> = { 1: 'ח', 2: 'ט', 3: 'ט+ח', 4: '2ט' };

export function ScaleInfo() {
  const { scale } = useScaleContext();
  const chooseScale = useStore((s) => s.chooseScale);
  const related = relatedScales(scale);

  return (
    <section className={`${ui.panel} ${ui.stack}`} aria-label="על הסולם">
      <div>
        <h2 className={ui.title}>{scale.he}</h2>
        <p className={styles.desc}>{scale.desc}</p>
      </div>
      <div>
        <p className={ui.label}>מרווחים (ט = טון, ח = חצי טון)</p>
        <div className={styles.steps}>
          {scaleSteps(scale).map((step, i) => (
            <div key={i} style={{ flexGrow: step }}>
              {STEP_LABEL[step] ?? step}
            </div>
          ))}
        </div>
      </div>
      <div>
        <p className={ui.label}>דרגות לעומת מז'ור</p>
        <div className={styles.degrees}>
          {scale.degrees.map((degree) => (
            <span key={degree.label} className={degree.acc ? styles.altered : ''} dir="ltr">
              {degree.label}
            </span>
          ))}
        </div>
      </div>
      <div>
        <p className={ui.label}>שנה צליל אחד וקבל</p>
        {related.length ? (
          <div className={styles.related}>
            {related.map((r) => (
              <button key={r.scale.id} type="button" onClick={() => chooseScale(r.scale.id)}>
                <b>{r.scale.he}</b>
                <span dir="ltr">{r.from} → {r.to}</span>
              </button>
            ))}
          </div>
        ) : (
          <p className={ui.muted}>לסולם הזה אין שכן במרחק צליל אחד. נסה את "הפתע אותי".</p>
        )}
      </div>
    </section>
  );
}
