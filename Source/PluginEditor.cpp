#include "PluginEditor.h"

namespace
{
const juce::Colour accent  { 0xff7fe3d4 };
const juce::Colour accent2 { 0xffb48cff };
const juce::Colour accent3 { 0xffffb86b };
}

// ------------------------------------------------------------ StepGrid
void StepGrid::paint (juce::Graphics& g)
{
    auto area = getLocalBounds().toFloat();
    g.setColour (juce::Colour (0x22ffffff));
    g.fillRoundedRectangle (area, 8.0f);

    const float w = area.getWidth() / 8.0f;
    const int cur = proc.currentStep.load();

    for (int i = 0; i < 8; ++i)
    {
        auto cell = juce::Rectangle<float> (area.getX() + w * (float) i, area.getY(), w, area.getHeight()).reduced (5.0f);
        g.setColour (juce::Colour (0x18ffffff));
        g.fillRoundedRectangle (cell, 5.0f);

        const float v = params[i] != nullptr ? params[i]->getValue() : 0.0f;
        auto bar = cell.withTrimmedTop (cell.getHeight() * (1.0f - v));
        juce::ColourGradient grad (accent2, bar.getX(), bar.getY(), accent, bar.getX(), bar.getBottom(), false);
        g.setGradientFill (grad);
        g.fillRoundedRectangle (bar, 5.0f);

        if (i == cur)
        {
            g.setColour (juce::Colours::white.withAlpha (0.9f));
            g.drawRoundedRectangle (cell, 5.0f, 2.0f);
        }
        g.setColour (juce::Colours::white.withAlpha (0.5f));
        g.setFont (juce::FontOptions (12.0f));
        g.drawText (juce::String (i + 1), cell.removeFromBottom (16.0f).toNearestInt(), juce::Justification::centred);
    }
}

void StepGrid::setFromMouse (const juce::MouseEvent& e)
{
    const int idx = juce::jlimit (0, 7, (int) (e.position.x / (float) getWidth() * 8.0f));
    const float v = juce::jlimit (0.0f, 1.0f, 1.0f - (e.position.y - 5.0f) / ((float) getHeight() - 10.0f));
    if (idx != active)
    {
        if (active >= 0) params[active]->endChangeGesture();
        active = idx;
        params[active]->beginChangeGesture();
    }
    params[idx]->setValueNotifyingHost (v);
}

void StepGrid::mouseUp (const juce::MouseEvent&)
{
    if (active >= 0) params[active]->endChangeGesture();
    active = -1;
}

// ------------------------------------------------------------ NoteStrip
void NoteStrip::paint (juce::Graphics& g)
{
    auto area = getLocalBounds().toFloat();
    g.setColour (juce::Colour (0x16ffffff));
    g.fillRoundedRectangle (area, 8.0f);

    const float w = area.getWidth() / 8.0f;
    const int root  = (int) proc.apvts.getRawParameterValue ("root")->load();
    const int scale = (int) proc.apvts.getRawParameterValue ("scale")->load();
    const int cur = proc.currentStep.load();

    for (int i = 0; i < 8; ++i)
    {
        auto cell = juce::Rectangle<float> (area.getX() + w * (float) i, area.getY(), w, area.getHeight()).reduced (5.0f);
        const int deg = params[i] != nullptr ? (int) std::lround (params[i]->convertFrom0to1 (params[i]->getValue())) : 0;

        const float y = cell.getBottom() - 11.0f - (cell.getHeight() - 22.0f) * ((float) deg / 14.0f);
        g.setColour (juce::Colours::white.withAlpha (0.08f));
        g.drawHorizontalLine ((int) y, cell.getX() + 4.0f, cell.getRight() - 4.0f);

        auto pill = juce::Rectangle<float> (34.0f, 18.0f).withCentre ({ cell.getCentreX(), y });
        g.setColour (i == cur ? accent3 : accent3.withAlpha (0.75f));
        g.fillRoundedRectangle (pill, 9.0f);
        g.setColour (juce::Colour (0xff1a1230));
        g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
        g.drawText (gg::noteName (root + gg::scaleSemitone (scale, deg)), pill.toNearestInt(), juce::Justification::centred);
    }
}

void NoteStrip::setFromMouse (const juce::MouseEvent& e)
{
    const int idx = juce::jlimit (0, 7, (int) (e.position.x / (float) getWidth() * 8.0f));
    const float t = juce::jlimit (0.0f, 1.0f, 1.0f - (e.position.y - 16.0f) / ((float) getHeight() - 32.0f));
    const int deg = juce::jlimit (0, 14, (int) std::lround (t * 14.0f));
    if (idx != active)
    {
        if (active >= 0) params[active]->endChangeGesture();
        active = idx;
        params[active]->beginChangeGesture();
    }
    params[idx]->setValueNotifyingHost (params[idx]->convertTo0to1 ((float) deg));
}

