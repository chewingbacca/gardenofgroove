#pragma once
#include "PluginProcessor.h"
#include <memory>

class StepGrid : public juce::Component, private juce::Timer
{
public:
    explicit StepGrid (GrooveGardenProcessor& p) : proc (p)
    {
        for (int i = 0; i < 8; ++i)
            params[i] = proc.apvts.getParameter ("step" + juce::String (i + 1));
        startTimerHz (30);
    }
    ~StepGrid() override { stopTimer(); }

    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent& e) override { setFromMouse (e); }
    void mouseDrag (const juce::MouseEvent& e) override { setFromMouse (e); }
    void mouseUp (const juce::MouseEvent&) override;

private:
    void timerCallback() override { repaint(); }
    void setFromMouse (const juce::MouseEvent& e);

    GrooveGardenProcessor& proc;
    juce::RangedAudioParameter* params[8] {};
    int active = -1;
};

class GrooveGardenEditor : public juce::AudioProcessorEditor
{
public:
    explicit GrooveGardenEditor (GrooveGardenProcessor&);
    ~GrooveGardenEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct Knob
    {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att;
    };

    void addKnob (Knob& k, const juce::String& text, const juce::String& paramId);
    void setPattern (std::function<float (int)> fn);

    GrooveGardenProcessor& proc;
    juce::LookAndFeel_V4 laf;

    StepGrid grid;
    juce::TextButton randomBtn { "Random" }, sineBtn { "Sin" }, triBtn { "Tri" }, rectBtn { "Rect" };
    juce::ComboBox rateBox, shapeBox;
    juce::Label rateLabel, shapeLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> rateAtt, shapeAtt;

    Knob depth, filterAmt, cutoff, drive, air, width, mix;
    Knob delayMix, delayFb, shimmer, reverbMix, reverbSize, output;
    std::array<Knob*, 13> knobs {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GrooveGardenEditor)
};
