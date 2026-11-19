#include "HeatProcessor.h"
#include "HeatEditor.h"
using namespace juce;

AudioProcessorValueTreeState::ParameterLayout HeatAudioProcessor::createLayout()
{
    std::vector<std::unique_ptr<RangedAudioParameter>> p;
    auto dB  = AudioParameterFloatAttributes().withLabel ("dB");
    auto pct = AudioParameterFloatAttributes().withStringFromValueFunction (
        [] (float v, int) { return String (roundToInt (v * 100.0f)) + " %"; });
    auto driveTxt = AudioParameterFloatAttributes().withStringFromValueFunction (
        [] (float v, int) { return String (v * 5.0f, 1); });   // escala 5..-5 como el HEAT original
    auto toneTxt = AudioParameterFloatAttributes().withStringFromValueFunction (
        [] (float v, int) { return String (v * 3.0f, 1); });   // escala 3..-3

    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "input", 1 },  "Entrada",
        NormalisableRange<float> (-12.0f, 12.0f, 0.1f), 0.0f, dB));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "drive", 1 },  "Drive",
        NormalisableRange<float> (-1.0f, 1.0f, 0.001f), 0.0f, driveTxt));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "tone", 1 },   "Tone",
        NormalisableRange<float> (-1.0f, 1.0f, 0.001f), 0.0f, toneTxt));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "mix", 1 },    "Mezcla",
        NormalisableRange<float> (0.0f, 1.0f, 0.001f), 1.0f, pct));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "output", 1 }, "Salida",
        NormalisableRange<float> (-12.0f, 12.0f, 0.1f), 0.0f, dB));
    p.push_back (std::make_unique<AudioParameterBool> (ParameterID { "auto", 1 },    "Auto-gain", true));
    return { p.begin(), p.end() };
}

HeatAudioProcessor::HeatAudioProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  AudioChannelSet::stereo(), true)
                        .withOutput ("Output", AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "SilkHeatState", createLayout())
{
    pInput  = apvts.getRawParameterValue ("input");
    pDrive  = apvts.getRawParameterValue ("drive");
    pTone   = apvts.getRawParameterValue ("tone");
    pMix    = apvts.getRawParameterValue ("mix");
    pOutput = apvts.getRawParameterValue ("output");
    pAuto   = apvts.getRawParameterValue ("auto");
}

bool HeatAudioProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto out = l.getMainOutputChannelSet();
    return (out == AudioChannelSet::mono() || out == AudioChannelSet::stereo())
           && out == l.getMainInputChannelSet();
}

//==============================================================================
namespace
{
    struct Stage { float aTape, dTube, w, g1, g2, bias; };

    // Mapa del Drive bipolar -> etapas. aTape/dTube en 0..1.
    Stage stageFor (float d)
    {
        Stage s;
        s.aTape = d < 0.0f ? -d : 0.45f;           // derecha: cinta moderada fija + tubo
        s.dTube = jmax (0.0f, d);
        s.w     = jmin (1.0f, std::abs (d) / 0.1f); // exactamente neutro en el centro
        s.g1    = 0.8f + 11.0f * s.aTape * s.aTape;
        s.g2    = 0.8f + 8.0f * s.dTube * s.dTube;
        s.bias  = 0.15f * s.dTube;
        return s;
    }
}

void HeatAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;
    const int nch = jmax (1, getTotalNumInputChannels());
    oversampler.initProcessing ((size_t) samplesPerBlock);
    oversampler.reset();
    const int latency = (int) std::round (oversampler.getLatencyInSamples());
    setLatencySamples (latency);

    dsp::ProcessSpec spec { sampleRate, (uint32) samplesPerBlock, (uint32) nch };
    dryDelay.prepare (spec);
    dryDelay.setDelay ((float) latency);
    dryDelay.reset();
    dryBuffer.setSize (2, samplesPerBlock);
    dryBuffer.clear();

    for (auto* f : { &preLow, &preHigh, &postLow, &postHigh, &lowPass, &toneShelf, &dcBlock }) f->reset();
    dcBlock.setHighPass1 (sr, 15.0);

    const auto st = stageFor (pDrive->load());
    inSm.reset (sampleRate, 0.04);  inSm.setCurrentAndTargetValue (Decibels::decibelsToGain (pInput->load()));
    outSm.reset (sampleRate, 0.04); outSm.setCurrentAndTargetValue (Decibels::decibelsToGain (pOutput->load()));
    mixSm.reset (sampleRate, 0.04); mixSm.setCurrentAndTargetValue (pMix->load());
    driveSm.reset (sampleRate, 0.05); driveSm.setCurrentAndTargetValue (pDrive->load());
    toneSm.reset (sampleRate, 0.05);  toneSm.setCurrentAndTargetValue (pTone->load());
    g1Sm.reset (sampleRate * 4.0, 0.04);   g1Sm.setCurrentAndTargetValue (st.g1);
    g2Sm.reset (sampleRate * 4.0, 0.04);   g2Sm.setCurrentAndTargetValue (st.g2);
    biasSm.reset (sampleRate * 4.0, 0.04); biasSm.setCurrentAndTargetValue (st.bias);
    msIn = msOut = 0.0f; agGain = 1.0f;
}

void HeatAudioProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer&)
{
    ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    const int nch = jmin (buffer.getNumChannels(), 2);
    if (n == 0 || nch == 0) return;
    for (int c = getTotalNumInputChannels(); c < getTotalNumOutputChannels(); ++c) buffer.clear (c, 0, n);

    inSm.setTargetValue (Decibels::decibelsToGain (pInput->load()));
    outSm.setTargetValue (Decibels::decibelsToGain (pOutput->load()));
    mixSm.setTargetValue (pMix->load());
    driveSm.setTargetValue (pDrive->load());
    toneSm.setTargetValue (pTone->load());
    const float d = driveSm.skip (n);
    const float t = toneSm.skip (n);
    const bool autoOn = pAuto->load() > 0.5f;
    const auto st = stageFor (d);
    g1Sm.setTargetValue (st.g1); g2Sm.setTargetValue (st.g2); biasSm.setTargetValue (st.bias);
    const bool useTube = st.dTube > 0.001f;

    // seco compensado en latencia
    if (dryBuffer.getNumSamples() < n || dryBuffer.getNumChannels() < nch)
        dryBuffer.setSize (2, n, false, false, true);
    for (int c = 0; c < nch; ++c) dryBuffer.copyFrom (c, 0, buffer, c, 0, n);
    {
        dsp::AudioBlock<float> db (dryBuffer.getArrayOfWritePointers(), (size_t) nch, (size_t) n);
        dryDelay.process (dsp::ProcessContextReplacing<float> (db));
    }

    for (int i = 0; i < n; ++i)
    {
        const float g = inSm.getNextValue();
        for (int c = 0; c < nch; ++c) buffer.getWritePointer (c)[i] *= g;
    }
    float msInBlock = 0.0f;
    for (int c = 0; c < nch; ++c) { const float r = buffer.getRMSLevel (c, 0, n); msInBlock += r * r; }
    msInBlock /= (float) nch;

    // ---- filtros: Tone = cuánto agudo entra a la saturación + brillo final
    const float hiDb = 4.0f + 6.0f * t;
    preHigh.setHighShelf  (sr, 4000.0, +hiDb);
    postHigh.setHighShelf (sr, 4000.0, -hiDb);
    preLow.setLowShelf    (sr, 100.0, +3.0 * st.aTape);     // "engorda" graves/medios
    postLow.setLowShelf   (sr, 100.0, -1.5 * st.aTape);
    lowPass.setLowPass1   (sr, 20000.0 - 7500.0 * st.aTape); // suaviza agudos
    toneShelf.setHighShelf (sr, 5000.0, 4.0f * t * jmin (1.0f, std::abs (d) * 2.0f));

    for (int c = 0; c < nch; ++c)
    {
        float* x = buffer.getWritePointer (c);
        for (int i = 0; i < n; ++i) x[i] = preHigh.process (c, preLow.process (c, x[i]));
    }

    {
        dsp::AudioBlock<float> blk (buffer.getArrayOfWritePointers(), (size_t) nch, (size_t) n);
        auto up = oversampler.processSamplesUp (blk);
        const size_t nUp = up.getNumSamples();
        for (size_t i = 0; i < nUp; ++i)
        {
            const float g1 = g1Sm.getNextValue(), g2 = g2Sm.getNextValue(), b = biasSm.getNextValue();
            for (size_t c = 0; c < (size_t) nch; ++c)
            {
                float* x = up.getChannelPointer (c);
                float v = silk::shape (x[i], g1, 0.0f, false);          // cinta: simétrica -> impares
                if (useTube) v = silk::shape (v, g2, b, false);         // tubo: asimétrica -> pares
                x[i] = v;
            }
        }
        oversampler.processSamplesDown (blk);
    }

    for (int c = 0; c < nch; ++c)
    {
        float* x = buffer.getWritePointer (c);
        for (int i = 0; i < n; ++i)
        {
            float s = postLow.process (c, x[i]);
            s = postHigh.process (c, s);
            s = lowPass.process (c, s);
            s = dcBlock.process (c, s);
            x[i] = toneShelf.process (c, s);
        }
    }

    float msOutBlock = 0.0f;
    for (int c = 0; c < nch; ++c) { const float r = buffer.getRMSLevel (c, 0, n); msOutBlock += r * r; }
    msOutBlock /= (float) nch;
    const float a = std::exp (-(float) n / (float) (sr * 0.4));
    msIn  = a * msIn  + (1.0f - a) * msInBlock;
    msOut = a * msOut + (1.0f - a) * msOutBlock;
    float target = 1.0f;
    if (autoOn && msIn > 1.0e-10f && msOut > 1.0e-10f) target = jlimit (0.1f, 8.0f, std::sqrt (msIn / msOut));
    for (int c = 0; c < nch; ++c) buffer.applyGainRamp (c, 0, n, agGain, target);
    agGain = target;

    for (int i = 0; i < n; ++i)
    {
        const float m = mixSm.getNextValue() * st.w;
        const float og = outSm.getNextValue();
        for (int c = 0; c < nch; ++c)
        {
            float* w = buffer.getWritePointer (c);
            float y = (dryBuffer.getReadPointer (c)[i] * (1.0f - m) + w[i] * m) * og;
            if (! std::isfinite (y)) y = 0.0f;
            w[i] = y;
        }
    }
}

AudioProcessorEditor* HeatAudioProcessor::createEditor() { return new HeatEditor (*this); }

void HeatAudioProcessor::getStateInformation (MemoryBlock& dest)
{
    if (auto xml = apvts.copyState().createXml()) copyXmlToBinary (*xml, dest);
}
void HeatAudioProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType())) apvts.replaceState (ValueTree::fromXml (*xml));
}

#ifndef SILKSAT_TEST_BUILD
AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new HeatAudioProcessor(); }
#endif