void NoteStrip::mouseUp (const juce::MouseEvent&)
{
    if (active >= 0) params[active]->endChangeGesture();
    active = -1;
}

// ------------------------------------------------------------ Editor
void GrooveGardenEditor::styleLabel (juce::Label& l, const juce::String& text)
{
    l.setText (text, juce::dontSendNotification);
    l.setJustificationType (juce::Justification::centredRight);
    l.setColour (juce::Label::textColourId, juce::Colours::white.withAlpha (0.8f));
    addAndMakeVisible (l);
}

void GrooveGardenEditor::addKnob (const juce::String& paramId, const juce::String& text)
{
    auto k = std::make_unique<Knob>();
    k->slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    k->slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 64, 16);
    addAndMakeVisible (k->slider);
    k->label.setText (text, juce::dontSendNotification);
    k->label.setJustificationType (juce::Justification::centred);
    k->label.setColour (juce::Label::textColourId, juce::Colours::white.withAlpha (0.8f));
    addAndMakeVisible (k->label);
    k->att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, paramId, k->slider);
    knobs[paramId] = std::move (k);
}

void GrooveGardenEditor::addToggle (const juce::String& paramId, const juce::String& text)
{
    auto b = std::make_unique<juce::TextButton> (text);
    addAndMakeVisible (*b);
    // the attachment makes the button a toggle and keeps it in sync with the parameter
    toggleAtts.push_back (std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, paramId, *b));
    toggles[paramId] = std::move (b);
}

