#include "PluginEditor.h"

using namespace keshet;
using juce::Colour;
using juce::Rectangle;
using juce::String;

namespace
{
constexpr int WIDTH = 980, HEIGHT = 720, PAD = 18;

// The web app's dark theme (src/styles/tokens.css).
const Colour BG { 0xff141120 }, CARD { 0xff201a33 }, CARD_HI { 0xff2c2545 }, INK { 0xfff0ebfa }, MUTED { 0xffa79fbd };

Colour noteColour (int pc, double lightness = 0.79)
{
    uint8_t r, g, b;
    oklchToRgb (lightness, 0.15, noteHue (pc), r, g, b);
    return Colour (r, g, b);
}

String utf8 (const char* s) { return String::fromUTF8 (s); }
String heName (const Spelled& s) { return utf8 (HEBREW[s.letter]) + utf8 (accSymbol (s.acc)); }
String enName (const Spelled& s) { return String (LETTERS[s.letter]) + utf8 (accSymbol (s.acc)); }

juce::Font font (float size, bool bold = false)
{
    return juce::Font (juce::FontOptions (size, bold ? juce::Font::bold : juce::Font::plain));
}

bool isBlack (int midi)
{
    const int pc = mod (midi, 12);
    return pc == 1 || pc == 3 || pc == 6 || pc == 8 || pc == 10;
}
} // namespace

KeshetEditor::KeshetEditor (KeshetProcessor& p) : AudioProcessorEditor (p), proc (p)
{
    setSize (WIDTH, HEIGHT);

    auto setupSlider = [this] (juce::Slider& s, juce::Label& l, const String& text, const char* id, std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>& attach)
    {
        s.setSliderStyle (juce::Slider::LinearHorizontal);
        s.setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
        s.setColour (juce::Slider::trackColourId, INK.withAlpha (0.8f));
        s.setColour (juce::Slider::backgroundColourId, CARD_HI);
        s.setColour (juce::Slider::thumbColourId, INK);
        addAndMakeVisible (s);
        l.setText (text, juce::dontSendNotification);
        l.setFont (font (14));
        l.setColour (juce::Label::textColourId, MUTED);
        l.setJustificationType (juce::Justification::centredRight);
        addAndMakeVisible (l);
        attach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.params, id, s);
    };
    setupSlider (clickVolume, clickLabel, utf8 ("עוצמת מטרונום"), ids::clickVolume, clickAttach);
    setupSlider (noteVolume, noteLabel, utf8 ("עוצמת צלילים"), ids::noteVolume, noteAttach);
    setupSlider (tempo, tempoLabel, "BPM", ids::tempo, tempoAttach);
    tempo.setTextBoxStyle (juce::Slider::TextBoxLeft, false, 44, 22);
    tempo.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    tempo.setColour (juce::Slider::textBoxTextColourId, INK);

    timerCallback();
    startTimerHz (30);
}

KeshetEditor::~KeshetEditor() { stopTimer(); }

void KeshetEditor::timerCallback()
{
    const auto next = proc.currentOptions();
    if (! (next == options) || sequence.length == 0)
    {
        options = next;
        buildSequence (options, sequence);
        int low = 127, high = 0;
        for (int i = 0; i < sequence.length; ++i)
            if (! sequence.notes[(size_t) i].rest)
            {
                low = std::min (low, sequence.notes[(size_t) i].midi);
                high = std::max (high, sequence.notes[(size_t) i].midi);
            }
        keyLow = low - mod (low, 12);
        keyHigh = std::max (keyLow + 24, high + (12 - mod (high, 12)) % 12);
    }
    if (shownCategory < 0) shownCategory = (int) scales()[(size_t) options.scaleIndex].cat;

    const int taps = proc.tapCount.load();
    if (taps != lastTapCount)
    {
        lastTapCount = taps;
        tapShownUntil = juce::Time::getMillisecondCounterHiRes() + 900;
    }

    const bool showTempo = ! proc.hostHasTempo.load();
    tempo.setVisible (showTempo);
    tempoLabel.setVisible (showTempo);
    repaint();
}

