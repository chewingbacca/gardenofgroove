#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <atomic>
#include <vector>

namespace gg
{
struct DelayLine
{
    std::vector<float> buf;
    int w = 0;
    void init (int n) { buf.assign ((size_t) n, 0.0f); w = 0; }
    void push (float x) { buf[(size_t) w] = x; w = (w + 1) % (int) buf.size(); }
    float read (float d) const
    {
        const int n = (int) buf.size();
        float rp = (float) w - d;
        while (rp < 0.0f) rp += (float) n;
        int i0 = (int) rp;
        const float f = rp - (float) i0;
        i0 %= n;
        const int i1 = (i0 + 1) % n;
        return buf[(size_t) i0] * (1.0f - f) + buf[(size_t) i1] * f;
    }
};

// Octave-up pitch shifter (two crossfading read heads)
struct PitchUp
{
    DelayLine line;
    float ph = 0.0f;
    void init (int n) { line.init (n); ph = 0.0f; }
    float process (float x, float window)
    {
        line.push (x);
        ph += 1.0f / window;
        if (ph >= 1.0f) ph -= 1.0f;
        float p2 = ph + 0.5f;
        if (p2 >= 1.0f) p2 -= 1.0f;
        const float d1 = (1.0f - ph) * window + 1.0f;
        const float d2 = (1.0f - p2) * window + 1.0f;
        float g1 = std::sin (juce::MathConstants<float>::pi * ph);
        float g2 = std::sin (juce::MathConstants<float>::pi * p2);
        g1 *= g1; g2 *= g2;
        return line.read (d1) * g1 + line.read (d2) * g2;
    }
};
} // namespace gg

class GrooveGardenProcessor : public juce::AudioProcessor
{
public:
    GrooveGardenProcessor();
    ~GrooveGardenProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Groove Garden"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 4.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    juce::AudioProcessorValueTreeState apvts;
    std::atomic<int> currentStep { 0 };

private:
    float param (const char* id) const { return apvts.getRawParameterValue (id)->load(); }

    double sr = 44100.0;
    double freePos = 0.0;
    float gateGain = 1.0f, modSmooth = 0.0f;

    std::array<juce::dsp::StateVariableTPTFilter<float>, 2> svf;
    std::array<juce::dsp::IIR::Filter<float>, 2> airShelf;
    float lastAir = -1.0f;

    gg::DelayLine delay[2];
    float curDelay = 20000.0f;
    float fbLP[2] { 0.0f, 0.0f };
    gg::PitchUp pitch[2];

    juce::Reverb reverb;
    juce::AudioBuffer<float> fxBuf, sendBuf;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GrooveGardenProcessor)
};
