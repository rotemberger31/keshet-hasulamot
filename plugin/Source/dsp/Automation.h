#pragma once

// Just enough of Web Audio's AudioParam to port src/audio as it is:
// setValueAtTime, linearRamp, exponentialRamp and setTarget, evaluated forward in time.

#include <algorithm>
#include <array>
#include <cmath>

namespace keshet
{
class Automation
{
public:
    void reset (double value)
    {
        count = 0;
        cursor = 0;
        segTime = 0;
        segValue = value;
        targeting = false;
    }

    void set (double value, double t) { add ({ Kind::set, t, value, 0 }); }
    void linear (double value, double t) { add ({ Kind::linear, t, value, 0 }); }
    void exponential (double value, double t) { add ({ Kind::exponential, t, value, 0 }); }
    void target (double value, double t, double tc) { add ({ Kind::target, t, value, tc }); }

    /** Time must not go backwards between calls. */
    double valueAt (double t)
    {
        while (cursor < count && events[(size_t) cursor].time <= t)
        {
            const Event& e = events[(size_t) cursor];
            const double before = holdValue (e.time);
            targeting = e.kind == Kind::target;
            if (targeting)
            {
                targetValue = e.value;
                targetTc = std::max (1e-6, e.tc);
                segValue = before;
            }
            else
            {
                segValue = e.value;
            }
            segTime = e.time;
            ++cursor;
        }

        if (cursor < count)
        {
            const Event& next = events[(size_t) cursor];
            if (next.kind == Kind::linear || next.kind == Kind::exponential)
            {
                const double span = next.time - segTime;
                const double from = holdValue (segTime);
                if (span <= 0) return next.value;
                const double x = (t - segTime) / span;
                if (next.kind == Kind::linear || from * next.value <= 0)
                    return from + (next.value - from) * x;
                return from * std::pow (next.value / from, x);
            }
        }
        return holdValue (t);
    }

private:
    enum class Kind { set, linear, exponential, target };
    struct Event { Kind kind; double time; double value; double tc; };

    double holdValue (double t) const
    {
        if (! targeting) return segValue;
        return targetValue + (segValue - targetValue) * std::exp (-(t - segTime) / targetTc);
    }

    /** Kept sorted by time, like Web Audio; equal times keep insertion order. */
    void add (Event e)
    {
        if (count == (int) events.size()) return;
        int i = count;
        while (i > cursor && events[(size_t) i - 1].time > e.time)
        {
            events[(size_t) i] = events[(size_t) i - 1];
            --i;
        }
        events[(size_t) i] = e;
        ++count;
    }

    std::array<Event, 12> events {};
    int count = 0, cursor = 0;
    double segTime = 0, segValue = 0;
    bool targeting = false;
    double targetValue = 0, targetTc = 1;
};

/** RBJ biquad, with Web Audio's conventions: lowpass/highpass Q in dB, bandpass with 0 dB peak. */
class Biquad
{
public:
    enum class Type { lowpass, highpass, bandpass };

    void setup (Type t, double sampleRate, double freq, double q)
    {
        type = t;
        sr = sampleRate;
        qValue = q;
        lastFreq = -1;
        setFrequency (freq);
    }

    void reset() { z1 = z2 = 0; }

    void setFrequency (double freq)
    {
        freq = std::clamp (freq, 10.0, sr * 0.49);
        if (std::abs (freq - lastFreq) < 0.01) return;
        lastFreq = freq;
        const double w = 2 * 3.14159265358979323846 * freq / sr;
        const double cw = std::cos (w), sw = std::sin (w);
        const double q = type == Type::bandpass ? std::max (1e-4, qValue) : std::pow (10.0, qValue / 20.0);
        const double alpha = sw / (2 * q);
        double b0, b1, b2;
        switch (type)
        {
            case Type::lowpass:  b0 = (1 - cw) / 2; b1 = 1 - cw; b2 = (1 - cw) / 2; break;
            case Type::highpass: b0 = (1 + cw) / 2; b1 = -(1 + cw); b2 = (1 + cw) / 2; break;
            default:             b0 = alpha; b1 = 0; b2 = -alpha; break;
        }
        const double a0 = 1 + alpha;
        nb0 = b0 / a0; nb1 = b1 / a0; nb2 = b2 / a0;
        na1 = -2 * cw / a0; na2 = (1 - alpha) / a0;
    }

    double process (double x)
    {
        const double y = nb0 * x + z1;
        z1 = nb1 * x - na1 * y + z2;
        z2 = nb2 * x - na2 * y;
        return y;
    }

private:
    Type type = Type::lowpass;
    double sr = 48000, qValue = 1, lastFreq = -1;
    double nb0 = 1, nb1 = 0, nb2 = 0, na1 = 0, na2 = 0, z1 = 0, z2 = 0;
};
} // namespace keshet