void KeshetEditor::resized()
{
    // Matches the options block in paint(): volumes on the left of the last options row.
    const int rowY = 488;
    clickLabel.setBounds (PAD + 170, rowY, 110, 24);
    clickVolume.setBounds (PAD, rowY, 170, 24);
    noteLabel.setBounds (PAD + 170, rowY + 28, 110, 24);
    noteVolume.setBounds (PAD, rowY + 28, 170, 24);
    tempo.setBounds (PAD + 66, 26, 200, 24);
    tempoLabel.setBounds (PAD + 270, 26, 40, 24);
}

void KeshetEditor::button (juce::Graphics& g, Rectangle<float> r, const String& text, bool selected, Colour tint, std::function<void()> action, float fontSize)
{
    const auto fill = selected ? tint : CARD_HI;
    g.setColour (fill);
    g.fillRoundedRectangle (r, 9.0f);
    if (selected)
    {
        g.setColour (INK.withAlpha (0.9f));
        g.drawRoundedRectangle (r.reduced (1), 9.0f, 2.0f);
    }
    g.setColour (selected && tint != INK ? Colour (0xff1a1528) : (selected ? BG : INK));
    g.setFont (font (fontSize, selected));
    g.drawFittedText (text, r.toNearestInt().reduced (4, 0), juce::Justification::centred, 2, 0.8f);
    hits.push_back ({ r, std::move (action) });
}

void KeshetEditor::paint (juce::Graphics& g)
{
    hits.clear();
    g.fillAll (BG);
    auto area = getLocalBounds().toFloat().reduced (PAD, 12);

    drawHeader (g, area.removeFromTop (52));
    area.removeFromTop (8);

    auto top = area.removeFromTop (118);
    drawNow (g, top.removeFromRight (250));
    top.removeFromRight (12);
    drawChips (g, top);
    area.removeFromTop (12);

    drawRoots (g, area.removeFromTop (64));
    area.removeFromTop (10);
    drawScales (g, area.removeFromTop (132));
    area.removeFromTop (10);
    drawOptions (g, area.removeFromTop (128));
    area.removeFromTop (10);
    drawKeyboard (g, area.removeFromTop (112));
    area.removeFromTop (8);

    g.setColour (MUTED);
    g.setFont (font (15));
    g.drawFittedText (utf8 (scales()[(size_t) options.scaleIndex].desc), area.toNearestInt(), juce::Justification::topRight, 2, 1.0f);
}

void KeshetEditor::drawHeader (juce::Graphics& g, Rectangle<float> r)
{
    g.setColour (INK);
    g.setFont (font (30, true));
    g.drawText (utf8 ("קשת הסולמות"), r.removeFromRight (260), juce::Justification::centredRight);

    // Rainbow underline, one stripe per pitch class.
    auto stripe = Rectangle<float> (r.getRight() + 260 - 190, r.getBottom() - 6, 190, 5);
    for (int pc = 0; pc < 12; ++pc)
    {
        g.setColour (noteColour (pc));
        g.fillRect (stripe.removeFromRight (190.0f / 12));
    }

    const bool running = proc.running.load();
    auto play = r.removeFromLeft (52).withSizeKeepingCentre (48, 48);
    g.setColour (running ? noteColour (0) : noteColour (4));
    g.fillEllipse (play);
    g.setColour (BG);
    if (running)
    {
        g.fillRoundedRectangle (play.withSizeKeepingCentre (16, 16), 2);
    }
    else
    {
        juce::Path tri;
        const auto c = play.getCentre();
        tri.addTriangle (c.x - 6, c.y - 10, c.x - 6, c.y + 10, c.x + 11, c.y); // points right: forward in time
        g.fillPath (tri);
    }
    hits.push_back ({ play, [this] { proc.togglePlay(); } });

    r.removeFromLeft (14);
    String status;
    if (proc.followingHost.load())
        status = utf8 ("מסונכרן לתוכנה · ") + String (proc.tempo.load(), std::fmod (proc.tempo.load(), 1.0) == 0 ? 0 : 1) + " BPM";
    else if (running)
        status = utf8 ("מנגן · ") + String (proc.tempo.load(), 0) + " BPM";
    else
        status = utf8 ("לחצו נגן, או נגנו באבלטון");
    if (! proc.hostHasTempo.load()) r.removeFromLeft (330);
    g.setColour (MUTED);
    g.setFont (font (15));
    g.drawText (status, r, juce::Justification::centredLeft);
}

