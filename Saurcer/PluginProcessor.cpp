#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace juce;

//==============================================================================
static NormalisableRange<float> logRange (float lo, float hi)
{
    NormalisableRange<float> r (lo, hi);
    r.setSkewForCentre (std::sqrt (lo * hi));
    return r;
}

AudioProcessorValueTreeState::ParameterLayout LaPautaSLChannelAudioProcessor::createLayout()
{
    AudioProcessorValueTreeState::ParameterLayout layout;

    auto addF = [&] (const char* id, const char* name, NormalisableRange<float> r, float def, const char* unit)
    {
        layout.add (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name, r, def,
                                                           AudioParameterFloatAttributes().withLabel (unit)));
    };
    auto addB = [&] (const char* id, const char* name, bool def = false)
    {
        layout.add (std::make_unique<AudioParameterBool> (ParameterID { id, 1 }, name, def));
    };

    // Ganancia / fase / analógico
    addF (ID::inGain,  "Input",  { -18.f, 18.f }, 0.f, "dB");
    addF (ID::outGain, "Output", { -24.f, 12.f }, 0.f, "dB");
    addB (ID::phase,  "Phase");
    addB (ID::analog, "Analog");

    // Filtros
    addB (ID::hpfOn, "HPF On");  addF (ID::hpf, "HPF", logRange (20.f, 350.f),     20.f,    "Hz");
    addB (ID::lpfOn, "LPF On");  addF (ID::lpf, "LPF", logRange (3000.f, 12000.f), 12000.f, "Hz");

    // EQ
    addF (ID::hfGain, "HF Gain", { -16.f, 16.f }, 0.f, "dB");
    addF (ID::hfFreq, "HF Freq", logRange (1500.f, 16000.f), 8000.f, "Hz");
    addB (ID::hfBell, "HF Bell");

    addF (ID::hmfGain, "HMF Gain", { -16.f, 16.f }, 0.f, "dB");
    addF (ID::hmfFreq, "HMF Freq", logRange (600.f, 7000.f), 3000.f, "Hz");
    addF (ID::hmfQ,    "HMF Q",    logRange (0.5f, 2.5f), 1.f, "");
    addB (ID::hmfX3,   "HMF x3");

    addF (ID::lmfGain, "LMF Gain", { -16.f, 16.f }, 0.f, "dB");
    addF (ID::lmfFreq, "LMF Freq", logRange (200.f, 2500.f), 600.f, "Hz");
    addF (ID::lmfQ,    "LMF Q",    logRange (0.5f, 2.5f), 1.f, "");
    addB (ID::lmfD3,   "LMF /3");

    addF (ID::lfGain, "LF Gain", { -16.f, 16.f }, 0.f, "dB");
    addF (ID::lfFreq, "LF Freq", logRange (30.f, 450.f), 100.f, "Hz");
    addB (ID::lfBell, "LF Bell");

    // Ruteo
    addB (ID::eqByp,   "EQ Bypass");
    addB (ID::eqSc,    "Flt/EQ to Dyn SC");
    addB (ID::dynByp,  "Dyn Bypass");
    addB (ID::dynPost, "Dyn to Ch Out");

    // Compresor
    addF (ID::compThr,   "Comp Threshold", { -20.f, 10.f }, 10.f, "dB");
    addF (ID::compRatio, "Comp Ratio",     { 1.f, 20.f }, 1.f, ":1");
    addF (ID::compRel,   "Comp Release",   logRange (0.1f, 4.f), 0.4f, "s");
    addB (ID::fastAtk,   "Fast Attack");

    // Expansor / Gate
    addF (ID::gateThr,   "Gate Threshold", { -30.f, 10.f }, -30.f, "dB");
    addF (ID::gateRange, "Gate Range",     { 0.f, 40.f }, 0.f, "dB");
    addF (ID::gateRel,   "Gate Release",   logRange (0.1f, 4.f), 0.1f, "s");
    addB (ID::gateMode,  "Gate Mode");

    return layout;
}

