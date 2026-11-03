// Drives KeshetProcessor like a host would and checks timing, MIDI, stopping and the audio itself.
// Usage: RenderTests [snapshot.png] [render.wav]
#include "../Source/PluginEditor.h"
#include "../Source/PluginProcessor.h"

#include <juce_audio_formats/juce_audio_formats.h>

static int failures = 0;
#define CHECK(cond, ...)                                    \
    do {                                                    \
        if (! (cond)) {                                     \
            ++failures;                                     \
            std::printf ("FAIL line %d: ", __LINE__);       \
            std::printf (__VA_ARGS__);                      \
            std::printf ("\n");                             \
        }                                                   \
    } while (0)

struct FakeHost : juce::AudioPlayHead
{
    bool playing = false;
    double bpm = 120, ppq = 0;
    bool giveTempo = true;

    juce::Optional<PositionInfo> getPosition() const override
    {
        PositionInfo info;
        info.setIsPlaying (playing);
        if (giveTempo) info.setBpm (bpm);
        info.setPpqPosition (ppq);
        info.setPpqPositionOfLastBarStart (std::floor (ppq / 4) * 4);
        info.setTimeSignature (juce::AudioPlayHead::TimeSignature { 4, 4 });
        return info;
    }
};

struct Rig
{
    static constexpr double SR = 48000;
    static constexpr int BLOCK = 512;

    KeshetProcessor proc;
    FakeHost host;
    juce::AudioBuffer<float> buffer { 2, BLOCK };
    long long sample = 0;
    std::vector<std::pair<long long, juce::MidiMessage>> midiOut;
    std::vector<float> audio; // left channel

    Rig()
    {
        proc.setPlayHead (&host);
        proc.prepareToPlay (SR, BLOCK);
    }

    void run (double seconds, std::vector<int> notesIn = {})
    {
        const long long blocks = (long long) std::ceil (seconds * SR / BLOCK);
        for (long long b = 0; b < blocks; ++b)
        {
            juce::MidiBuffer midi;
            if (b == 0)
                for (int n : notesIn) midi.addEvent (juce::MidiMessage::noteOn (1, n, (juce::uint8) 100), 0);
            proc.processBlock (buffer, midi);
            for (const auto meta : midi) midiOut.push_back ({ sample + meta.samplePosition, meta.getMessage() });
            const float* l = buffer.getReadPointer (0);
            audio.insert (audio.end(), l, l + BLOCK);
            sample += BLOCK;
            if (host.playing) host.ppq += BLOCK * host.bpm / 60.0 / SR;
        }
    }

    std::vector<std::pair<long long, int>> noteOns (long long from = 0) const
    {
        std::vector<std::pair<long long, int>> out;
        for (const auto& [at, m] : midiOut)
            if (m.isNoteOn() && at >= from) out.push_back ({ at, m.getNoteNumber() });
        return out;
    }

    float peak (long long from, long long to) const
    {
        float p = 0;
        for (long long i = std::max (0LL, from); i < std::min ((long long) audio.size(), to); ++i) p = std::max (p, std::abs (audio[(size_t) i]));
        return p;
    }

    int openNotes() const
    {
        int open = 0;
        for (const auto& [at, m] : midiOut)
            open += m.isNoteOn() ? 1 : m.isNoteOff() ? -1 : 0;
        return open;
    }
};

