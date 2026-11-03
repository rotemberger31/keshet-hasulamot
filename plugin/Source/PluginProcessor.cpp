#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace keshet;

namespace
{
constexpr double NOTE_LENGTH = 0.92; // of a step, so repeated notes still articulate
constexpr double TAP_LENGTH = 0.75;
constexpr int MIDI_CHANNEL = 1;
constexpr int VELOCITY = 100;
}

KeshetProcessor::KeshetProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      params (*this, nullptr, "KESHET", createLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout KeshetProcessor::createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout layout;
    auto choice = [&] (const char* id, const char* name, StringArray items, int def)
    { layout.add (std::make_unique<AudioParameterChoice> (ParameterID { id, 1 }, name, items, def)); };
    auto toggle = [&] (const char* id, const char* name, bool def)
    { layout.add (std::make_unique<AudioParameterBool> (ParameterID { id, 1 }, name, def)); };

    StringArray roots;
    for (int pc = 0; pc < 12; ++pc)
    {
        const auto s = spellRoot (pc, true);
        roots.add (String (LETTERS[s.letter]) + (s.acc < 0 ? "b" : s.acc > 0 ? "#" : ""));
    }
    StringArray scaleNames;
    for (const auto& s : scales()) scaleNames.add (s.en);

    choice (ids::root, "Root", roots, 2); // D, like the course exercise
    toggle (ids::flats, "Prefer flats", true);
    choice (ids::scale, "Scale", scaleNames, 1); // Dorian: D to D on the white keys
    choice (ids::direction, "Direction", { "Up", "Down", "Up & down" }, 0);
    choice (ids::pattern, "Pattern", { "Scale", "Thirds", "Arpeggio" }, 0);
    choice (ids::sub, "Notes per beat", { "1 (quarters)", "2 (eighths)", "3 (triplets)", "4 (sixteenths)" }, 0);
    choice (ids::singer, "Sound", { "Female voice", "Male voice", "Piano", "Silent (MIDI only)" }, 0);
    toggle (ids::pianoUnder, "Piano under voice", true);
    toggle (ids::click, "Metronome", true);
    toggle (ids::countIn, "Count-in", true);
    toggle (ids::loop, "Loop", true);
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { ids::clickVolume, 1 }, "Metronome volume", NormalisableRange<float> (0, 1), 0.7f));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { ids::noteVolume, 1 }, "Notes volume", NormalisableRange<float> (0, 1), 0.8f));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { ids::tempo, 1 }, "Tempo (no host)", NormalisableRange<float> (40, 200, 1), 70.0f));
    return layout;
}

SequenceOptions KeshetProcessor::currentOptions() const
{
    SequenceOptions o;
    o.rootPc = (int) getParam (ids::root);
    o.preferFlats = getParam (ids::flats) > 0.5f;
    o.scaleIndex = (int) getParam (ids::scale);
    o.direction = (Direction) (int) getParam (ids::direction);
    o.pattern = (Pattern) (int) getParam (ids::pattern);
    o.sub = (int) getParam (ids::sub) + 1;
    o.singer = (Singer) (int) getParam (ids::singer);
    return o;
}

void KeshetProcessor::setParam (const char* id, float plainValue)
{
    auto* p = params.getParameter (id);
    p->beginChangeGesture();
    p->setValueNotifyingHost (p->convertTo0to1 (plainValue));
    p->endChangeGesture();
}

void KeshetProcessor::prepareToPlay (double sr, int samplesPerBlock)
{
    sampleRate = sr;
    blockSize = samplesPerBlock;
    synth.prepare (sr);
    outBuffer.ensureSize (4096);
    clock = 0;
    numNoteOffs = 0;
    internalRunning = false;
    wasHostPlaying = false;
    haveSequence = false;
}

bool KeshetProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono();
}

void KeshetProcessor::tapNote (int midi)
{
    const auto scope = tapFifo.write (1);
    if (scope.blockSize1 > 0) tapQueue[(size_t) scope.startIndex1] = midi;
    else if (scope.blockSize2 > 0) tapQueue[(size_t) scope.startIndex2] = midi;
}