//==============================================================================
LaPautaSLChannelAudioProcessor::LaPautaSLChannelAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  AudioChannelSet::stereo(), true)
                          .withOutput ("Output", AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
}

bool LaPautaSLChannelAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    if (out != AudioChannelSet::mono() && out != AudioChannelSet::stereo())
        return false;
    return out == layouts.getMainInputChannelSet();
}

void LaPautaSLChannelAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    fs = sampleRate;
    eq.reset();
    scEq.reset();
    dyn.prepare (sampleRate);
    scBuf.setSize (2, jmax (1, samplesPerBlock));
    prevIn = prevOut = 1.f;
}

//==============================================================================
void LaPautaSLChannelAudioProcessor::processBlock (AudioBuffer<float>& buf, MidiBuffer&)
{
    ScopedNoDenormals noDenormals;

    const int n   = buf.getNumSamples();
    const int nch = jmin (2, buf.getNumChannels());
    for (int c = getTotalNumInputChannels(); c < getTotalNumOutputChannels(); ++c)
        buf.clear (c, 0, n);

    auto P = [this] (const char* id) { return apvts.getRawParameterValue (id)->load(); };
    auto B = [&P]  (const char* id) { return P (id) > 0.5f; };

    // ---- Entrada: fase + ganancia ----
    const float inG = Decibels::decibelsToGain (P (ID::inGain)) * (B (ID::phase) ? -1.f : 1.f);
    buf.applyGainRamp (0, n, prevIn, inG);
    prevIn = inG;

    float pk = 0.f;
    for (int c = 0; c < nch; ++c) pk = jmax (pk, buf.getMagnitude (c, 0, n));
    inPeak.store (pk);

    // ---- Parámetros ----
    lp::EqParams ep;
    ep.hpfOn = B (ID::hpfOn); ep.hpf = P (ID::hpf);
    ep.lpfOn = B (ID::lpfOn); ep.lpf = P (ID::lpf);
    ep.hfG = P (ID::hfGain);   ep.hfF = P (ID::hfFreq);   ep.hfBell = B (ID::hfBell);
    ep.hmfG = P (ID::hmfGain); ep.hmfF = P (ID::hmfFreq); ep.hmfQ = P (ID::hmfQ); ep.hmfX3 = B (ID::hmfX3);
    ep.lmfG = P (ID::lmfGain); ep.lmfF = P (ID::lmfFreq); ep.lmfQ = P (ID::lmfQ); ep.lmfD3 = B (ID::lmfD3);
    ep.lfG = P (ID::lfGain);   ep.lfF = P (ID::lfFreq);   ep.lfBell = B (ID::lfBell);

    lp::DynParams dp;
    dp.compThr = P (ID::compThr);   dp.compRatio = P (ID::compRatio);
    dp.compRelS = P (ID::compRel);  dp.fastAtk = B (ID::fastAtk);
    dp.gateThr = P (ID::gateThr);   dp.gateRange = P (ID::gateRange);
    dp.gateRelS = P (ID::gateRel);  dp.gateIsGate = B (ID::gateMode);

    const bool eqOn    = ! B (ID::eqByp);
    const bool dynOn   = ! B (ID::dynByp);
    const bool dynPost = B (ID::dynPost);
    const bool scFromEq = B (ID::eqSc);

    if (eqOn) eq.setup (ep, fs);
    if (dynOn && ! dynPost && scFromEq && eqOn) scEq.setup (ep, fs);

    float* ch[2] = { buf.getWritePointer (0), nch > 1 ? buf.getWritePointer (1) : nullptr };

    auto runEq = [&]
    {
        if (! eqOn) return;
        for (int c = 0; c < nch; ++c)
            for (int i = 0; i < n; ++i)
                ch[c][i] = eq.process (c, ch[c][i]);
    };

    auto runDyn = [&] (bool sidechainThroughEq)
    {
        if (! dynOn) { compGrDb.store (0.f); gateGrDb.store (0.f); return; }

        const float* sc[2] = { ch[0], ch[1] };
        if (sidechainThroughEq)
        {
            if (scBuf.getNumSamples() < n) scBuf.setSize (2, n, false, false, true);
            for (int c = 0; c < nch; ++c)
            {
                float* d = scBuf.getWritePointer (c);
                for (int i = 0; i < n; ++i)
                    d[i] = scEq.process (c, ch[c][i]);
                sc[c] = d;
            }
        }
        dyn.process (ch, sc, nch, n, dp);
        compGrDb.store (dyn.compGr);
        gateGrDb.store (dyn.gateGr);
    };

    if (dynPost) { runEq(); runDyn (false); }
    else         { runDyn (scFromEq && eqOn); runEq(); }

    // ---- Analógico: saturación suave + ruido muy bajo ----
    if (B (ID::analog))
    {
        for (int c = 0; c < nch; ++c)
            for (int i = 0; i < n; ++i)
            {
                noiseState ^= noiseState << 13; noiseState ^= noiseState >> 17; noiseState ^= noiseState << 5;
                const float noise = ((float) noiseState / 4294967295.f * 2.f - 1.f) * 3.0e-6f;
                ch[c][i] = std::tanh (ch[c][i] * 1.2f) / 1.2f + noise;
            }
    }

    // ---- Salida ----
    const float outG = Decibels::decibelsToGain (P (ID::outGain));
    buf.applyGainRamp (0, n, prevOut, outG);
    prevOut = outG;

    pk = 0.f;
    for (int c = 0; c < nch; ++c) pk = jmax (pk, buf.getMagnitude (c, 0, n));
    outPeak.store (pk);
}

