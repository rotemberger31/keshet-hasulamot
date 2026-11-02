import type { Direction, Pattern, Subdivision } from '../music/sequence';
import { useStore } from '../state/store';
import { Seg } from './Seg';
import ui from './ui.module.css';

const DIRECTIONS: { value: Direction; label: string }[] = [
  { value: 'up', label: 'עולה' },
  { value: 'down', label: 'יורד' },
  { value: 'updown', label: 'עולה ויורד' },
];
const PATTERNS: { value: Pattern; label: string }[] = [
  { value: 'scale', label: 'סולם' },
  { value: 'thirds', label: 'טרצות' },
  { value: 'arp', label: "ארפג'ים" },
];
const SUBS: { value: Subdivision; label: string }[] = [
  { value: 1, label: '1' },
  { value: 2, label: '2' },
  { value: 3, label: '3 (טריולה)' },
  { value: 4, label: '4' },
];

export function PatternControls() {
  const direction = useStore((s) => s.direction);
  const pattern = useStore((s) => s.pattern);
  const sub = useStore((s) => s.sub);
  const set = useStore((s) => s.set);

  return (
    <section className={`${ui.panel} ${ui.stack}`} aria-label="כיוון ותבנית">
      <div>
        <p className={ui.label}>כיוון</p>
        <Seg label="כיוון" value={direction} options={DIRECTIONS} onChange={(value) => set({ direction: value })} />
      </div>
      <div>
        <p className={ui.label}>תבנית</p>
        <Seg label="תבנית" value={pattern} options={PATTERNS} onChange={(value) => set({ pattern: value })} />
      </div>
      <div>
        <p className={ui.label}>צלילים בכל פעמה</p>
        <Seg label="צלילים בכל פעמה" value={sub} options={SUBS} onChange={(value) => set({ sub: value })} />
      </div>
    </section>
  );
}
