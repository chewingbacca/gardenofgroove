#include "PluginProcessor.h"
#include "PluginEditor.h"

using Param = juce::AudioParameterFloat;
using juce::ParameterID;
using juce::NormalisableRange;

juce::AudioProcessorValueTreeState::ParameterLayout GrooveGardenProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout l;

    const float defaults[8] = { 1.0f, 0.45f, 0.8f, 0.3f, 1.0f, 0.55f, 0.7f, 0.35f };
    for (int i = 0; i < 8; ++i)
        l.add (std::make_unique<Param> (ParameterID { "step" + juce::String (i + 1), 1 },
                                        "Step " + juce::String (i + 1),
                                        NormalisableRange<float> (0.0f, 1.0f), defaults[i]));

    l.add (std::make_unique<juce::AudioParameterChoice> (ParameterID { "rate", 1 }, "Rate",
            juce::StringArray { "1/4", "1/8", "1/16", "1/16T", "1/32" }, 2));
    l.add (std::make_unique<juce::AudioParameterChoice> (ParameterID { "shape", 1 }, "Shape",
            juce::StringArray { "Sine", "Triangle", "Rect" }, 0));

    auto add = [&] (const char* id, const char* name, float lo, float hi, float def, float skew = 1.0f)
    {
        NormalisableRange<float> r (lo, hi);
        r.setSkewForCentre (skew > 0 && skew != 1.0f ? skew : (lo + hi) * 0.5f);
        l.add (std::make_unique<Param> (ParameterID { id, 1 }, name, r, def));
    };
    auto addLin = [&] (const char* id, const char* name, float lo, float hi, float def)
    {
        l.add (std::make_unique<Param> (ParameterID { id, 1 }, name, NormalisableRange<float> (lo, hi), def));
    };

    addLin ("depth",     "Depth",        0.0f, 1.0f, 0.9f);
    addLin ("filterAmt", "Filter Seq",   0.0f, 1.0f, 0.4f);
    add    ("cutoff",    "Cutoff",       200.0f, 18000.0f, 9000.0f, 2000.0f);
    addLin ("drive",     "Drive",        0.0f, 1.0f, 0.25f);
    addLin ("air",       "Air",          0.0f, 1.0f, 0.5f);
    addLin ("width",     "Width",        0.0f, 2.0f, 1.2f);
    addLin ("mix",       "Mix",          0.0f, 1.0f, 1.0f);
    addLin ("delayMix",  "Delay",        0.0f, 1.0f, 0.25f);
    addLin ("delayFb",   "Feedback",     0.0f, 0.9f, 0.45f);
    addLin ("shimmer",   "Shimmer",      0.0f, 1.0f, 0.3f);
    addLin ("reverbMix", "Reverb",       0.0f, 1.0f, 0.3f);
    addLin ("reverbSize","Size",         0.0f, 1.0f, 0.75f);
    addLin ("output",    "Output dB",   -12.0f, 6.0f, 0.0f);
    return l;
}

GrooveGardenProcessor::GrooveGardenProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
}

bool GrooveGardenProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo()
        && layouts.getMainInputChannelSet()  == juce::AudioChannelSet::stereo();
}

void GrooveGardenProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;
    juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) samplesPerBlock, 2 };
    for (auto& f : svf)
    {
        f.prepare (spec);
        f.setType (juce::dsp::StateVariableTPTFilterType::lowpass);
        f.setResonance (0.8f);
        f.reset();
    }
    for (auto& a : airShelf) a.reset();
    lastAir = -1.0f;

    for (int c = 0; c < 2; ++c)
    {
        delay[c].init ((int) (3.0 * sampleRate) + 8);
        pitch[c].init ((int) (0.1 * sampleRate) + 16);
        fbLP[c] = 0.0f;
    }
    curDelay = (float) (0.375 * sampleRate);
    reverb.setSampleRate (sampleRate);
    reverb.reset();
    fxBuf.setSize (2, samplesPerBlock);
    sendBuf.setSize (2, samplesPerBlock);
    gateGain = 1.0f; modSmooth = 0.0f;
}

void GrooveGardenProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    if (n == 0 || buffer.getNumChannels() < 2) return;

    // ---- transport ----
    double bpm = 120.0, ppq = 0.0;
    bool playing = false, havePpq = false;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            if (auto b = pos->getBpm()) bpm = *b;
            playing = pos->getIsPlaying();
            if (auto p = pos->getPpqPosition()) { ppq = *p; havePpq = true; }
        }
    if (bpm < 20.0) bpm = 120.0;
    double songPos = (playing && havePpq) ? ppq : freePos;
    const double qPerSample = bpm / 60.0 / sr;

    // ---- params ----
    float level[8];
    for (int i = 0; i < 8; ++i) level[i] = param (("step" + juce::String (i + 1)).toRawUTF8());
    static const double stepLens[5] = { 1.0, 0.5, 0.25, 1.0 / 6.0, 0.125 };
    const double stepLen = stepLens[juce::jlimit (0, 4, (int) param ("rate"))];
    const int shape      = (int) param ("shape");
    const float depth    = param ("depth");
    const float fAmt     = param ("filterAmt");
    const float cutoff   = param ("cutoff");
    const float drive    = param ("drive");
    const float air      = param ("air");
    const float width    = param ("width");
    const float mix      = param ("mix");
    const float dMix     = param ("delayMix");
    const float dFb      = param ("delayFb");
    const float shimmer  = param ("shimmer");
    const float rMix     = param ("reverbMix");
    const float rSize    = param ("reverbSize");
    const float outGain  = juce::Decibels::decibelsToGain (param ("output"));

    if (std::abs (air - lastAir) > 1.0e-4f)
    {
        auto c = juce::dsp::IIR::Coefficients<float>::makeHighShelf (
                    sr, 7500.0, 0.7f, juce::Decibels::decibelsToGain (air * 7.0f));
        airShelf[0].coefficients = c;
        airShelf[1].coefficients = c;
        lastAir = air;
    }

    juce::Reverb::Parameters rp;
    rp.roomSize = 0.3f + 0.69f * rSize;
    rp.damping = 0.4f;
    rp.wetLevel = 0.5f;
    rp.dryLevel = 0.0f;
    rp.width = 1.0f;
    rp.freezeMode = 0.0f;
    reverb.setParameters (rp);

    fxBuf.setSize (2, n, false, false, true);
    sendBuf.setSize (2, n, false, false, true);

    float* inL = buffer.getWritePointer (0);
    float* inR = buffer.getWritePointer (1);
    float* fxL = fxBuf.getWritePointer (0);
    float* fxR = fxBuf.getWritePointer (1);
    float* sL  = sendBuf.getWritePointer (0);
    float* sR  = sendBuf.getWritePointer (1);

    const float smoothCoef = 1.0f - std::exp (-1.0f / (0.0015f * (float) sr));
    const float modCoef    = 1.0f - std::exp (-1.0f / (0.004f * (float) sr));
    const float fbLpCoef   = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * 5000.0f / (float) sr);
    const float delayTarget = juce::jlimit (64.0f, (float) delay[0].buf.size() - 8.0f,
                                            (float) (0.75 * 60.0 / bpm * sr));
    const float pitchWindow = 0.06f * (float) sr;
    const float driveG = 1.0f + drive * 6.0f;
    const float driveComp = 1.0f / std::sqrt (driveG);
    const float pi = juce::MathConstants<float>::pi;
    int lastIdx = 0;

    for (int i = 0; i < n; ++i)
    {
        // ---- step sequencer ----
        const double stepPos = songPos / stepLen;
        const double fl = std::floor (stepPos);
        const float frac = (float) (stepPos - fl);
        const int idx = (int) ((((long long) fl % 8) + 8) % 8);
        lastIdx = idx;

        float env = 1.0f;
        if (shape == 0)      env = std::sin (pi * frac);
        else if (shape == 1) env = 1.0f - std::abs (2.0f * frac - 1.0f);

        const float m = level[idx] * env;
        const float target = (1.0f - depth) + depth * m;
        gateGain += (target - gateGain) * smoothCoef;
        modSmooth += (m - modSmooth) * modCoef;

        // ---- tone chain ----
        float l = inL[i] * gateGain;
        float r = inR[i] * gateGain;

        if (drive > 0.001f)
        {
            l = std::tanh (driveG * l) * driveComp;
            r = std::tanh (driveG * r) * driveComp;
        }

        const float cut = juce::jlimit (60.0f, 19000.0f,
                          cutoff * std::pow (2.0f, fAmt * 6.0f * (modSmooth - 0.5f)));
        svf[0].setCutoffFrequency (cut);
        svf[1].setCutoffFrequency (cut);
        l = svf[0].processSample (0, l);
        r = svf[1].processSample (1, r);

        l = airShelf[0].processSample (l);
        r = airShelf[1].processSample (r);

        // ---- ping-pong delay ----
        curDelay += (delayTarget - curDelay) * 0.0005f;
        const float dl = delay[0].read (curDelay);
        const float dr = delay[1].read (curDelay);
        fbLP[0] += (dl - fbLP[0]) * fbLpCoef;
        fbLP[1] += (dr - fbLP[1]) * fbLpCoef;
        const float mono = 0.5f * (l + r);
        delay[0].push (mono + std::tanh (fbLP[1] * dFb));
        delay[1].push (std::tanh (fbLP[0] * dFb));
        l += dl * dMix;
        r += dr * dMix;

        // ---- shimmer send ----
        const float shL = pitch[0].process (l, pitchWindow);
        const float shR = pitch[1].process (r, pitchWindow);
        sL[i] = l * 0.6f + shL * shimmer;
        sR[i] = r * 0.6f + shR * shimmer;

        fxL[i] = l;
        fxR[i] = r;

        songPos += qPerSample;
    }

    reverb.processStereo (sL, sR, n);

    // ---- final mix ----
    for (int i = 0; i < n; ++i)
    {
        float l = fxL[i] + sL[i] * rMix;
        float r = fxR[i] + sR[i] * rMix;

        const float mid = 0.5f * (l + r);
        const float side = 0.5f * (l - r) * width;
        l = mid + side;
        r = mid - side;

        inL[i] = (inL[i] * (1.0f - mix) + l * mix) * outGain;
        inR[i] = (inR[i] * (1.0f - mix) + r * mix) * outGain;
    }

    freePos = songPos;
    currentStep.store (lastIdx);
}

juce::AudioProcessorEditor* GrooveGardenProcessor::createEditor()
{
    return new GrooveGardenEditor (*this);
}

void GrooveGardenProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void GrooveGardenProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new GrooveGardenProcessor();
}
