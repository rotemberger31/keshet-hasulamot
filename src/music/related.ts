import { SCALES, type Scale } from './scales';

export interface RelatedScale {
  scale: Scale;
  from: string; // degree label dropped, "4"
  to: string; // degree label added, "♯4"
}

/** Same-size scales that differ from `scale` in exactly one note. */
export function relatedScales(scale: Scale): RelatedScale[] {
  const related: RelatedScale[] = [];
  for (const other of SCALES) {
    if (other.id === scale.id || other.pcs.length !== scale.pcs.length) continue;
    const added = other.pcs.filter((pc) => !scale.pcs.includes(pc));
    const removed = scale.pcs.filter((pc) => !other.pcs.includes(pc));
    if (added.length !== 1 || removed.length !== 1) continue;
    related.push({
      scale: other,
      from: scale.degrees[scale.pcs.indexOf(removed[0])].label,
      to: other.degrees[other.pcs.indexOf(added[0])].label,
    });
  }
  return related;
}

/** Step sizes in semitones, including the last step back to the octave. */
export function scaleSteps(scale: Scale): number[] {
  const semis = [...scale.degrees.map((d) => d.semi), 12];
  return semis.slice(1).map((semi, i) => semi - semis[i]);
}