void KeshetEditor::drawNow (juce::Graphics& g, Rectangle<float> r)
{
    const Scale& scale = scales()[(size_t) options.scaleIndex];
    const int step = proc.currentStep.load();
    const int count = proc.countBeat.load();
    const bool tapping = juce::Time::getMillisecondCounterHiRes() < tapShownUntil && ! proc.running.load();

    const SeqNote* note = step >= 0 && step < sequence.length && ! sequence.notes[(size_t) step].rest ? &sequence.notes[(size_t) step] : nullptr;
    Spelled spelled;
    String degree;
    bool hasNote = false;
    if (note)
    {
        spelled = note->spelled;
        degree = utf8 (scale.degrees[(size_t) note->degreeIndex].label);
        hasNote = true;
    }
    else if (tapping)
    {
        const int pc = mod (proc.tapMidi.load(), 12);
        spelled = spellRoot (pc, options.preferFlats);
        for (int i = 0; i < scale.numDegrees; ++i)
            if (mod (options.rootPc + scale.degrees[(size_t) i].semi, 12) == pc)
            {
                spelled = spellDegree (options.rootPc, options.preferFlats, scale.degrees[(size_t) i]);
                degree = utf8 (scale.degrees[(size_t) i].label);
            }
        hasNote = true;
    }

    g.setColour (hasNote ? noteColour (spelled.pc) : CARD);
    g.fillRoundedRectangle (r, 16);
    const auto ink = hasNote ? Colour (0xff1a1528) : INK;

    if (count >= 0)
    {
        g.setColour (INK);
        g.setFont (font (64, true));
        g.drawText (String (count + 1), r, juce::Justification::centred);
        return;
    }
    if (! hasNote)
    {
        const auto root = spellRoot (options.rootPc, options.preferFlats);
        g.setColour (INK);
        g.setFont (font (24, true));
        g.drawFittedText (heName (root) + " " + utf8 (scale.he), r.reduced (12).removeFromTop (60).toNearestInt(), juce::Justification::centred, 2);
        g.setColour (MUTED);
        g.setFont (font (16));
        g.drawText (enName (root) + " " + scale.en, r.reduced (12).removeFromBottom (36), juce::Justification::centred);
        return;
    }
    g.setColour (ink);
    g.setFont (font (52, true));
    g.drawText (heName (spelled), r.withTrimmedBottom (34), juce::Justification::centred);
    g.setFont (font (18));
    g.drawText (enName (spelled) + (degree.isNotEmpty() ? utf8 ("   ·   ") + degree : String()), r.removeFromBottom (40), juce::Justification::centred);
}

