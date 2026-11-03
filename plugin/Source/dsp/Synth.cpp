#include "Synth.h"

#include <algorithm>
#include <cmath>

namespace keshet
{
namespace
{
constexpr double TWO_PI = 6.283185307179586;
constexpr int FILTER_UPDATE = 16; // samples between filter coefficient updates

using Formants = std::array<double, 4>;
struct FormantTable { Formants a, e, i, o, l, m, r, d, b; };

const FormantTable MALE {
    { 730, 1090, 2440, 3400 }, { 440, 1900, 2550, 3350 }, { 290, 2250, 2950, 3500 }, { 470, 820, 2600, 3300 },
    { 350, 1150, 2600, 3300 }, { 260, 1000, 2250, 3300 }, { 420, 1400, 2450, 3300 }, { 300, 1700, 2600, 3300 },
    { 280, 900, 2300, 3300 },
};
const FormantTable FEMALE {
    { 860, 1230, 2820, 3900 }, { 480, 2280, 2900, 3950 }, { 330, 2700, 3250, 4100 }, { 530, 930, 2800, 3800 },
    { 400, 1300, 2900, 3800 }, { 300, 1150, 2550, 3800 }, { 480, 1600, 2800, 3800 }, { 340, 1950, 2900, 3800 },
    { 320, 1050, 2600, 3800 },
};
const double FORMANT_Q[4] = { 7, 11, 15, 17 };

const Formants* lookup (const FormantTable& t, char c)
{
    switch (c)
    {
        case 'a': return &t.a;
        case 'e': return &t.e;
        case 'i': return &t.i;
        case 'o': return &t.o;
        case 'l': return &t.l;
        case 'm': return &t.m;
        case 'r': return &t.r;
        case 'd': return &t.d;
        case 'b': return &t.b;
        default:  return nullptr;
    }
}

const Formants& formantGain (char vowel)
{
    static const Formants A { 1, 0.6, 0.35, 0.15 }, E { 1, 0.75, 0.45, 0.2 }, I { 1, 0.8, 0.5, 0.22 }, O { 1, 0.55, 0.25, 0.1 };
    switch (vowel)
    {
        case 'a': return A;
        case 'e': return E;
        case 'i': return I;
        default:  return O;
    }
}

struct Shape { char onset; char vowel; char coda; };

Shape shapeOf (Syllable s)
{
    switch (s)
    {
        case Syllable::do_: return { 'd', 'o', 0 };
        case Syllable::re:  return { 'r', 'e', 0 };
        case Syllable::mi:  return { 'm', 'i', 0 };
        case Syllable::fa:  return { 'f', 'a', 0 };
        case Syllable::sol: return { 's', 'o', 'l' };
        case Syllable::la:  return { 'l', 'a', 0 };
        case Syllable::si:  return { 's', 'i', 0 };
        case Syllable::di:  return { 'd', 'i', 0 };
        case Syllable::ez:  return { 0, 'e', 'z' };
        case Syllable::be:  return { 'b', 'e', 0 };
        case Syllable::mol: return { 'm', 'o', 'l' };
    }
    return { 0, 'a', 0 };
}

const double PIANO_PARTIALS[6][2] = { { 1, 1 }, { 2, 0.42 }, { 3, 0.22 }, { 4, 0.12 }, { 5, 0.07 }, { 6, 0.04 } };
constexpr double MAX_NOTE_S = 1.4;

inline double killFade (double t, double killAt)
{
    return t <= killAt ? 1.0 : std::exp (-(t - killAt) / 0.004);
}
} // namespace

void Synth::prepare (double sampleRate)
{
    sr = sampleRate;
    for (auto& v : pianos) v.active = false;
    for (auto& v : clicks) v.active = false;
    for (auto& v : noises) v.active = false;
    for (auto& v : singers) v.active = false;
    compEnv = 0;

    // Web Audio's PeriodicWave normalizes the full wave; every cut-down table shares that scale.
    glottal.assign (MAX_HARMONICS + 1, std::vector<float> (TABLE_SIZE, 0.0f));
    std::vector<double> sum (TABLE_SIZE, 0.0);
    for (int h = 1; h <= MAX_HARMONICS; ++h)
    {
        const double a = 1.0 / std::pow (h, 1.2);
        for (int i = 0; i < TABLE_SIZE; ++i)
            sum[(size_t) i] += a * std::sin (TWO_PI * h * i / TABLE_SIZE);
        for (int i = 0; i < TABLE_SIZE; ++i)
            glottal[(size_t) h][(size_t) i] = (float) sum[(size_t) i];
    }
    float peak = 0;
    for (float x : glottal[MAX_HARMONICS]) peak = std::max (peak, std::abs (x));
    for (auto& table : glottal)
        for (auto& x : table) x /= peak;
}

void Synth::setVolumes (double click, double note)
{
    clickVolume = click * 0.9;
    noteVolume = note;
}

template <typename V, size_t N>
V& Synth::allocate (std::array<V, N>& pool, double time)
{
    for (auto& v : pool)
        if (! v.active) return v;
    // Steal the one that started first.
    V* oldest = &pool[0];
    for (auto& v : pool)
        if (v.start < oldest->start) oldest = &v;
    (void) time;
    return *oldest;
}

void Synth::playPiano (double time, int midi, double duration, double velocity)
{
    auto& v = allocate (pianos, time);
    const double freq = midiToFreq (midi);
    const double held = std::max (duration, 0.25);
    v = Piano {};
    v.active = true;
    v.bus = Bus::note;
    v.start = time;
    v.end = time + held + 0.7;
    for (int i = 0; i < 6; ++i)
    {
        v.inc[i] = freq * PIANO_PARTIALS[i][0] * (1 + 0.0007 * i) / sr; // slight inharmonicity
        v.amp[i] = PIANO_PARTIALS[i][1] * 0.3;
    }
    v.lowpass.setup (Biquad::Type::lowpass, sr, std::min (9000.0, freq * 9), 1);
    v.cutoff.reset (std::min (9000.0, freq * 9));
    v.cutoff.set (std::min (9000.0, freq * 9), time);
    v.cutoff.exponential (std::max (freq * 2.5, 500.0), time + 1.4);
    v.gain.reset (0);
    v.gain.set (0, time);
    v.gain.linear (velocity, time + 0.004);
    v.gain.exponential (velocity * 0.35, time + 0.3);
    v.gain.target (0.0001, time + held, 0.18);
}

void Synth::playClick (double time, bool accent)
{
    auto& v = allocate (clicks, time);
    v = Click {};
    v.active = true;
    v.bus = Bus::click;
    v.start = time;
    v.end = time + 0.06;
    const double freq = accent ? 2000 : 1350;
    v.inc = freq / sr;
    v.harmonics = std::max (1, (int) (sr * 0.45 / freq));
    v.highpass.setup (Biquad::Type::highpass, sr, 600, 1);
    v.gain.reset (0);
    v.gain.set (0, time);
    v.gain.linear (accent ? 0.35 : 0.22, time + 0.002);
    v.gain.exponential (0.001, time + 0.05);
}

void Synth::noiseBurst (double time, double duration, Biquad::Type type, double freq, double q, double level)
{
    auto& v = allocate (noises, time);
    v = Noise {};
    v.active = true;
    v.bus = Bus::note;
    v.start = time;
    v.end = time + duration + 0.02;
    seed = seed * 1664525u + 1013904223u;
    v.rng = seed | 1u;
    v.filter.setup (type, sr, freq, q);
    v.gain.reset (0);
    v.gain.set (0, time);
    v.gain.linear (level, time + duration * 0.3);
    v.gain.linear (0, time + duration);
}

/** Shapes the amplitude for the onset. Returns when the vowel formants are reached. */
double Synth::shapeOnset (Sing& v, char onset, double t, double k)
{
    const double glide = 0.05 * k;
    auto& env = v.env;
    switch (onset)
    {
        case 'd':
        case 'b':
            env.set (0, t + 0.025 * k);
            env.linear (1, t + 0.05 * k);
            noiseBurst (t + 0.02 * k, 0.018, Biquad::Type::bandpass, onset == 'd' ? 3200 : 1200, 1, onset == 'd' ? 0.12 : 0.07);
            return t + 0.05 * k + glide;
        case 'r':
            env.linear (0.7, t + 0.015);
            env.linear (0.12, t + 0.035 * k + 0.01);
            env.linear (1, t + 0.06 * k + 0.015);
            return t + 0.06 * k + glide;
        case 'm':
            env.linear (0.45, t + 0.015);
            env.set (0.45, t + 0.07 * k);
            env.linear (1, t + 0.1 * k);
            return t + 0.08 * k + glide;
        case 'l':
            env.linear (0.6, t + 0.02);
            env.linear (1, t + 0.08 * k);
            return t + 0.06 * k + glide;
        case 'f':
            env.set (0, t + 0.07 * k);
            env.linear (1, t + 0.1 * k);
            noiseBurst (t, 0.09 * k, Biquad::Type::highpass, 1600, 0.7, 0.09);
            return t + 0.08 * k + glide * 0.6;
        case 's':
            env.set (0, t + 0.09 * k);
            env.linear (1, t + 0.12 * k);
            noiseBurst (t, 0.11 * k, Biquad::Type::bandpass, 6200, 1.6, 0.22);
            return t + 0.1 * k + glide * 0.6;
        default:
            env.linear (1, t + 0.025);
            return t + 0.01;
    }
}

void Synth::singFormant (double time, int midi, double rawDuration, Syllable syllable, VoiceKind voice, bool vibrato)
{
    const Shape shape = shapeOf (syllable);
    const FormantTable& table = voice == VoiceKind::male ? MALE : FEMALE;
    const double duration = std::max (0.06, rawDuration);
    const double k = std::min (1.0, duration / 0.34); // squeezes consonants on short notes
    const double f0 = midiToFreq (midi);

    auto& v = allocate (singers, time);
    v = Sing {};
    v.active = true;
    v.bus = Bus::note;
    v.start = time;
    const double end = time + duration;
    v.end = end + 0.05;
    v.f0 = f0;
    const int harmonics = std::clamp ((int) (sr * 0.45 / (f0 * 1.03)), 1, MAX_HARMONICS);
    v.table = glottal[(size_t) harmonics].data();

    v.freq.reset (f0 * 0.99);
    v.freq.set (f0 * 0.99, time);
    v.freq.linear (f0, time + 0.04 * k + 0.01);
    v.vibrato = vibrato && duration > 0.35;
    v.depth.reset (0);
    v.depth.set (0, time + 0.18);
    v.depth.linear (f0 * 0.011, time + 0.4);

    v.env.reset (0);
    v.env.set (0, time);
    v.outGain = voice == VoiceKind::male ? 1.1 : 0.85;

    const Formants& vowelFormants = *lookup (table, shape.vowel);
    const Formants* onsetFormants = lookup (table, shape.onset);
    const Formants& startFormants = onsetFormants ? *onsetFormants : vowelFormants;
    const double vowelAt = shapeOnset (v, shape.onset, time, k);

    const double release = std::min (0.07, duration * 0.25);
    const double codaAt = end - 0.1 * k - release;
    if (shape.coda == 'l')
    {
        v.env.set (1, codaAt);
        v.env.linear (0.55, end - release);
    }
    else if (shape.coda == 'z')
    {
        v.env.set (1, codaAt);
        v.env.linear (0.35, end - release);
        noiseBurst (codaAt, 0.1 * k + release, Biquad::Type::bandpass, 5200, 1.4, 0.16);
    }
    else
    {
        v.env.set (1, end - release);
    }
    v.env.linear (0, end);

    const Formants& gains = formantGain (shape.vowel);
    for (int j = 0; j < 4; ++j)
    {
        v.bands[j].setup (Biquad::Type::bandpass, sr, startFormants[(size_t) j], FORMANT_Q[j]);
        auto& f = v.bandFreq[j];
        f.reset (startFormants[(size_t) j]);
        f.set (startFormants[(size_t) j], time);
        f.linear (vowelFormants[(size_t) j], std::max (vowelAt, time + 0.005));
        if (shape.coda == 'l')
        {
            f.set (vowelFormants[(size_t) j], codaAt);
            f.linear (table.l[(size_t) j], end - release);
        }
        v.bandGain[j] = gains[(size_t) j];
    }

    v.nasal = shape.onset == 'm';
    if (v.nasal)
    {
        v.nasalFilter.setup (Biquad::Type::lowpass, sr, 380, 1);
        v.nasalGain.reset (0.5);
        v.nasalGain.set (0.5, time);
        v.nasalGain.linear (0, time + 0.1 * k);
    }
}

void Synth::singNote (double time, int midi, double duration, const Spelled& spelled, VoiceKind voice)
{
    const double total = std::min (duration, MAX_NOTE_S);
    TimedSyllable parts[5];
    const int count = noteSyllables (spelled, parts);
    for (int i = 0; i < count; ++i)
        singFormant (time + parts[i].start * total, midi, parts[i].length * total, parts[i].syllable, voice, i == count - 1);
}

void Synth::kill (double time)
{
    auto cut = [time] (auto& pool)
    {
        for (auto& v : pool)
        {
            if (! v.active) continue;
            if (v.start >= time) v.active = false;
            v.killAt = std::min (v.killAt, time);
            v.end = std::min (v.end, time + 0.06);
        }
    };
    cut (pianos);
    cut (clicks);
    cut (noises);
    cut (singers);
}

int Synth::activeVoices() const
{
    int n = 0;
    auto count = [&n] (const auto& pool) { for (const auto& v : pool) n += v.active ? 1 : 0; };
    count (pianos);
    count (clicks);
    count (noises);
    count (singers);
    return n;
}

double Synth::compress (double x)
{
    // DynamicsCompressor with threshold −14 dB, ratio 4, a 30 dB knee, 3 ms attack and 250 ms release.
    const double level = std::abs (x);
    const double coeff = level > compEnv ? std::exp (-1.0 / (0.003 * sr)) : std::exp (-1.0 / (0.25 * sr));
    compEnv = level + coeff * (compEnv - level);
    const double db = 20 * std::log10 (std::max (compEnv, 1e-9));
    constexpr double threshold = -14, ratio = 4, knee = 30;
    double over = db - threshold;
    double reduction;
    if (over <= -knee / 2) reduction = 0;
    else if (over >= knee / 2) reduction = over * (1 - 1 / ratio);
    else reduction = (1 - 1 / ratio) * (over + knee / 2) * (over + knee / 2) / (2 * knee);
    return x * std::pow (10.0, -reduction / 20);
}

void Synth::render (float* left, float* right, int numSamples, double blockStart)
{
    for (int s = 0; s < numSamples; ++s)
    {
        const double t = blockStart + s / sr;
        const bool update = (s % FILTER_UPDATE) == 0;
        double clickBus = 0, noteBus = 0;
        auto out = [&] (const Base& v, double x) { (v.bus == Bus::click ? clickBus : noteBus) += x * killFade (t, v.killAt); };

        for (auto& v : pianos)
        {
            if (! v.active || t < v.start) continue;
            if (t >= v.end) { v.active = false; continue; }
            double x = 0;
            for (int i = 0; i < 6; ++i)
            {
                x += std::sin (TWO_PI * v.phase[i]) * v.amp[i];
                v.phase[i] += v.inc[i];
                v.phase[i] -= std::floor (v.phase[i]);
            }
            if (update) v.lowpass.setFrequency (v.cutoff.valueAt (t));
            out (v, v.lowpass.process (x) * v.gain.valueAt (t));
        }

        for (auto& v : clicks)
        {
            if (! v.active || t < v.start) continue;
            if (t >= v.end) { v.active = false; continue; }
            double x = 0;
            for (int h = 1; h <= v.harmonics; h += 2)
                x += std::sin (TWO_PI * v.phase * h) / h;
            x *= 4 / 3.14159265358979323846;
            v.phase += v.inc;
            v.phase -= std::floor (v.phase);
            out (v, v.highpass.process (x) * v.gain.valueAt (t));
        }

        for (auto& v : noises)
        {
            if (! v.active || t < v.start) continue;
            if (t >= v.end) { v.active = false; continue; }
            v.rng ^= v.rng << 13;
            v.rng ^= v.rng >> 17;
            v.rng ^= v.rng << 5;
            const double x = (v.rng / 4294967295.0) * 2 - 1;
            out (v, v.filter.process (x) * v.gain.valueAt (t));
        }

        for (auto& v : singers)
        {
            if (! v.active || t < v.start) continue;
            if (t >= v.end) { v.active = false; continue; }
            double f = v.freq.valueAt (t);
            if (v.vibrato) f += std::sin (TWO_PI * 5.4 * (t - v.start)) * v.depth.valueAt (t);
            const double pos = v.phase * TABLE_SIZE;
            const int i0 = (int) pos;
            const double frac = pos - i0;
            const double wave = v.table[i0] + (v.table[(i0 + 1) % TABLE_SIZE] - v.table[i0]) * frac;
            v.phase += f / sr;
            v.phase -= std::floor (v.phase);

            const double e = wave * v.env.valueAt (t);
            double y = 0;
            for (int j = 0; j < 4; ++j)
            {
                if (update) v.bands[j].setFrequency (v.bandFreq[j].valueAt (t));
                y += v.bands[j].process (e) * v.bandGain[j];
            }
            if (v.nasal) y += v.nasalFilter.process (e) * v.nasalGain.valueAt (t);
            out (v, y * v.outGain);
        }

        const double mixed = compress ((clickBus * clickVolume + noteBus * noteVolume) * 0.9);
        left[s] += (float) mixed;
        if (right) right[s] += (float) mixed;
    }
}
} // namespace keshet
