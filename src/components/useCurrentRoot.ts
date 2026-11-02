import { getScale } from '../music/scales';
import { spellDegree } from '../music/spelling';
import { useStore } from '../state/store';

export function useScaleContext() {
  const rootPc = useStore((s) => s.rootPc);
  const preferFlats = useStore((s) => s.preferFlats);
  const scale = getScale(useStore((s) => s.scaleId));
  const notes = scale.degrees.map((degree) => spellDegree(rootPc, preferFlats, degree));
  return { rootPc, preferFlats, scale, notes, root: notes[0] };
}