GrooveGardenEditor::GrooveGardenEditor (GrooveGardenProcessor& p)
    : AudioProcessorEditor (&p), proc (p), grid (p), notes (p)
{
    laf.setColour (juce::Slider::rotarySliderFillColourId, accent);
    laf.setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colour (0x33ffffff));
    laf.setColour (juce::Slider::thumbColourId, juce::Colours::white);
    laf.setColour (juce::Slider::textBoxTextColourId, juce::Colours::white.withAlpha (0.85f));
    laf.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    laf.setColour (juce::TextButton::buttonColourId, juce::Colour (0x33ffffff));
    laf.setColour (juce::TextButton::textColourOffId, juce::Colours::white.withAlpha (0.55f));
    laf.setColour (juce::TextButton::buttonOnColourId, accent.withAlpha (0.55f));
    laf.setColour (juce::TextButton::textColourOnId, juce::Colours::white);
    laf.setColour (juce::ComboBox::backgroundColourId, juce::Colour (0x33ffffff));
    laf.setColour (juce::ComboBox::textColourId, juce::Colours::white);
    laf.setColour (juce::ComboBox::outlineColourId, juce::Colours::transparentBlack);
    laf.setColour (juce::ComboBox::arrowColourId, accent);
    laf.setColour (juce::ToggleButton::textColourId, juce::Colours::white);
    laf.setColour (juce::ToggleButton::tickColourId, accent);
    laf.setColour (juce::ToggleButton::tickDisabledColourId, juce::Colour (0x66ffffff));
    laf.setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff1a1230));
    laf.setColour (juce::PopupMenu::textColourId, juce::Colours::white);
    setLookAndFeel (&laf);

    addAndMakeVisible (grid);
    addAndMakeVisible (notes);

    // pattern buttons
    randomBtn.onClick = [this]
    {
        auto& rng = juce::Random::getSystemRandom();
        setLevels ([&rng] (int) { return rng.nextFloat() < 0.18f ? 0.0f : 0.2f + 0.8f * rng.nextFloat(); });
    };
    const float pi = juce::MathConstants<float>::pi;
    sineBtn.onClick = [this, pi] { setLevels ([pi] (int i) { return 0.5f + 0.5f * std::sin (2.0f * pi * (float) i / 8.0f); }); };
    triBtn.onClick  = [this] { setLevels ([] (int i) { return 1.0f - std::abs (2.0f * (float) i / 8.0f - 1.0f); }); };
    rectBtn.onClick = [this] { setLevels ([] (int i) { return i < 4 ? 1.0f : 0.2f; }); };

    // "autolead": a random-walk melody through the chosen scale
    melodyBtn.onClick = [this]
    {
        auto& rng = juce::Random::getSystemRandom();
        static const int starts[4] = { 0, 2, 4, 7 };
        static const int moves[10] = { -3, -2, -1, -1, 0, 1, 1, 2, 3, 4 };
        std::vector<int> deg (8);
        int d = starts[rng.nextInt (4)];
        for (int i = 0; i < 8; ++i)
        {
            deg[(size_t) i] = d;
            d += moves[rng.nextInt (10)];
            if (d < 0) d = -d;
            if (d > 14) d = 28 - d;
            d = juce::jlimit (0, 14, d);
        }
        setNotes (deg);
    };
    for (auto* b : { &randomBtn, &sineBtn, &triBtn, &rectBtn, &melodyBtn }) addAndMakeVisible (*b);
    melodyBtn.setColour (juce::TextButton::buttonColourId, accent3.withAlpha (0.35f));

    // combos
    rateBox.addItemList ({ "1/4", "1/8", "1/16", "1/16T", "1/32" }, 1);
    shapeBox.addItemList ({ "Sine", "Triangle", "Rect" }, 1);
    rootBox.addItemList ({ "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" }, 1);
    scaleBox.addItemList ({ "Minor", "Major", "Dorian", "Phrygian", "Lydian", "Mixolydian", "Pent Minor", "Pent Major" }, 1);
    for (auto* c : { &rateBox, &shapeBox, &rootBox, &scaleBox }) addAndMakeVisible (*c);
    rateAtt  = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, "rate", rateBox);
    shapeAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, "shape", shapeBox);
    rootAtt  = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, "root", rootBox);
    scaleAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, "scale", scaleBox);
    styleLabel (rateLabel, "Rate");
    styleLabel (shapeLabel, "Step shape");
    styleLabel (rootLabel, "Root");
    styleLabel (scaleLabel, "Scale");

    // on/off switches for every defining feature
    addToggle ("seqOn", "Chop");        addToggle ("evolveOn", "Evolve");   addToggle ("motionOn", "Motion");
    addToggle ("granOn", "Grains");     addToggle ("freeze", "Freeze");
    addToggle ("pluckOn", "Melody");
    addToggle ("delayOn", "Delay");     addToggle ("shimmerOn", "Shimmer"); addToggle ("reverbOn", "Reverb");

    // sequence + tone
    addKnob ("depth", "Depth");       addKnob ("source", "Source");
    addKnob ("evolve", "Evolve");     addKnob ("filterAmt", "Filter Seq");
    addKnob ("cutoff", "Cutoff");     addKnob ("drive", "Drive");
    addKnob ("air", "Air");           addKnob ("motion", "Motion");
    // granulator
    addKnob ("gLevel", "Level");      addKnob ("gSize", "Size ms");
    addKnob ("gDensity", "Density");  addKnob ("gBurst", "Burst");
    addKnob ("gPos", "Position");     addKnob ("gSpray", "Spray");
    addKnob ("gPitch", "Pitch st");   addKnob ("gPitchSpray", "Pitch Spray");
    addKnob ("gScatter", "Scatter");  addKnob ("gReverse", "Reverse");
    // pluck
    addKnob ("pLevel", "Level");      addKnob ("pDecay", "Decay");
    addKnob ("pTone", "Tone");        addKnob ("pPick", "Pick");
    addKnob ("pOct", "Octave");
    // space + master
    addKnob ("delayMix", "Delay");    addKnob ("delayFb", "Feedback");
    addKnob ("shimmer", "Shimmer");   addKnob ("reverbMix", "Reverb");
    addKnob ("reverbSize", "Size");   addKnob ("width", "Width");
    addKnob ("mix", "Mix");           addKnob ("output", "Output");

    setSize (920, 860);
}

GrooveGardenEditor::~GrooveGardenEditor()
{
    setLookAndFeel (nullptr);
}

void GrooveGardenEditor::setLevels (std::function<float (int)> fn)
{
    for (int i = 0; i < 8; ++i)
        if (auto* prm = proc.apvts.getParameter ("step" + juce::String (i + 1)))
        {
            prm->beginChangeGesture();
            prm->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, fn (i)));
            prm->endChangeGesture();
        }
}

void GrooveGardenEditor::setNotes (const std::vector<int>& degrees)
{
    for (int i = 0; i < 8 && i < (int) degrees.size(); ++i)
        if (auto* prm = proc.apvts.getParameter ("note" + juce::String (i + 1)))
        {
            prm->beginChangeGesture();
            prm->setValueNotifyingHost (prm->convertTo0to1 ((float) juce::jlimit (0, 14, degrees[(size_t) i])));
            prm->endChangeGesture();
        }
}

