#pragma once

#include "PluginProcessor.h"

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

/** Everything on screen reads the parameters on each frame, so host automation and presets show up live. */
class KeshetEditor final : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit KeshetEditor (KeshetProcessor&);
    ~KeshetEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;

private:
    void timerCallback() override;

    struct Hit
    {
        juce::Rectangle<float> area;
        std::function<void()> action;
    };

    void drawHeader (juce::Graphics&, juce::Rectangle<float>);
    void drawNow (juce::Graphics&, juce::Rectangle<float>);
    void drawChips (juce::Graphics&, juce::Rectangle<float>);
    void drawRoots (juce::Graphics&, juce::Rectangle<float>);
    void drawScales (juce::Graphics&, juce::Rectangle<float>);
    void drawOptions (juce::Graphics&, juce::Rectangle<float>);
    void drawKeyboard (juce::Graphics&, juce::Rectangle<float>);
    void drawSeg (juce::Graphics&, juce::Rectangle<float>, const juce::String& title, const char* paramId, const juce::StringArray& labels);
    void drawToggle (juce::Graphics&, juce::Rectangle<float>, const juce::String& label, const char* paramId);
    void button (juce::Graphics&, juce::Rectangle<float>, const juce::String& text, bool selected, juce::Colour tint, std::function<void()> action, float fontSize = 15.0f);

    int keyAt (juce::Point<float>) const;

    KeshetProcessor& proc;
    keshet::Sequence sequence;
    keshet::SequenceOptions options;
    int shownCategory = -1;
    int lastTapCount = 0;
    double tapShownUntil = 0;

    std::vector<Hit> hits;
    juce::Rectangle<float> keyboardArea;
    int keyLow = 48, keyHigh = 84;

    juce::Slider clickVolume, noteVolume, tempo;
    juce::Label clickLabel, noteLabel, tempoLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> clickAttach, noteAttach, tempoAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KeshetEditor)
};
