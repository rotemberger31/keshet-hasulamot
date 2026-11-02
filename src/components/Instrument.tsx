import { useStore, type Instrument as InstrumentId } from '../state/store';
import { Fretboard } from './Fretboard';
import { Keyboard } from './Keyboard';
import { Seg } from './Seg';
import ui from './ui.module.css';

const OPTIONS: { value: InstrumentId; label: string }[] = [
  { value: 'piano', label: 'פסנתר' },
  { value: 'guitar', label: 'גיטרה' },
];

export function Instrument() {
  const instrument = useStore((s) => s.instrument);
  const set = useStore((s) => s.set);
  return (
    <section className={`${ui.panel} ${ui.stack}`} aria-label="כלי">
      <div className={ui.row}>
        <h2 className={ui.title}>על הכלי</h2>
        <Seg label="בחירת כלי" value={instrument} options={OPTIONS} onChange={(value) => set({ instrument: value })} />
      </div>
      {instrument === 'piano' ? <Keyboard /> : <Fretboard />}
    </section>
  );
}
