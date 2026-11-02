import { useEffect } from 'react';
import { togglePlay } from '../audio/player';
import { useStore } from '../state/store';
import styles from './App.module.css';
import { CourseDrill } from './CourseDrill';
import { Header } from './Header';
import { Instrument } from './Instrument';
import { MicPanel } from './MicPanel';
import { NowCard } from './NowCard';
import { PatternControls } from './PatternControls';
import { Practice } from './Practice';
import { RootPicker } from './RootPicker';
import { ScaleInfo } from './ScaleInfo';
import { ScalePicker } from './ScalePicker';
import { VoiceControls } from './VoiceControls';

export function App() {
  const theme = useStore((s) => s.theme);

  useEffect(() => {
    if (theme === 'auto') delete document.documentElement.dataset.theme;
    else document.documentElement.dataset.theme = theme;
  }, [theme]);

  useEffect(() => {
    const onKey = (event: KeyboardEvent) => {
      if (event.code !== 'Space' || /INPUT|BUTTON/.test((event.target as HTMLElement).tagName)) return;
      event.preventDefault();
      togglePlay();
    };
    document.addEventListener('keydown', onKey);
    return () => document.removeEventListener('keydown', onKey);
  }, []);

  return (
    <div className={styles.app}>
      <Header />
      <NowCard />
      <main className={styles.main}>
        <Practice />
        <Instrument />
        <ScaleInfo />
        <CourseDrill />
      </main>
      <aside className={styles.side}>
        <RootPicker />
        <ScalePicker />
        <PatternControls />
        <VoiceControls />
        <MicPanel />
      </aside>
    </div>
  );
}