static void testHostSync()
{
    Rig rig; // defaults: D Dorian, up, one note per beat, female voice
    rig.host.playing = true;
    rig.run (8.0); // four bars at 120 BPM

    const auto ons = rig.noteOns();
    CHECK (ons.size() == 16, "host sync: %zu note-ons in 4 bars, want 16", ons.size());
    const int dorian[] = { 62, 64, 65, 67, 69, 71, 72, 74 };
    for (size_t i = 0; i < ons.size() && i < 16; ++i)
    {
        const long long want = (long long) i * 24000; // one beat at 120 BPM
        CHECK (std::llabs (ons[i].first - want) <= 1, "note %zu at sample %lld, want %lld", i, ons[i].first, want);
        CHECK (ons[i].second == dorian[i % 8], "note %zu is %d, want %d", i, ons[i].second, dorian[i % 8]);
    }
    float maxAbs = 0;
    bool finite = true;
    for (float x : rig.audio)
    {
        finite = finite && std::isfinite (x);
        maxAbs = std::max (maxAbs, std::abs (x));
    }
    CHECK (finite, "audio has NaN or inf");
    CHECK (maxAbs > 0.05f && maxAbs < 1.0f, "audio peak %f", maxAbs);
    std::printf ("host sync: 16 notes on the beat, peak %.3f\n", maxAbs);

    // Host stops mid-note: sound and MIDI must end.
    rig.run (0.25);
    rig.host.playing = false;
    const long long stopAt = rig.sample;
    rig.run (0.5);
    const float after = rig.peak (stopAt + (long long) (0.08 * Rig::SR), stopAt + (long long) (0.5 * Rig::SR));
    CHECK (after < 1e-4f, "still sounding after host stop: %f", after);
    CHECK (rig.openNotes() == 0, "%d MIDI notes left hanging after stop", rig.openNotes());
    CHECK (! rig.proc.running.load(), "still running after host stop");
    std::printf ("host stop: silent (%.6f), no hanging MIDI notes\n", after);
}

static void testJumpAndSubdivision()
{
    Rig rig;
    rig.proc.setParam (ids::sub, 3); // four per beat
    rig.proc.setParam (ids::pattern, 2); // arpeggios
    rig.proc.setParam (ids::direction, 2); // up and down
    rig.host.playing = true;
    rig.host.ppq = 7.3; // start mid-bar, between steps
    rig.run (1.0);
    const auto ons = rig.noteOns();
    // First step boundary after ppq 7.3 at 4 per beat is 7.5.
    CHECK (! ons.empty() && std::llabs (ons[0].first - (long long) (0.2 * 24000)) <= 1, "first sixteenth at %lld", ons.empty() ? -1 : ons[0].first);
    CHECK (ons.size() == 8, "sixteenths in 1 s at 120 BPM: %zu", ons.size());

    rig.host.ppq = 0; // the host loops back
    rig.run (1.0);
    CHECK (rig.openNotes() <= 1, "notes left open after a jump: %d", rig.openNotes());
    std::printf ("sixteenths: start mid-bar on the grid, survive a jump\n");
}

static void testInternalClock()
{
    Rig rig;
    rig.host.giveTempo = false; // standalone: the Tempo parameter (70) rules
    rig.proc.setParam (ids::loop, 0);
    rig.proc.togglePlay();
    const double beat = 60.0 / 70;
    rig.run (beat * 4 + beat * 8 + 2.0);
    const auto ons = rig.noteOns();
    CHECK (ons.size() == 8, "one pass, no loop: %zu notes", ons.size());
    const long long firstWant = (long long) std::llround (4 * beat * Rig::SR);
    CHECK (! ons.empty() && std::llabs (ons[0].first - firstWant) <= 512, "first note after the count-in at %lld, want %lld", ons.empty() ? -1 : ons[0].first, firstWant);
    // Count-in clicks sound before the first note.
    CHECK (rig.peak (0, (long long) (0.05 * Rig::SR)) > 0.01f, "no count-in click");
    CHECK (! rig.proc.running.load(), "should stop after one pass");
    CHECK (rig.openNotes() == 0, "hanging notes after one pass");
    std::printf ("own clock: count-in at 70 BPM, then 8 notes, then stops\n");

    // Stop button mid-play cuts the sound.
    Rig r2;
    r2.host.giveTempo = false;
    r2.proc.setParam (ids::countIn, 0);
    r2.proc.togglePlay();
    r2.run (1.0);
    r2.proc.togglePlay();
    const long long stopAt = r2.sample;
    r2.run (0.5);
    const float after = r2.peak (stopAt + (long long) (0.08 * Rig::SR), r2.sample);
    CHECK (after < 1e-4f, "sound after Stop: %f", after);
    std::printf ("stop button: silent after (%.6f)\n", after);
}

