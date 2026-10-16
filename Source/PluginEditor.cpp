#include "PluginEditor.h"

namespace
{
const juce::Colour accent  { 0xff7fe3d4 };
const juce::Colour accent2 { 0xffb48cff };
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
        g.setFont (12.0f);
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

// ------------------------------------------------------------ Editor
void GrooveGardenEditor::addKnob (Knob& k, const juce::String& text, const juce::String& paramId)
{
    k.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    k.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 64, 16);
    addAndMakeVisible (k.slider);
    k.label.setText (text, juce::dontSendNotification);
    k.label.setJustificationType (juce::Justification::centred);
    k.label.setColour (juce::Label::textColourId, juce::Colours::white.withAlpha (0.8f));
    addAndMakeVisible (k.label);
    k.att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, paramId, k.slider);
}

GrooveGardenEditor::GrooveGardenEditor (GrooveGardenProcessor& p)
    : AudioProcessorEditor (&p), proc (p), grid (p)
{
    laf.setColour (juce::Slider::rotarySliderFillColourId, accent);
    laf.setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colour (0x33ffffff));
    laf.setColour (juce::Slider::thumbColourId, juce::Colours::white);
    laf.setColour (juce::Slider::textBoxTextColourId, juce::Colours::white.withAlpha (0.85f));
    laf.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    laf.setColour (juce::TextButton::buttonColourId, juce::Colour (0x33ffffff));
    laf.setColour (juce::TextButton::textColourOffId, juce::Colours::white);
    laf.setColour (juce::ComboBox::backgroundColourId, juce::Colour (0x33ffffff));
    laf.setColour (juce::ComboBox::textColourId, juce::Colours::white);
    laf.setColour (juce::ComboBox::outlineColourId, juce::Colours::transparentBlack);
    laf.setColour (juce::ComboBox::arrowColourId, accent);
    setLookAndFeel (&laf);

    addAndMakeVisible (grid);

    auto fill = [this] (std::function<float (int)> f) { return [this, f] { setPattern (f); }; };
    randomBtn.onClick = [this]
    {
        auto& rng = juce::Random::getSystemRandom();
        setPattern ([&rng] (int) { return rng.nextFloat() < 0.18f ? 0.0f : 0.2f + 0.8f * rng.nextFloat(); });
    };
    const float pi = juce::MathConstants<float>::pi;
    sineBtn.onClick = fill ([pi] (int i) { return 0.5f + 0.5f * std::sin (2.0f * pi * (float) i / 8.0f); });
    triBtn.onClick  = fill ([] (int i) { return 1.0f - std::abs (2.0f * (float) i / 8.0f - 1.0f); });
    rectBtn.onClick = fill ([] (int i) { return i < 4 ? 1.0f : 0.2f; });
    for (auto* b : { &randomBtn, &sineBtn, &triBtn, &rectBtn }) addAndMakeVisible (*b);

    rateBox.addItemList ({ "1/4", "1/8", "1/16", "1/16T", "1/32" }, 1);
    shapeBox.addItemList ({ "Sine", "Triangle", "Rect" }, 1);
    addAndMakeVisible (rateBox);
    addAndMakeVisible (shapeBox);
    rateAtt  = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, "rate", rateBox);
    shapeAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, "shape", shapeBox);
    for (auto* l : { &rateLabel, &shapeLabel })
    {
        l->setColour (juce::Label::textColourId, juce::Colours::white.withAlpha (0.8f));
        addAndMakeVisible (*l);
    }
    rateLabel.setText ("Rate", juce::dontSendNotification);
    shapeLabel.setText ("Step shape", juce::dontSendNotification);

    addKnob (depth, "Depth", "depth");
    addKnob (filterAmt, "Filter Seq", "filterAmt");
    addKnob (cutoff, "Cutoff", "cutoff");
    addKnob (drive, "Drive", "drive");
    addKnob (air, "Air", "air");
    addKnob (width, "Width", "width");
    addKnob (mix, "Mix", "mix");
    addKnob (delayMix, "Delay", "delayMix");
    addKnob (delayFb, "Feedback", "delayFb");
    addKnob (shimmer, "Shimmer", "shimmer");
    addKnob (reverbMix, "Reverb", "reverbMix");
    addKnob (reverbSize, "Size", "reverbSize");
    addKnob (output, "Output", "output");

    knobs = { &depth, &filterAmt, &cutoff, &drive, &air, &width, &mix,
              &delayMix, &delayFb, &shimmer, &reverbMix, &reverbSize, &output };

    setSize (760, 560);
}

GrooveGardenEditor::~GrooveGardenEditor()
{
    setLookAndFeel (nullptr);
}

void GrooveGardenEditor::setPattern (std::function<float (int)> fn)
{
    for (int i = 0; i < 8; ++i)
        if (auto* prm = proc.apvts.getParameter ("step" + juce::String (i + 1)))
        {
            prm->beginChangeGesture();
            prm->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, fn (i)));
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
    g.drawText ("GROOVE GARDEN", 24, 14, 400, 34, juce::Justification::left);
    g.setColour (accent.withAlpha (0.8f));
    g.setFont (juce::FontOptions (13.0f));
    g.drawText ("8-step groove sequencer + hi-fi ear candy", 26, 46, 400, 18, juce::Justification::left);

    g.setColour (juce::Colours::white.withAlpha (0.45f));
    g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    g.drawText ("TONE", 24, 322, 100, 16, juce::Justification::left);
    g.drawText ("SPACE", 24, 438, 100, 16, juce::Justification::left);
}

void GrooveGardenEditor::resized()
{
    auto r = getLocalBounds().reduced (24);
    r.removeFromTop (46);

    grid.setBounds (r.removeFromTop (190));
    r.removeFromTop (10);

    auto bar = r.removeFromTop (30);
    for (auto* b : { &randomBtn, &sineBtn, &triBtn, &rectBtn })
    {
        b->setBounds (bar.removeFromLeft (74));
        bar.removeFromLeft (6);
    }
    auto right = bar.removeFromRight (330);
    rateLabel.setBounds (right.removeFromLeft (36));
    rateBox.setBounds (right.removeFromLeft (80));
    right.removeFromLeft (14);
    shapeLabel.setBounds (right.removeFromLeft (76));
    shapeBox.setBounds (right.removeFromLeft (104));

    r.removeFromTop (20);

    auto layoutRow = [] (juce::Rectangle<int> row, std::vector<Knob*> ks)
    {
        const int w = row.getWidth() / (int) ks.size();
        for (auto* k : ks)
        {
            auto c = row.removeFromLeft (w);
            k->label.setBounds (c.removeFromTop (16));
            k->slider.setBounds (c);
        }
    };
    layoutRow (r.removeFromTop (100), { &depth, &filterAmt, &cutoff, &drive, &air, &width, &mix });
    r.removeFromTop (16);
    r.removeFromTop (16);
    layoutRow (r.removeFromTop (100), { &delayMix, &delayFb, &shimmer, &reverbMix, &reverbSize, &output });
}
