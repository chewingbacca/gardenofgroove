#include "PluginProcessor.h"
#include "PluginEditor.h"

using Param = juce::AudioParameterFloat;
using juce::ParameterID;
using juce::NormalisableRange;

static const char* kStepIds[8] = { "step1", "step2", "step3", "step4", "step5", "step6", "step7", "step8" };
static const char* kNoteIds[8] = { "note1", "note2", "note3", "note4", "note5", "note6", "note7", "note8" };

juce::AudioProcessorValueTreeState::ParameterLayout GrooveGardenProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout l;

    const float stepDefaults[8] = { 1.0f, 0.45f, 0.8f, 0.3f, 1.0f, 0.55f, 0.7f, 0.35f };
    const int noteDefaults[8]   = { 0, 2, 4, 7, 4, 2, 5, 9 };
    for (int i = 0; i < 8; ++i)
    {
        l.add (std::make_unique<Param> (ParameterID { kStepIds[i], 1 }, "Step " + juce::String (i + 1),
                                        NormalisableRange<float> (0.0f, 1.0f), stepDefaults[i]));
        l.add (std::make_unique<juce::AudioParameterInt> (ParameterID { kNoteIds[i], 1 }, "Note " + juce::String (i + 1),
                                                          0, 14, noteDefaults[i]));
    }

    l.add (std::make_unique<juce::AudioParameterChoice> (ParameterID { "rate", 1 }, "Rate",
            juce::StringArray { "1/4", "1/8", "1/16", "1/16T", "1/32" }, 2));
    l.add (std::make_unique<juce::AudioParameterChoice> (ParameterID { "shape", 1 }, "Shape",
            juce::StringArray { "Sine", "Triangle", "Rect" }, 0));
    l.add (std::make_unique<juce::AudioParameterChoice> (ParameterID { "root", 1 }, "Root",
            juce::StringArray { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" }, 9));
    l.add (std::make_unique<juce::AudioParameterChoice> (ParameterID { "scale", 1 }, "Scale",
            juce::StringArray { "Minor", "Major", "Dorian", "Phrygian", "Lydian", "Mixolydian", "Pent Minor", "Pent Major" }, 0));
    l.add (std::make_unique<juce::AudioParameterBool> (ParameterID { "freeze", 1 }, "Freeze", false));
    l.add (std::make_unique<juce::AudioParameterBool> (ParameterID { "pluckOn", 1 },   "Melody On",  true));
    l.add (std::make_unique<juce::AudioParameterBool> (ParameterID { "seqOn", 1 },     "Chop On",    true));
    l.add (std::make_unique<juce::AudioParameterBool> (ParameterID { "evolveOn", 1 },  "Evolve On",  true));
    l.add (std::make_unique<juce::AudioParameterBool> (ParameterID { "motionOn", 1 },  "Motion On",  true));
    l.add (std::make_unique<juce::AudioParameterBool> (ParameterID { "granOn", 1 },    "Grains On",  true));
    l.add (std::make_unique<juce::AudioParameterBool> (ParameterID { "delayOn", 1 },   "Delay On",   true));
    l.add (std::make_unique<juce::AudioParameterBool> (ParameterID { "shimmerOn", 1 }, "Shimmer On", true));
    l.add (std::make_unique<juce::AudioParameterBool> (ParameterID { "reverbOn", 1 },  "Reverb On",  true));

    auto lin = [&] (const char* id, const char* name, float lo, float hi, float def)
    {
        l.add (std::make_unique<Param> (ParameterID { id, 1 }, name, NormalisableRange<float> (lo, hi), def));
    };
    auto skew = [&] (const char* id, const char* name, float lo, float hi, float def, float centre)
    {
        NormalisableRange<float> r (lo, hi);
        r.setSkewForCentre (centre);
        l.add (std::make_unique<Param> (ParameterID { id, 1 }, name, r, def));
    };

    // sequence + tone
    lin  ("depth",     "Depth",       0.0f, 1.0f, 0.85f);
    lin  ("source",    "Source",      0.0f, 1.0f, 0.8f);
    lin  ("evolve",    "Evolve",      0.0f, 1.0f, 0.25f);
    lin  ("filterAmt", "Filter Seq",  0.0f, 1.0f, 0.4f);
    skew ("cutoff",    "Cutoff",      200.0f, 18000.0f, 9000.0f, 2000.0f);
    lin  ("drive",     "Drive",       0.0f, 1.0f, 0.2f);
    lin  ("air",       "Air",         0.0f, 1.0f, 0.5f);
    lin  ("motion",    "Motion",      0.0f, 1.0f, 0.4f);

    // granulator
    lin  ("gLevel",      "Grain Level",  0.0f, 1.0f, 0.45f);
    skew ("gSize",       "Grain Size",   10.0f, 500.0f, 90.0f, 90.0f);
    lin  ("gDensity",    "Density",      0.0f, 1.0f, 0.35f);
    lin  ("gBurst",      "Burst",        0.0f, 1.0f, 0.4f);
    lin  ("gPos",        "Position",     0.0f, 1.0f, 0.15f);
    lin  ("gSpray",      "Spray",        0.0f, 1.0f, 0.3f);
    l.add (std::make_unique<Param> (ParameterID { "gPitch", 1 }, "Grain Pitch",
                                    NormalisableRange<float> (-24.0f, 24.0f, 1.0f), 12.0f));
    lin  ("gPitchSpray", "Pitch Spray",  0.0f, 1.0f, 0.5f);
    lin  ("gScatter",    "Scatter",      0.0f, 1.0f, 0.8f);
    lin  ("gReverse",    "Reverse",      0.0f, 1.0f, 0.2f);

    // pluck / autolead
    lin  ("pLevel", "Pluck Level", 0.0f, 1.0f, 0.5f);
    lin  ("pDecay", "Pluck Decay", 0.0f, 1.0f, 0.6f);
    lin  ("pTone",  "Pluck Tone",  0.0f, 1.0f, 0.5f);
    lin  ("pPick",  "Pick",        0.0f, 1.0f, 0.4f);
    l.add (std::make_unique<juce::AudioParameterInt> (ParameterID { "pOct", 1 }, "Pluck Octave", -2, 2, 0));

    // space + master
    lin  ("delayMix",   "Delay",     0.0f, 1.0f, 0.25f);
    lin  ("delayFb",    "Feedback",  0.0f, 0.9f, 0.45f);
    lin  ("shimmer",    "Shimmer",   0.0f, 1.0f, 0.3f);
    lin  ("reverbMix",  "Reverb",    0.0f, 1.0f, 0.35f);
    lin  ("reverbSize", "Size",      0.0f, 1.0f, 0.8f);
    lin  ("width",      "Width",     0.0f, 2.0f, 1.2f);
    lin  ("mix",        "Mix",       0.0f, 1.0f, 1.0f);
    lin  ("output",     "Output dB", -12.0f, 6.0f, 0.0f);
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
    gg::scaleTable(); // make sure the static table exists before audio runs

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
        chorus[c].init ((int) (0.05 * sampleRate) + 16);
        gBuf[c].assign ((size_t) (6.0 * sampleRate), 0.0f);
        fbLP[c] = 0.0f;
    }
    gWrite = 0;
    for (auto& g : grains) g.active = false;
    for (auto& p : plucks)
    {
        p.line.init ((int) (sampleRate / 20.0) + 16);
        p.active = false;
    }
    nextPluck = 0;
    grainTimer = 0.0f;
    chorusPhase = 0.0f;
    liveInit = false;
    prevIdx = -1;
    pluckSm = 1.0f;
    grainSm = 1.0f;

    curDelay = (float) (0.375 * sampleRate);
    reverb.setSampleRate (sampleRate);
    reverb.reset();
    fxBuf.setSize (2, samplesPerBlock);
    sendBuf.setSize (2, samplesPerBlock);
    gateGain = 1.0f; modSmooth = 0.0f;
}

static inline float readInterp (const std::vector<float>& buf, double pos)
{
    const int n = (int) buf.size();
    int i0 = (int) pos;
    const float f = (float) (pos - (double) i0);
    i0 %= n;
    if (i0 < 0) i0 += n;
    const int i1 = (i0 + 1) % n;
    return buf[(size_t) i0] * (1.0f - f) + buf[(size_t) i1] * f;
}

void GrooveGardenProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    if (n == 0 || buffer.getNumChannels() < 2) return;

    // ---- transport ----
    double bpm = 120.0, ppq = 0.0;
    bool playing = false, havePpq = false, hasTransport = false;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            hasTransport = true;
            if (auto b = pos->getBpm()) bpm = *b;
            playing = pos->getIsPlaying();
            if (auto p = pos->getPpqPosition()) { ppq = *p; havePpq = true; }
        }
    if (bpm < 20.0) bpm = 120.0;
    // the sequencer, melody and grain bursts only run while the host is playing
    // (hosts that give no transport info at all keep the old free-running behaviour)
    const bool running = hasTransport ? playing : true;
    double songPos = (playing && havePpq) ? ppq : freePos;
    const double qPerSample = bpm / 60.0 / sr;

    // ---- params ----
    float baseLevel[8];
    int baseNote[8];
    for (int i = 0; i < 8; ++i)
    {
        baseLevel[i] = param (kStepIds[i]);
        baseNote[i]  = (int) std::lround (param (kNoteIds[i]));
    }
    static const double stepLens[5] = { 1.0, 0.5, 0.25, 1.0 / 6.0, 0.125 };
    const double stepLen = stepLens[juce::jlimit (0, 4, (int) param ("rate"))];
    const int shape      = (int) param ("shape");
    const int rootIdx    = (int) param ("root");
    const int scaleIdx   = (int) param ("scale");
    const bool freeze    = param ("freeze") > 0.5f;
    const bool pluckOn   = param ("pluckOn") > 0.5f;
    const bool seqOn     = param ("seqOn") > 0.5f;
    const bool evolveOn  = param ("evolveOn") > 0.5f;
    const bool motionOn  = param ("motionOn") > 0.5f;
    const bool granOn    = param ("granOn") > 0.5f;
    const bool delayOn   = param ("delayOn") > 0.5f;
    const bool shimmerOn = param ("shimmerOn") > 0.5f;
    const bool reverbOn  = param ("reverbOn") > 0.5f;
    const float depth    = param ("depth");
    const float source   = param ("source");
    const float evolve   = param ("evolve");
    const float fAmt     = param ("filterAmt");
    const float cutoff   = param ("cutoff");
    const float drive    = param ("drive");
    const float air      = param ("air");
    const float motion   = param ("motion");
    const float gLevel   = param ("gLevel");
    const float gSize    = param ("gSize");
    const float gDensity = param ("gDensity");
    const float gBurst   = param ("gBurst");
    const float gPos     = param ("gPos");
    const float gSpray   = param ("gSpray");
    const float gPitch   = param ("gPitch");
    const float gPSpray  = param ("gPitchSpray");
    const float gScatter = param ("gScatter");
    const float gReverse = param ("gReverse");
    const float pLevel   = param ("pLevel");
    const float pDecay   = param ("pDecay");
    const float pTone    = param ("pTone");
    const float pPick    = param ("pPick");
    const int   pOct     = (int) std::lround (param ("pOct"));
    const float width    = param ("width");
    const float mix      = param ("mix");
    const float dMix     = param ("delayMix");
    const float dFb      = param ("delayFb");
    const float shimmer  = shimmerOn ? param ("shimmer") : 0.0f;
    const float rMix     = reverbOn ? param ("reverbMix") : 0.0f;
    const float rSize    = param ("reverbSize");
    const float outGain  = juce::Decibels::decibelsToGain (param ("output"));

    const bool doEvolve = evolveOn && evolve >= 0.001f;
    if (!liveInit || !doEvolve)
    {
        for (int i = 0; i < 8; ++i) { liveLevel[i] = baseLevel[i]; liveNote[i] = baseNote[i]; }
        liveInit = true;
    }

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

    const float fsr = (float) sr;
    const float pi = juce::MathConstants<float>::pi;
    const float twoPi = juce::MathConstants<float>::twoPi;
    const float smoothCoef = 1.0f - std::exp (-1.0f / (0.0015f * fsr));
    const float modCoef    = 1.0f - std::exp (-1.0f / (0.004f * fsr));
    const float fbLpCoef   = 1.0f - std::exp (-twoPi * 5000.0f / fsr);
    const float delayTarget = juce::jlimit (64.0f, (float) delay[0].buf.size() - 8.0f,
                                            (float) (0.75 * 60.0 / bpm * sr));
    const float pitchWindow = 0.06f * fsr;
    const float driveG = 1.0f + drive * 6.0f;
    const float driveComp = 1.0f / std::sqrt (driveG);
    const double gBufLen = (double) gBuf[0].size();
    const int gBufSize = (int) gBuf[0].size();
    const float pluckFb = 0.96f + 0.0385f * pDecay;
    const float pluckA  = 0.08f + 0.85f * pTone;
    int lastIdx = 0;

    // ---- helpers ----
    auto mutate = [&]
    {
        for (int k = 0; k < 8; ++k)
        {
            liveLevel[k] = baseLevel[k];
            liveNote[k]  = baseNote[k];
            if (rng.nextFloat() < evolve)
            {
                if (rng.nextFloat() < 0.2f)
                    liveLevel[k] = 0.0f;
                else
                    liveLevel[k] = juce::jmin (1.0f, baseLevel[k] * (0.4f + 0.9f * rng.nextFloat())
                                                      + (baseLevel[k] < 0.05f ? 0.3f * rng.nextFloat() : 0.0f));
            }
            if (rng.nextFloat() < evolve)
                liveNote[k] = juce::jlimit (0, 14, baseNote[k] + rng.nextInt (5) - 2);
        }
    };

    auto spawnGrain = [&] (float vel)
    {
        for (auto& g : grains)
        {
            if (g.active) continue;

            const float jitter = 0.75f + 0.5f * rng.nextFloat();
            const int len = juce::jmax (64, (int) (gSize * 0.001f * jitter * fsr));

            float semis = gPitch;
            if (rng.nextFloat() < gPSpray)
            {
                static const int degs[8] = { -7, -3, 0, 2, 4, 7, 9, 11 };
                semis += (float) gg::scaleSemitone (scaleIdx, degs[rng.nextInt (8)]);
            }
            semis = juce::jlimit (-36.0f, 36.0f, semis);
            const float ratio = juce::jlimit (0.25f, 4.0f, std::pow (2.0f, semis / 12.0f));

            const bool rev = rng.nextFloat() < gReverse;
            double offset = (0.02 + gPos * 2.5 + rng.nextFloat() * gSpray * 1.0) * sr;
            offset = juce::jmax (offset, (double) len * (double) juce::jmax (1.0f, ratio) + 16.0);
            offset = juce::jmin (offset, gBufLen - 8.0);

            double start = (double) gWrite - offset;
            if (rev) start += (double) len * (double) ratio;
            while (start < 0.0) start += gBufLen;
            while (start >= gBufLen) start -= gBufLen;

            g.pos = start;
            g.inc = rev ? -(double) ratio : (double) ratio;
            g.age = 0;
            g.len = len;
            const float pan = (rng.nextFloat() * 2.0f - 1.0f) * gScatter;
            const float ang = (pan + 1.0f) * 0.25f * pi;
            g.gl = std::cos (ang) * 1.4142f;
            g.gr = std::sin (ang) * 1.4142f;
            g.gain = vel * gLevel * 0.6f;
            g.active = true;
            return;
        }
    };

    auto triggerPluck = [&] (int degree, float vel)
    {
        auto& v = plucks[(size_t) nextPluck];
        nextPluck = (nextPluck + 1) % (int) plucks.size();

        const int midi = 48 + rootIdx + pOct * 12 + gg::scaleSemitone (scaleIdx, degree);
        const float f = juce::jlimit (30.0f, 5000.0f, 440.0f * std::pow (2.0f, ((float) midi - 69.0f) / 12.0f));
        const float d = juce::jlimit (4.0f, (float) v.line.buf.size() - 4.0f,
                                      fsr / f - (1.0f - pluckA) / pluckA);
        v.line.clear();
        v.delay = d;
        v.fb = pluckFb;
        v.a = pluckA;
        v.lp = 0.0f;
        v.vel = vel;
        v.env = 0.0f;
        v.excLen = juce::jlimit ((int) (0.002f * fsr), (int) (0.02f * fsr), (int) (d * 2.0f));
        v.excLeft = v.excLen;
        v.age = 0;
        const float pan = (rng.nextFloat() - 0.5f) * 0.8f;
        const float ang = (pan + 1.0f) * 0.25f * pi;
        v.gl = std::cos (ang) * 1.4142f;
        v.gr = std::sin (ang) * 1.4142f;
        v.active = true;
    };

    // ---- main loop ----
    for (int i = 0; i < n; ++i)
    {
        // step sequencer
        const double stepPos = songPos / stepLen;
        const double fl = std::floor (stepPos);
        const float frac = (float) (stepPos - fl);
        const int idx = (int) ((((long long) fl % 8) + 8) % 8);
        lastIdx = idx;

        if (!running)
            prevIdx = -1;   // so step 1 fires again as soon as playback starts
        else if (idx != prevIdx)
        {
            if (idx == 0 && doEvolve) mutate();
            prevIdx = idx;
            const float lv = liveLevel[idx];
            if (lv > 0.05f)
            {
                if (pluckOn && pLevel > 0.001f) triggerPluck (liveNote[idx], lv);
                if (granOn && gBurst > 0.001f && gLevel > 0.001f)
                {
                    const int cnt = 1 + (int) (gBurst * 6.0f);
                    for (int k = 0; k < cnt; ++k) spawnGrain (lv);
                }
            }
        }

        float env = 1.0f;
        if (shape == 0)      env = std::sin (pi * frac);
        else if (shape == 1) env = 1.0f - std::abs (2.0f * frac - 1.0f);

        const bool chop = running && seqOn;
        const float m = chop ? liveLevel[idx] * env : 0.5f;
        const float target = chop ? (1.0f - depth) + depth * m : 1.0f;
        gateGain += (target - gateGain) * smoothCoef;
        modSmooth += (m - modSmooth) * modCoef;

        const float rawL = inL[i];
        const float rawR = inR[i];

        // live record for granulator
        if (!freeze)
        {
            gBuf[0][(size_t) gWrite] = rawL;
            gBuf[1][(size_t) gWrite] = rawR;
            gWrite = (gWrite + 1) % gBufSize;
        }

        // sequenced source + tone chain
        float l = rawL * gateGain;
        float r = rawR * gateGain;

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

        l = airShelf[0].processSample (l) * source;
        r = airShelf[1].processSample (r) * source;

        // free-running grain clouds
        if (running && granOn && gDensity > 0.001f && gLevel > 0.001f)
        {
            grainTimer -= 1.0f;
            if (grainTimer <= 0.0f)
            {
                spawnGrain (0.7f + 0.3f * rng.nextFloat());
                const float rate = 0.5f + gDensity * gDensity * 70.0f;
                grainTimer = fsr / rate * (0.5f + rng.nextFloat());
            }
        }

        // fade grains / plucks out when switched off, then silence the voices
        grainSm += ((granOn ? 1.0f : 0.0f) - grainSm) * 0.003f;
        pluckSm += ((pluckOn ? 1.0f : 0.0f) - pluckSm) * 0.003f;
        if (! granOn && grainSm < 1.0e-4f && grainSm > 0.0f)
        {
            grainSm = 0.0f;
            for (auto& g : grains) g.active = false;
        }
        if (! pluckOn && pluckSm < 1.0e-4f && pluckSm > 0.0f)
        {
            pluckSm = 0.0f;
            for (auto& v : plucks) v.active = false;
        }

        float gOutL = 0.0f, gOutR = 0.0f;
        for (auto& g : grains)
        {
            if (!g.active) continue;
            const float w0 = std::sin (pi * (float) g.age / (float) g.len);
            const float w = w0 * w0 * g.gain;
            gOutL += readInterp (gBuf[0], g.pos) * g.gl * w;
            gOutR += readInterp (gBuf[1], g.pos) * g.gr * w;
            g.pos += g.inc;
            if (g.pos >= gBufLen) g.pos -= gBufLen;
            else if (g.pos < 0.0) g.pos += gBufLen;
            if (++g.age >= g.len) g.active = false;
        }

        // plucked strings excited by the input
        float pOutL = 0.0f, pOutR = 0.0f;
        const float monoIn = 0.5f * (rawL + rawR);
        for (auto& v : plucks)
        {
            if (!v.active) continue;
            const float y = v.line.read (v.delay);
            v.lp += v.a * (y - v.lp);
            float inj = 0.0f;
            if (v.excLeft > 0)
            {
                const float e = (float) v.excLeft / (float) v.excLen;
                inj = (monoIn * (1.0f - pPick) * 2.0f + (rng.nextFloat() * 2.0f - 1.0f) * pPick * 0.6f) * e * v.vel;
                --v.excLeft;
            }
            v.line.push (inj + v.lp * v.fb);
            pOutL += y * v.gl;
            pOutR += y * v.gr;
            v.env += (std::abs (y) - v.env) * 0.001f;
            if (++v.age > v.excLen * 2 && v.env < 1.0e-6f) v.active = false;
        }

        l += std::tanh (gOutL) * grainSm + std::tanh (pOutL * pLevel * 1.2f) * 0.8f * pluckSm;
        r += std::tanh (gOutR) * grainSm + std::tanh (pOutR * pLevel * 1.2f) * 0.8f * pluckSm;

        // motion (chorus)
        if (motionOn && motion > 0.001f)
        {
            chorus[0].push (l);
            chorus[1].push (r);
            const float dL = (0.012f + 0.006f * motion * std::sin (twoPi * chorusPhase)) * fsr;
            const float dR = (0.012f + 0.006f * motion * std::sin (twoPi * (chorusPhase + 0.37f))) * fsr;
            const float cl = chorus[0].read (dL);
            const float cr = chorus[1].read (dR);
            l = l * (1.0f - 0.35f * motion) + cl * 0.5f * motion;
            r = r * (1.0f - 0.35f * motion) + cr * 0.5f * motion;
            chorusPhase += 0.27f / fsr;
            if (chorusPhase >= 1.0f) chorusPhase -= 1.0f;
        }

        // ping-pong delay
        curDelay += (delayTarget - curDelay) * 0.0005f;
        const float dl = delay[0].read (curDelay);
        const float dr = delay[1].read (curDelay);
        fbLP[0] += (dl - fbLP[0]) * fbLpCoef;
        fbLP[1] += (dr - fbLP[1]) * fbLpCoef;
        const float mono = 0.5f * (l + r);
        if (delayOn)
        {
            delay[0].push (mono + std::tanh (fbLP[1] * dFb));
            delay[1].push (std::tanh (fbLP[0] * dFb));
            l += dl * dMix;
            r += dr * dMix;
        }
        else
        {
            delay[0].push (0.0f);
            delay[1].push (0.0f);
        }

        // shimmer send
        const float shL = pitch[0].process (l, pitchWindow);
        const float shR = pitch[1].process (r, pitchWindow);
        sL[i] = l * 0.6f + shL * shimmer;
        sR[i] = r * 0.6f + shR * shimmer;

        fxL[i] = l;
        fxR[i] = r;

        if (running) songPos += qPerSample;
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

        // soft limiter: transparent below 0.8, gently caps at 1.0
        auto soft = [] (float x)
        {
            const float a = std::abs (x);
            if (a <= 0.8f) return x;
            const float y = 0.8f + 0.2f * std::tanh ((a - 0.8f) / 0.2f);
            return x < 0.0f ? -y : y;
        };
        l = soft (l);
        r = soft (r);

        inL[i] = (inL[i] * (1.0f - mix) + l * mix) * outGain;
        inR[i] = (inR[i] * (1.0f - mix) + r * mix) * outGain;
    }

    freePos = songPos;
    currentStep.store (running ? lastIdx : -1);
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
