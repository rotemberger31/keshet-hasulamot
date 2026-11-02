#!/usr/bin/env node
// Turns raw takes into app-ready samples: trims silence, normalizes loudness,
// exports mp3 and rebuilds the manifest. Needs ffmpeg on the PATH.
//
//   npm run voices -- <raw-folder>
//
// Raw takes: <raw-folder>/{female|male}/{syllable}_{midi}.wav (see docs/recording-guide.md)

import { execFileSync } from 'node:child_process';
import { mkdirSync, readdirSync, writeFileSync } from 'node:fs';
import { join, parse, resolve } from 'node:path';

const VOICES = ['female', 'male'];
const SYLLABLES = ['do', 're', 'mi', 'fa', 'sol', 'la', 'si', 'di', 'ez', 'be', 'mol'];
const OUT = resolve('public/voices');
const FILTER = [
  'silenceremove=start_periods=1:start_threshold=-40dB:start_silence=0.01',
  'areverse',
  'silenceremove=start_periods=1:start_threshold=-40dB:start_silence=0.03',
  'areverse',
  'loudnorm=I=-16:TP=-1.5:LRA=7',
  'afade=t=in:d=0.005',
].join(',');

const raw = process.argv[2];
if (!raw) {
  console.error('usage: npm run voices -- <raw-folder>');
  process.exit(1);
}

const manifest = {};
const problems = [];

for (const voice of VOICES) {
  let files;
  try {
    files = readdirSync(join(raw, voice));
  } catch {
    continue;
  }
  mkdirSync(join(OUT, voice), { recursive: true });
  manifest[voice] = {};

  for (const file of files) {
    const { name, ext } = parse(file);
    const match = name.match(/^([a-z]+)_(\d+)$/);
    if (!/\.(wav|aiff?|flac|m4a|mp3)$/i.test(ext)) continue;
    if (!match || !SYLLABLES.includes(match[1])) {
      problems.push(`${voice}/${file}: expected {syllable}_{midi}`);
      continue;
    }
    const [, syllable, midi] = match;
    execFileSync('ffmpeg', ['-y', '-loglevel', 'error', '-i', join(raw, voice, file), '-af', FILTER, '-ar', '44100', '-ac', '1', '-b:a', '96k', join(OUT, voice, `${syllable}_${midi}.mp3`)]);
    (manifest[voice][syllable] ??= []).push(Number(midi));
    console.log(`${voice}/${syllable}_${midi}.mp3`);
  }

  for (const syllable of SYLLABLES) {
    if (!manifest[voice][syllable]) problems.push(`${voice}: no takes for "${syllable}" (this voice stays on synthesis)`);
    manifest[voice][syllable]?.sort((a, b) => a - b);
  }
}

writeFileSync(join(OUT, 'manifest.json'), `${JSON.stringify(manifest, null, 2)}\n`);
console.log(`\nWrote ${join(OUT, 'manifest.json')}`);
problems.forEach((problem) => console.warn(`! ${problem}`));
