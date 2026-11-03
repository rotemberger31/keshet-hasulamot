# Keshet: the plugin

The same scales, spelling and singing voice as the web app, as a VST3/AU instrument for Ableton Live (or any DAW).

## In Ableton

1. Preferences → Plug-Ins: turn on **Use VST3 Plug-in System Folders** (or **Use Audio Units**), then **Rescan**.
2. Drag **Keshet** (under Plug-Ins → VST3 → Rotem Berger) onto a MIDI track.
3. Press Play in Live. The walk locks to Live's tempo and bars: step *k* lands on beat *k/notes-per-beat*.
   With Live stopped, the plugin's own Play button runs it at Live's tempo, with a count-in.
4. Play your MIDI keyboard into the track: each note is sung by its name in the current scale.

**Driving another instrument:** pick **Silent** under "who sings", then on another MIDI track set
*MIDI From* → the Keshet track → *Keshet*, and Monitor → *In*.

All settings are plugin parameters, so they save with the set and can be automated.

## Build

Needs the Xcode command-line tools and CMake (`pip3 install --user cmake`).

```bash
git clone --depth 1 --branch 8.0.10 https://github.com/juce-framework/JUCE.git deps/JUCE
cmake -S . -B build -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build -j10
```

This builds a universal (Apple Silicon + Intel) VST3, AU and standalone app, and copies the plugins to
`~/Library/Audio/Plug-Ins`.

## Tests

```bash
./build/MusicTests
./build/RenderTests_artefacts/Release/RenderTests editor.png sample.wav
auval -v aumu Ksht Rtbg
```

- `MusicTests`: the spelling cases and the full matrix from `src/music/music.test.ts`.
- `RenderTests`: drives the processor with a fake host. It checks that notes land on the beat, that the walk starts mid-bar on the grid, and that it survives a loop jump. It also checks that stopping leaves no sound and no hanging MIDI, that the count-in and one-pass mode work, and that MIDI in is sung. It renders every sound at the busiest setting and saves a screenshot and a WAV.

## Layout

- `Source/music/`: a 1:1 port of `src/music`, with no heap use, so the audio thread can rebuild a sequence.
- `Source/dsp/`: the piano, click and formant voice from `src/audio`, plus a small AudioParam-style automation class.
- `Source/PluginProcessor.*`: host sync, its own clock, and MIDI in and out.
- `Source/PluginEditor.*`: the screen.