void KeshetProcessor::sound (double time, int midi, double duration, const Spelled& spelled, Singer singer)
{
    if (singer == Singer::female || singer == Singer::male)
    {
        synth.singNote (time, midi, duration, spelled, singer == Singer::male ? VoiceKind::male : VoiceKind::female);
        if (getParam (ids::pianoUnder) > 0.5f) synth.playPiano (time, midi, duration, 0.18);
    }
    else if (singer == Singer::piano)
    {
        synth.playPiano (time, midi, duration, 0.5);
    }
}

/** A key from the screen or from MIDI in: says the note as this scale spells it. */
void KeshetProcessor::tapNow (int midi, double time)
{
    const auto o = currentOptions();
    const Scale& scale = scales()[(size_t) o.scaleIndex];
    const int pc = mod (midi, 12);
    Spelled spelled = spellRoot (pc, o.preferFlats);
    for (int i = 0; i < scale.numDegrees; ++i)
        if (mod (o.rootPc + scale.degrees[(size_t) i].semi, 12) == pc)
            spelled = spellDegree (o.rootPc, o.preferFlats, scale.degrees[(size_t) i]);
    sound (time, midi, TAP_LENGTH, spelled, o.singer == Singer::silent ? Singer::piano : o.singer);
    tapMidi = midi;
    tapCount.fetch_add (1);
}

void KeshetProcessor::cut (double time)
{
    continuous = false;
    synth.kill (time);
    flushNoteOffs (true);
    currentStep = -1;
    countBeat = -1;
}

void KeshetProcessor::flushNoteOffs (bool all)
{
    int kept = 0;
    for (int i = 0; i < numNoteOffs; ++i)
    {
        const auto& off = noteOffs[(size_t) i];
        const long long offset = off.at - clock;
        if (all || offset < blockSize)
            midiOut->addEvent (juce::MidiMessage::noteOff (MIDI_CHANNEL, off.note), (int) juce::jlimit (0LL, (long long) blockSize - 1, all ? 0LL : offset));
        else
            noteOffs[(size_t) kept++] = off;
    }
    numNoteOffs = kept;
}

void KeshetProcessor::doStep (long long k, double time, int offset, double bpm, int beatInBar, bool internal)
{
    const int sub = lastOptions.sub;
    if (k % sub == 0 && getParam (ids::click) > 0.5f)
        synth.playClick (time, beatInBar == 0);

    if (k < 0)
    {
        countBeat = (int) mod ((int) (k / sub), 4);
        return;
    }
    countBeat = -1;

    if (internal && getParam (ids::loop) < 0.5f && k >= sequence.length)
    {
        // One pass only: let the last note ring out instead of cutting it.
        internalRunning = false;
        running = false;
        currentStep = -1;
        return;
    }

    const int index = (int) (k % sequence.length);
    currentStep = index;
    const SeqNote& note = sequence.notes[(size_t) index];
    if (note.rest) return;

    const double duration = 60.0 / bpm / sub * NOTE_LENGTH;
    sound (time, note.midi, duration, note.spelled, lastOptions.singer);

    midiOut->addEvent (juce::MidiMessage::noteOn (MIDI_CHANNEL, note.midi, (juce::uint8) VELOCITY), offset);
    if (numNoteOffs < (int) noteOffs.size())
        noteOffs[(size_t) numNoteOffs++] = { clock + offset + (long long) (duration * sampleRate), note.midi };
}

void KeshetProcessor::emitSteps (double ppqStart, double ppqEnd, double ppqPerSample, int numSamples, double bpm, double barStart, int beatsPerBar, bool internal)
{
    const int sub = lastOptions.sub;
    long long k = (long long) std::ceil (ppqStart * sub - 1e-6);
    // Block edges come from floating-point positions, so a step on the edge could be seen twice.
    if (continuous) k = std::max (k, nextStep);
    for (; (double) k / sub < ppqEnd - 1e-6; ++k)
    {
        const double ppq = (double) k / sub;
        const int offset = juce::jlimit (0, numSamples - 1, (int) std::lround ((ppq - ppqStart) / ppqPerSample));
        nextStep = k + 1;
        const double time = (double) (clock + offset) / sampleRate;
        const long long beat = (long long) std::floor (ppq - barStart + 1e-9);
        doStep (k, time, offset, bpm, (int) (((beat % beatsPerBar) + beatsPerBar) % beatsPerBar), internal);
        if (internal && ! internalRunning) break;
    }
}

void KeshetProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int numSamples = buffer.getNumSamples();
    blockSize = numSamples;
    buffer.clear();

    // Incoming MIDI passes through, and each note-on is sung by name.
    auto& out = outBuffer;
    out.clear();
    midiOut = &out;
    const double blockStart = (double) clock / sampleRate;
    for (const auto meta : midi)
    {
        out.addEvent (meta.getMessage(), meta.samplePosition);
        if (meta.getMessage().isNoteOn())
            tapNow (meta.getMessage().getNoteNumber(), blockStart + meta.samplePosition / sampleRate);
    }
    {
        const auto scope = tapFifo.read (tapFifo.getNumReady());
        for (int i = 0; i < scope.blockSize1; ++i) tapNow (tapQueue[(size_t) (scope.startIndex1 + i)], blockStart);
        for (int i = 0; i < scope.blockSize2; ++i) tapNow (tapQueue[(size_t) (scope.startIndex2 + i)], blockStart);
    }

    const auto options = currentOptions();
    if (! haveSequence || ! (options == lastOptions))
    {
        if (haveSequence && options.sub != lastOptions.sub) continuous = false;
        buildSequence (options, sequence);
        lastOptions = options;
        haveSequence = true;
    }
    synth.setVolumes (getParam (ids::clickVolume), getParam (ids::noteVolume));

    double bpm = getParam (ids::tempo);
    bool hostPlaying = false;
    std::optional<double> hostPpq;
    double barStart = 0;
    int beatsPerBar = 4;
    if (auto* head = getPlayHead())
    {
        if (const auto pos = head->getPosition())
        {
            if (const auto b = pos->getBpm(); b && *b > 0)
            {
                bpm = *b;
                hostHasTempo = true;
            }
            hostPlaying = pos->getIsPlaying();
            if (const auto ppq = pos->getPpqPosition()) hostPpq = *ppq;
            if (const auto bar = pos->getPpqPositionOfLastBarStart()) barStart = *bar;
            if (const auto sig = pos->getTimeSignature(); sig && sig->numerator > 0)
                beatsPerBar = sig->numerator;
        }
    }
    tempo = bpm;
    const double ppqPerSample = bpm / 60.0 / sampleRate;

    // The Play button: starts or stops the own clock. While the host plays, it follows the host instead.
    const int requests = playRequests.load();
    if (requests != handledPlayRequests)
    {
        const bool toggles = ((requests - handledPlayRequests) % 2) != 0;
        handledPlayRequests = requests;
        if (toggles && ! hostPlaying)
        {
            if (internalRunning)
            {
                internalRunning = false;
                cut (blockStart);
            }
            else
            {
                cut (blockStart);
                internalRunning = true;
                internalPpq = getParam (ids::countIn) > 0.5f ? -4.0 : 0.0;
            }
        }
    }

    if (hostPlaying && hostPpq)
    {
        if (internalRunning)
        {
            internalRunning = false;
            cut (blockStart);
        }
        const double ppqStart = *hostPpq;
        if (wasHostPlaying && std::abs (ppqStart - expectedPpq) > 0.01)
            cut (blockStart); // the host jumped or looped
        const double ppqEnd = ppqStart + numSamples * ppqPerSample;
        if (! wasHostPlaying) continuous = false;
        emitSteps (ppqStart, ppqEnd, ppqPerSample, numSamples, bpm, barStart, beatsPerBar, false);
        continuous = true;
        expectedPpq = ppqEnd;
    }
    else
    {
        if (wasHostPlaying) cut (blockStart);
        if (internalRunning)
        {
            const double ppqEnd = internalPpq + numSamples * ppqPerSample;
            emitSteps (internalPpq, ppqEnd, ppqPerSample, numSamples, bpm, 0, 4, true);
            continuous = true;
            internalPpq = ppqEnd;
        }
    }
    wasHostPlaying = hostPlaying && hostPpq.has_value();
    followingHost = wasHostPlaying;
    running = wasHostPlaying || internalRunning;
    if (! running && countBeat >= 0) countBeat = -1;

    flushNoteOffs (false);

    float* left = buffer.getWritePointer (0);
    float* right = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : nullptr;
    synth.render (left, right, numSamples, blockStart);

    midi.swapWith (out);
    midiOut = nullptr;
    clock += numSamples;
}

void KeshetProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = params.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void KeshetProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (params.state.getType()))
            params.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new KeshetProcessor(); }
