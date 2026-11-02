import type { Singer } from '../music/sequence';
import { useStore } from '../state/store';
import { Seg } from './Seg';
import ui from './ui.module.css';

const SINGERS: { value: Singer; label: string }[] = [
  { value: 'female', label: 'קול אישה' },
  { value: 'male', label: 'קול גבר' },
  { value: 'piano', label: 'פסנתר' },
  { value: 'silent', label: 'אני אומר' },
];

export function VoiceControls() {
  const singer = useStore((s) => s.singer);
  const pianoUnder = useStore((s) => s.pianoUnder);
  const clickVolume = useStore((s) => s.clickVolume);
  const noteVolume = useStore((s) => s.noteVolume);
  const set = useStore((s) => s.set);

  return (
    <section className={ui.panel} aria-label="מי שר">
      <p className={ui.label}>מי שר</p>
      <Seg label="מי שר" value={singer} options={SINGERS} onChange={(value) => set({ singer: value })} />
      {(singer === 'female' || singer === 'male') && (
        <label className={ui.check}>
          <input type="checkbox" checked={pianoUnder} onChange={(e) => set({ pianoUnder: e.target.checked })} /> פסנתר שקט מתחת לקול
        </label>
      )}
      <label className={ui.volume}>
        מטרונום
        <input type="range" min={0} max={1} step={0.05} value={clickVolume} onChange={(e) => set({ clickVolume: Number(e.target.value) })} />
      </label>
      <label className={ui.volume}>
        צליל
        <input type="range" min={0} max={1} step={0.05} value={noteVolume} onChange={(e) => set({ noteVolume: Number(e.target.value) })} />
      </label>
    </section>
  );
}