void GrooveGardenEditor::paint (juce::Graphics& g)
{
    juce::ColourGradient bg (juce::Colour (0xff0b1020), 0, 0, juce::Colour (0xff261a45), (float) getWidth(), (float) getHeight(), false);
    g.setGradientFill (bg);
    g.fillAll();

    g.setColour (juce::Colours::white);
    g.setFont (juce::FontOptions (26.0f, juce::Font::bold));
    g.drawText ("GROOVE GARDEN", 24, 12, 400, 34, juce::Justification::left);
    g.setColour (accent.withAlpha (0.8f));
    g.setFont (juce::FontOptions (13.0f));
    g.drawText ("groove sequencer + granulator + autolead plucks", 26, 44, 500, 18, juce::Justification::left);

    for (auto& t : titles)
    {
        g.setColour (juce::Colours::white.withAlpha (0.5f));
        g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
        g.drawText (t.first, t.second, juce::Justification::centredLeft);
        g.setColour (juce::Colours::white.withAlpha (0.08f));
        g.drawHorizontalLine (t.second.getBottom() - 1, (float) t.second.getX() + 130.0f, (float) t.second.getRight());
    }
}

void GrooveGardenEditor::resized()
{
    titles.clear();
    auto r = getLocalBounds().reduced (24);
    r.removeFromTop (48);

    grid.setBounds (r.removeFromTop (150));
    r.removeFromTop (4);
    notes.setBounds (r.removeFromTop (64));
    r.removeFromTop (8);

    auto bar = r.removeFromTop (28);
    for (auto* b : { &randomBtn, &sineBtn, &triBtn, &rectBtn })
    {
        b->setBounds (bar.removeFromLeft (64));
        bar.removeFromLeft (6);
    }
    bar.removeFromLeft (6);
    melodyBtn.setBounds (bar.removeFromLeft (100));
    auto right = bar.removeFromRight (300);
    rateLabel.setBounds (right.removeFromLeft (36));
    right.removeFromLeft (4);
    rateBox.setBounds (right.removeFromLeft (72));
    right.removeFromLeft (10);
    shapeLabel.setBounds (right.removeFromLeft (70));
    right.removeFromLeft (4);
    shapeBox.setBounds (right.removeFromLeft (100));

    r.removeFromTop (8);

    juce::Rectangle<int> lastHead;
    auto section = [&] (const juce::String& title, std::vector<juce::String> ids, int perRow) -> juce::Rectangle<int>
    {
        lastHead = r.removeFromTop (20);
        titles.push_back ({ title, lastHead });
        auto row = r.removeFromTop (94);
        r.removeFromTop (4);
        const int w = row.getWidth() / perRow;
        for (auto& id : ids)
        {
            auto c = row.removeFromLeft (w);
            auto& k = K (id);
            k.label.setBounds (c.removeFromTop (16));
            k.slider.setBounds (c);
        }
        return row;
    };

    // puts the section's on/off buttons along the right of its header (last id = rightmost)
    auto placeToggles = [&] (std::vector<juce::String> ids)
    {
        auto h = lastHead;
        for (auto it = ids.rbegin(); it != ids.rend(); ++it)
        {
            toggles.at (*it)->setBounds (h.removeFromRight (74).reduced (2, 0));
        }
    };

    section ("SEQUENCE + TONE",
             { "depth", "source", "evolve", "filterAmt", "cutoff", "drive", "air", "motion" }, 8);
    placeToggles ({ "seqOn", "evolveOn", "motionOn" });

    section ("GRANULATOR",
             { "gLevel", "gSize", "gDensity", "gBurst", "gPos", "gSpray", "gPitch", "gPitchSpray", "gScatter", "gReverse" }, 10);
    placeToggles ({ "granOn", "freeze" });

    auto left = section ("PLUCK / AUTOLEAD", { "pLevel", "pDecay", "pTone", "pPick", "pOct" }, 10);
    placeToggles ({ "pluckOn" });
    auto combos = left.reduced (12, 30);
    rootLabel.setBounds (combos.removeFromLeft (44));
    combos.removeFromLeft (4);
    rootBox.setBounds (combos.removeFromLeft (70));
    combos.removeFromLeft (16);
    scaleLabel.setBounds (combos.removeFromLeft (48));
    combos.removeFromLeft (4);
    scaleBox.setBounds (combos.removeFromLeft (130));

    section ("SPACE + MASTER",
             { "delayMix", "delayFb", "shimmer", "reverbMix", "reverbSize", "width", "mix", "output" }, 8);
    placeToggles ({ "delayOn", "shimmerOn", "reverbOn" });
}