void KeshetEditor::drawChips (juce::Graphics& g, Rectangle<float> r)
{
    const Scale& scale = scales()[(size_t) options.scaleIndex];
    const int step = proc.currentStep.load();
    const int currentDegree = step >= 0 && step < sequence.length && ! sequence.notes[(size_t) step].rest ? sequence.notes[(size_t) step].degreeIndex : -1;

    g.setColour (CARD);
    g.fillRoundedRectangle (r, 16);
    r = r.reduced (12);
    const float gap = 8;
    const float w = (r.getWidth() - gap * (scale.numDegrees - 1)) / scale.numDegrees;
    for (int i = 0; i < scale.numDegrees; ++i)
    {
        const auto& d = scale.degrees[(size_t) i];
        const auto s = spellDegree (options.rootPc, options.preferFlats, d);
        // Right to left: the root sits on the right, like Hebrew text.
        auto chip = Rectangle<float> (r.getRight() - (i + 1) * w - i * gap, r.getY(), w, r.getHeight());
        const bool on = i == currentDegree;
        if (on) chip = chip.expanded (2, 4);
        g.setColour (noteColour (s.pc, on ? 0.86 : 0.72));
        g.fillRoundedRectangle (chip, 12);
        if (on)
        {
            g.setColour (INK);
            g.drawRoundedRectangle (chip, 12, 3);
        }
        g.setColour (Colour (0xff1a1528));
        g.setFont (font (w > 70 ? 26.0f : 21.0f, true));
        g.drawText (heName (s), chip.withTrimmedBottom (chip.getHeight() * 0.42f), juce::Justification::centredBottom);
        g.setFont (font (15));
        g.drawText (enName (s), chip.withTrimmedTop (chip.getHeight() * 0.56f).withTrimmedBottom (chip.getHeight() * 0.2f), juce::Justification::centred);
        g.setFont (font (13, true));
        g.setColour (Colour (0xff1a1528).withAlpha (0.65f));
        g.drawText (utf8 (d.label), chip.removeFromBottom (chip.getHeight() * 0.24f), juce::Justification::centred);
    }
}

void KeshetEditor::drawRoots (juce::Graphics& g, Rectangle<float> r)
{
    g.setColour (MUTED);
    g.setFont (font (15, true));
    g.drawText (utf8 ("צליל יסוד"), r.removeFromTop (22), juce::Justification::centredRight);
    r.removeFromTop (4);

    // ♭/♯ spelling on the far left.
    auto flats = r.removeFromLeft (96);
    const bool preferFlats = options.preferFlats;
    button (g, flats.removeFromLeft (46), utf8 ("♯"), ! preferFlats, INK, [this] { proc.setParam (ids::flats, 0); }, 20);
    flats.removeFromLeft (4);
    button (g, flats.removeFromLeft (46), utf8 ("♭"), preferFlats, INK, [this] { proc.setParam (ids::flats, 1); }, 20);
    r.removeFromLeft (16);

    const float gap = 6;
    const float w = (r.getWidth() - gap * 11) / 12;
    for (int pc = 0; pc < 12; ++pc)
    {
        const auto b = Rectangle<float> (r.getRight() - (pc + 1) * w - pc * gap, r.getY(), w, r.getHeight());
        const auto s = spellRoot (pc, preferFlats);
        button (g, b, heName (s), pc == options.rootPc, noteColour (pc), [this, pc] { proc.setParam (ids::root, (float) pc); }, 17);
        // A colour stripe even when not selected, so the wheel is always visible.
        if (pc != options.rootPc)
        {
            g.setColour (noteColour (pc));
            g.fillRoundedRectangle (b.withTop (b.getBottom() - 5).reduced (8, 0), 2);
        }
    }
}

void KeshetEditor::drawScales (juce::Graphics& g, Rectangle<float> r)
{
    auto tabs = r.removeFromTop (34);
    const float gap = 6;
    const float tw = (tabs.getWidth() - gap * (NUM_CATEGORIES - 1)) / NUM_CATEGORIES;
    for (int c = 0; c < NUM_CATEGORIES; ++c)
    {
        const auto b = Rectangle<float> (tabs.getRight() - (c + 1) * tw - c * gap, tabs.getY(), tw, tabs.getHeight());
        const bool here = c == (int) scales()[(size_t) options.scaleIndex].cat;
        button (g, b, utf8 (CATEGORY_HE[c]) + (here && c != shownCategory ? utf8 (" •") : String()), c == shownCategory, INK, [this, c] { shownCategory = c; }, 15);
    }
    r.removeFromTop (8);

    std::vector<int> inCategory;
    for (int i = 0; i < NUM_SCALES; ++i)
        if ((int) scales()[(size_t) i].cat == shownCategory) inCategory.push_back (i);

    const int perRow = 4;
    const float rowH = 40;
    const float w = (r.getWidth() - gap * (perRow - 1)) / perRow;
    for (size_t n = 0; n < inCategory.size(); ++n)
    {
        const int i = inCategory[n];
        const int col = (int) n % perRow, row = (int) n / perRow;
        const auto b = Rectangle<float> (r.getRight() - (col + 1) * w - col * gap, r.getY() + row * (rowH + 6), w, rowH);
        button (g, b, utf8 (scales()[(size_t) i].he), i == options.scaleIndex, noteColour (options.rootPc), [this, i] { proc.setParam (ids::scale, (float) i); }, 15);
    }
}

