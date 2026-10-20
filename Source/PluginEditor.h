#pragma once
#include "PluginProcessor.h"
#include <map>
#include <memory>
#include <vector>
#include <functional>

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

// one pitch dot per step (scale degree 0..14), drawn under the level bars
class NoteStrip : public juce::Component, private juce::Timer
{
public:
    explicit NoteStrip (GrooveGardenProcessor& p) : proc (p)
    {
        for (int i = 0; i < 8; ++i)
            params[i] = proc.apvts.getParameter ("note" + juce::String (i + 1));
        startTimerHz (15);
    }
    ~NoteStrip() override { stopTimer(); }

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

    void addKnob (const juce::String& paramId, const juce::String& text);
    void addToggle (const juce::String& paramId, const juce::String& text);
    Knob& K (const juce::String& id) { return *knobs.at (id); }
    void setLevels (std::function<float (int)> fn);
    void setNotes (const std::vector<int>& degrees);
    void styleLabel (juce::Label& l, const juce::String& text);

    GrooveGardenProcessor& proc;
    juce::LookAndFeel_V4 laf;

    StepGrid grid;
    NoteStrip notes;
    juce::TextButton randomBtn { "Random" }, sineBtn { "Sin" }, triBtn { "Tri" }, rectBtn { "Rect" };
    juce::TextButton melodyBtn { "New Melody" };
    juce::ComboBox rateBox, shapeBox, rootBox, scaleBox;
    juce::Label rateLabel, shapeLabel, rootLabel, scaleLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> rateAtt, shapeAtt, rootAtt, scaleAtt;
    std::map<juce::String, std::unique_ptr<juce::TextButton>> toggles;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>> toggleAtts;

    std::map<juce::String, std::unique_ptr<Knob>> knobs;
    std::vector<std::pair<juce::String, juce::Rectangle<int>>> titles;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GrooveGardenEditor)
};