static void testMidiInSings()
{
    Rig rig;
    rig.run (1.0, { 63 });
    CHECK (rig.peak (0, rig.sample) > 0.02f, "MIDI in did not sound");
    CHECK (rig.proc.tapMidi.load() == 63, "tap not shown");
    const auto ons = rig.noteOns();
    CHECK (ons.size() == 1 && ons[0].second == 63, "MIDI in should pass through");
    std::printf ("MIDI in: sung and passed through\n");
}

static void testEveryVoiceAndSpeed (const juce::File& wav)
{
    // Every singer at the busiest setting, timed against real time.
    double rendered = 0, spent = 0;
    for (int singer = 0; singer < 4; ++singer)
    {
        Rig rig;
        rig.proc.setParam (ids::singer, (float) singer);
        rig.proc.setParam (ids::sub, 3);
        rig.proc.setParam (ids::scale, 23); // 8-note diminished
        rig.proc.setParam (ids::root, 1);
        rig.proc.setParam (ids::flats, 0); // C♯ with double sharps
        rig.host.bpm = 180;
        rig.host.playing = true;
        const auto t0 = juce::Time::getMillisecondCounterHiRes();
        rig.run (5.0);
        spent += (juce::Time::getMillisecondCounterHiRes() - t0) / 1000.0;
        rendered += 5.0;
        bool finite = true;
        for (float x : rig.audio) finite = finite && std::isfinite (x) && std::abs (x) < 1.0f;
        CHECK (finite, "singer %d: bad samples", singer);
        if (singer < 3) CHECK (rig.peak (0, rig.sample) > 0.02f, "singer %d silent", singer);
    }
    std::printf ("busiest setting, all 4 sounds: %.1fx faster than real time\n", rendered / spent);

    if (wav == juce::File()) return;
    // A listenable sample: D Dorian up and down, female voice with piano, 70 BPM.
    Rig rig;
    rig.proc.setParam (ids::direction, 2);
    rig.host.bpm = 70;
    rig.host.playing = true;
    rig.run (60.0 / 70 * 16 + 1);
    juce::WavAudioFormat format;
    wav.deleteFile();
    std::unique_ptr<juce::OutputStream> stream (wav.createOutputStream().release());
    if (auto writer = (format.createWriterFor (stream, juce::AudioFormatWriterOptions{}.withSampleRate (Rig::SR).withNumChannels (1).withBitsPerSample (16))))
    {
        const float* data = rig.audio.data();
        writer->writeFromFloatArrays (&data, 1, (int) rig.audio.size());
    }
}

static void snapshot (const juce::File& file)
{
    KeshetProcessor proc;
    FakeHost host;
    host.playing = true;
    proc.setPlayHead (&host);
    proc.prepareToPlay (48000, 512);
    proc.setParam (ids::root, 4);
    proc.setParam (ids::scale, 10); // Hijaz on E
    juce::AudioBuffer<float> buffer (2, 512);
    juce::MidiBuffer midi;
    for (int i = 0; i < 100; ++i)
    {
        proc.processBlock (buffer, midi);
        host.ppq += 512 * 2.0 / 48000;
    }
    std::unique_ptr<juce::AudioProcessorEditor> editor (proc.createEditor());
    const auto image = editor->createComponentSnapshot (editor->getLocalBounds(), true, 1.0f);
    file.deleteFile();
    juce::FileOutputStream out (file);
    juce::PNGImageFormat().writeImageToStream (image, out);
    std::printf ("snapshot: %s (%dx%d)\n", file.getFullPathName().toRawUTF8(), image.getWidth(), image.getHeight());
}

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;
    testHostSync();
    testJumpAndSubdivision();
    testInternalClock();
    testMidiInSings();
    testEveryVoiceAndSpeed (argc > 2 ? juce::File (argv[2]) : juce::File());
    if (argc > 1) snapshot (juce::File (argv[1]));
    if (failures)
    {
        std::printf ("%d failures\n", failures);
        return 1;
    }
    std::printf ("render: all passed\n");
    return 0;
}