void KeshetEditor::drawSeg (juce::Graphics& g, Rectangle<float> r, const String& title, const char* paramId, const juce::StringArray& labels)
{
    g.setColour (MUTED);
    g.setFont (font (14, true));
    g.drawText (title, r.removeFromTop (20), juce::Justification::centredRight);
    r.removeFromTop (3);
    const int value = (int) proc.getParam (paramId);
    const float gap = 4;
    const float w = (r.getWidth() - gap * (labels.size() - 1)) / labels.size();
    for (int i = 0; i < labels.size(); ++i)
    {
        const auto b = Rectangle<float> (r.getRight() - (i + 1) * w - i * gap, r.getY(), w, r.getHeight());
        button (g, b, labels[i], i == value, INK, [this, paramId, i] { proc.setParam (paramId, (float) i); }, 14);
    }
}

void KeshetEditor::drawToggle (juce::Graphics& g, Rectangle<float> r, const String& label, const char* paramId)
{
    const bool on = proc.getParam (paramId) > 0.5f;
    button (g, r, (on ? utf8 ("✓ ") : String()) + label, on, INK, [this, paramId, on] { proc.setParam (paramId, on ? 0.0f : 1.0f); }, 14);
}

void KeshetEditor::drawOptions (juce::Graphics& g, Rectangle<float> r)
{
    auto row1 = r.removeFromTop (58);
    const float gap = 14;
    const float w = (row1.getWidth() - gap * 3) / 4;
    drawSeg (g, row1.removeFromRight (w), utf8 ("כיוון"), ids::direction, { utf8 ("עולה"), utf8 ("יורד"), utf8 ("עולה ויורד") });
    row1.removeFromRight (gap);
    drawSeg (g, row1.removeFromRight (w), utf8 ("תבנית"), ids::pattern, { utf8 ("סולם"), utf8 ("טרצות"), utf8 ("ארפג'ים") });
    row1.removeFromRight (gap);
    drawSeg (g, row1.removeFromRight (w), utf8 ("צלילים בפעמה"), ids::sub, { "1", "2", "3", "4" });
    row1.removeFromRight (gap);
    drawSeg (g, row1, utf8 ("מי שר"), ids::singer, { utf8 ("זמרת"), utf8 ("זמר"), utf8 ("פסנתר"), utf8 ("שקט") });

    r.removeFromTop (12);
    auto row2 = r.removeFromTop (52).withTrimmedLeft (300); // volume sliders sit on the left (see resized)
    const float tw = (row2.getWidth() - 8 * 3) / 4;
    const std::pair<const char*, const char*> toggles[] = {
        { "פסנתר מתחת לקול", ids::pianoUnder }, { "מטרונום", ids::click }, { "ספירה לפני", ids::countIn }, { "לופ", ids::loop },
    };
    for (const auto& [label, id] : toggles)
    {
        drawToggle (g, row2.removeFromRight (tw).withSizeKeepingCentre (tw, 38), utf8 (label), id);
        row2.removeFromRight (8);
    }
}