//==============================================================================
void LaPautaSLChannelAudioProcessor::setCurrentProgram (int index)
{
    if (isPositiveAndBelow (index, getNumPrograms()))
    {
        currentProgram = index;
        applyFactoryPreset (index);
    }
}

const String LaPautaSLChannelAudioProcessor::getProgramName (int index)
{
    return isPositiveAndBelow (index, getNumPrograms()) ? String (factoryPresets()[(size_t) index].name) : String();
}

void LaPautaSLChannelAudioProcessor::applyFactoryPreset (int index)
{
    if (! isPositiveAndBelow (index, getNumPrograms())) return;

    for (auto* p : getParameters())
        p->setValueNotifyingHost (p->getDefaultValue());

    for (auto& kv : factoryPresets()[(size_t) index].values)
        if (auto* rp = apvts.getParameter (kv.first))
            rp->setValueNotifyingHost (rp->convertTo0to1 (kv.second));
}

File LaPautaSLChannelAudioProcessor::getUserPresetDir()
{
    auto d = File::getSpecialLocation (File::userApplicationDataDirectory)
                 .getChildFile ("La Pauta").getChildFile ("SL Channel");
    d.createDirectory();
    return d;
}

void LaPautaSLChannelAudioProcessor::saveUserPreset (const String& name)
{
    auto f = getUserPresetDir().getChildFile (File::createLegalFileName (name) + ".lpsl");
    if (auto xml = apvts.copyState().createXml())
        xml->writeTo (f);
}

bool LaPautaSLChannelAudioProcessor::loadUserPreset (const String& name)
{
    auto f = getUserPresetDir().getChildFile (File::createLegalFileName (name) + ".lpsl");
    if (auto xml = parseXML (f))
        if (xml->hasTagName (apvts.state.getType()))
        {
            apvts.replaceState (ValueTree::fromXml (*xml));
            return true;
        }
    return false;
}

StringArray LaPautaSLChannelAudioProcessor::getUserPresetNames() const
{
    StringArray names;
    for (auto& f : getUserPresetDir().findChildFiles (File::findFiles, false, "*.lpsl"))
        names.add (f.getFileNameWithoutExtension());
    names.sort (true);
    return names;
}

//==============================================================================
void LaPautaSLChannelAudioProcessor::getStateInformation (MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void LaPautaSLChannelAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (ValueTree::fromXml (*xml));
}

AudioProcessorEditor* LaPautaSLChannelAudioProcessor::createEditor()
{
    return new LaPautaSLChannelAudioProcessorEditor (*this);
}

AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new LaPautaSLChannelAudioProcessor();
}
