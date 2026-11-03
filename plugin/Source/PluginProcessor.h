#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "dsp/Synth.h"
#include "music/Music.h"

#include <atomic>

namespace ids
{
inline constexpr auto root = "root", flats = "flats", scale = "scale", direction = "direction", pattern = "pattern",
                      sub = "sub", singer = "singer", pianoUnder = "pianoUnder", click = "click", countIn = "countIn",
                      loop = "loop", clickVolume = "clickVolume", noteVolume = "noteVolume", tempo = "tempo";
}

/**
 * Follows the host: while Ableton plays, step k of the walk lands on beat k/sub of the song,
 * so the sequence stays locked to the bars. While the host is stopped, the Play button runs
 * its own clock at the host's tempo, with a count-in, like the web app.
 */
class KeshetProcessor final : public juce::AudioProcessor
{
public:
    KeshetProcessor();

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 2.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState params;

    keshet::SequenceOptions currentOptions() const;
    void setParam (const char* id, float plainValue);
    float getParam (const char* id) const { return params.getRawParameterValue (id)->load(); }

    // Called from the editor.
    void togglePlay() { playRequests.fetch_add (1); }
    void tapNote (int midi);

    // Read by the editor.
    std::atomic<int> currentStep { -1 };  // index into the sequence, -1 when idle
    std::atomic<int> countBeat { -1 };    // count-in beat 0..3, -1 otherwise
    std::atomic<bool> running { false };
    std::atomic<bool> followingHost { false };
    std::atomic<bool> hostHasTempo { false };
    std::atomic<double> tempo { 70 };
    std::atomic<int> tapMidi { -1 };
    std::atomic<int> tapCount { 0 };

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    void emitSteps (double ppqStart, double ppqEnd, double ppqPerSample, int numSamples, double bpm, double barStart, int beatsPerBar, bool internal);
    void doStep (long long k, double time, int offset, double bpm, int beatInBar, bool internal);
    void sound (double time, int midi, double duration, const keshet::Spelled& spelled, keshet::Singer singer);
    void tapNow (int midi, double time);
    void cut (double time);
    void flushNoteOffs (bool all);

    keshet::Synth synth;
    keshet::Sequence sequence;
    keshet::SequenceOptions lastOptions;
    bool haveSequence = false;

    double sampleRate = 48000;
    long long clock = 0; // samples since prepareToPlay
    juce::MidiBuffer outBuffer;
    juce::MidiBuffer* midiOut = nullptr;
    int blockSize = 0;

    bool wasHostPlaying = false;
    double expectedPpq = 0;
    bool continuous = false;  // the next block follows on from the last one
    long long nextStep = 0;   // first step not yet played
    bool internalRunning = false;
    double internalPpq = 0;
    int handledPlayRequests = 0;
    std::atomic<int> playRequests { 0 };

    struct NoteOff { long long at; int note; };
    std::array<NoteOff, 64> noteOffs {};
    int numNoteOffs = 0;

    std::array<int, 64> tapQueue {};
    juce::AbstractFifo tapFifo { 64 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KeshetProcessor)
};