void KeshetEditor::drawKeyboard (juce::Graphics& g, Rectangle<float> r)
{
    keyboardArea = r;
    g.setColour (CARD);
    g.fillRoundedRectangle (r, 14);
    r = r.reduced (10);

    const Scale& scale = scales()[(size_t) options.scaleIndex];
    auto inScale = [&] (int midi)
    {
        for (int i = 0; i < scale.numDegrees; ++i)
            if (mod (options.rootPc + scale.degrees[(size_t) i].semi, 12) == mod (midi, 12)) return true;
        return false;
    };
    const int step = proc.currentStep.load();
    int current = step >= 0 && step < sequence.length && ! sequence.notes[(size_t) step].rest ? sequence.notes[(size_t) step].midi : -1;
    if (current < 0 && juce::Time::getMillisecondCounterHiRes() < tapShownUntil) current = proc.tapMidi.load();

    int whites = 0;
    for (int m = keyLow; m <= keyHigh; ++m) whites += isBlack (m) ? 0 : 1;
    const float ww = r.getWidth() / whites;

    // Low notes on the left, as on a real keyboard.
    float x = r.getX();
    for (int m = keyLow; m <= keyHigh; ++m)
    {
        if (isBlack (m)) continue;
        const auto key = Rectangle<float> (x, r.getY(), ww, r.getHeight()).reduced (1.5f, 0);
        g.setColour (m == current ? noteColour (mod (m, 12), 0.86) : inScale (m) ? noteColour (mod (m, 12), 0.9).interpolatedWith (juce::Colours::white, 0.55f) : Colour (0xffe9e5f2));
        g.fillRoundedRectangle (key, 5);
        if (inScale (m))
        {
            g.setColour (noteColour (mod (m, 12)));
            g.fillEllipse (key.getCentreX() - 6, key.getBottom() - 18, 12, 12);
        }
        if (m == current)
        {
            g.setColour (INK);
            g.drawRoundedRectangle (key, 5, 3);
        }
        x += ww;
    }
    x = r.getX();
    for (int m = keyLow; m <= keyHigh; ++m)
    {
        if (! isBlack (m))
        {
            x += ww;
            continue;
        }
        const auto key = Rectangle<float> (x - ww * 0.32f, r.getY(), ww * 0.64f, r.getHeight() * 0.6f);
        g.setColour (m == current ? noteColour (mod (m, 12), 0.86) : inScale (m) ? noteColour (mod (m, 12), 0.55) : Colour (0xff2a2438));
        g.fillRoundedRectangle (key, 4);
        if (m == current)
        {
            g.setColour (INK);
            g.drawRoundedRectangle (key, 4, 2.5f);
        }
    }
}

int KeshetEditor::keyAt (juce::Point<float> p) const
{
    const auto r = keyboardArea.reduced (10);
    if (! r.contains (p)) return -1;
    int whites = 0;
    for (int m = keyLow; m <= keyHigh; ++m) whites += isBlack (m) ? 0 : 1;
    const float ww = r.getWidth() / whites;
    if (p.y < r.getY() + r.getHeight() * 0.6f)
    {
        float x = r.getX();
        for (int m = keyLow; m <= keyHigh; ++m)
        {
            if (! isBlack (m)) { x += ww; continue; }
            if (std::abs (p.x - x) < ww * 0.32f) return m;
        }
    }
    float x = r.getX();
    for (int m = keyLow; m <= keyHigh; ++m)
    {
        if (isBlack (m)) continue;
        if (p.x >= x && p.x < x + ww) return m;
        x += ww;
    }
    return -1;
}

void KeshetEditor::mouseDown (const juce::MouseEvent& e)
{
    const auto p = e.position;
    if (const int key = keyAt (p); key >= 0)
    {
        proc.tapNote (key);
        return;
    }
    for (const auto& hit : hits)
        if (hit.area.contains (p))
        {
            hit.action();
            repaint();
            return;
        }
}

juce::AudioProcessorEditor* KeshetProcessor::createEditor() { return new KeshetEditor (*this); }
