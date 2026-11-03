#pragma once

// The sounds of src/audio (piano.ts, metronome.ts, voice/formant.ts), as fixed voice pools.
// Nothing here allocates after prepare(), so it runs on the audio thread.
// Times are absolute seconds on the plugin's own sample clock.

#include "../music/Music.h"
#include "Automation.h"

#include <array>
#include <cstdint>
#include <vector>

namespace keshet
{
enum class Bus { click, note };
enum class VoiceKind { female, male };

class Synth
{
public:
    void prepare (double sampleRate);
    void setVolumes (double click, double note);

    void playPiano (double time, int midi, double duration, double velocity);
    void playClick (double time, bool accent);
    /** Sings the full note name on one pitch: "re be mol" across the note. */
    void singNote (double time, int midi, double duration, const Spelled& spelled, VoiceKind voice);

    /** Cuts everything, including what is booked ahead of the clock. */
    void kill (double time);

    /** Adds to both channels (`right` may be null). `blockStart` is the time of the first sample. */
    void render (float* left, float* right, int numSamples, double blockStart);

    int activeVoices() const;

private:
    struct Base
    {
        bool active = false;
        double start = 0, end = 0;
        double killAt = 1e30;
        Bus bus = Bus::note;
    };

    struct Piano : Base
    {
        double phase[6] {}, inc[6] {}, amp[6] {};
        Biquad lowpass;
        Automation cutoff, gain;
    };

    struct Click : Base
    {
        double phase = 0, inc = 0;
        int harmonics = 1;
        Biquad highpass;
        Automation gain;
    };

    struct Noise : Base
    {
        uint32_t rng = 1;
        Biquad filter;
        Automation gain;
    };

    struct Sing : Base
    {
        const float* table = nullptr;
        double phase = 0, f0 = 0, outGain = 1;
        bool vibrato = false, nasal = false;
        Automation freq, depth, env, nasalGain;
        Automation bandFreq[4];
        double bandGain[4] {};
        Biquad bands[4], nasalFilter;
    };

    template <typename V, size_t N>
    V& allocate (std::array<V, N>& pool, double time);

    void noiseBurst (double time, double duration, Biquad::Type type, double freq, double q, double level);
    double shapeOnset (Sing& v, char onset, double t, double k);
    void singFormant (double time, int midi, double rawDuration, Syllable syllable, VoiceKind voice, bool vibrato);
    double compress (double x);

    double sr = 48000;
    double clickVolume = 0.63, noteVolume = 0.8;
    double compEnv = 0;
    uint32_t seed = 0x9e3779b9u;

    std::array<Piano, 24> pianos;
    std::array<Click, 8> clicks;
    std::array<Noise, 24> noises;
    std::array<Sing, 16> singers;

    static constexpr int TABLE_SIZE = 2048;
    static constexpr int MAX_HARMONICS = 59;
    /** glottal[h] holds the wave cut at h harmonics, so high notes do not alias. */
    std::vector<std::vector<float>> glottal;
};
} // namespace keshet
