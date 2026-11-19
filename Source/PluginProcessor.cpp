#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace juce;

//==============================================================================
AudioProcessorValueTreeState::ParameterLayout SilkSatAudioProcessor::createLayout()
{
    std::vector<std::unique_ptr<RangedAudioParameter>> p;

    auto pct = AudioParameterFloatAttributes().withStringFromValueFunction (
        [] (float v, int) { return String (roundToInt (v * 100.0f)) + " %"; });
    auto dB = AudioParameterFloatAttributes().withLabel ("dB");
    auto hz = AudioParameterFloatAttributes().withLabel ("Hz");

    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "input", 1 },  "Entrada",
        NormalisableRange<float> (-12.0f, 12.0f, 0.1f), 0.0f, dB));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "drive", 1 },  "Saturación",
        NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.35f, pct));
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "model", 1 }, "Modelo",
        StringArray { "Tubo", "Discreto", "Transformador", "Cinta" }, 0));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "soften", 1 }, "Suavizar",
        NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.5f, pct));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "warmth", 1 }, "Calidez",
        NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.5f, pct));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "deharsh", 1 }, "Anti-filo",
        NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.3f, pct));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "dhfreq", 1 }, "Zona filo",
        NormalisableRange<float> (2000.0f, 10000.0f, 1.0f, 0.5f), 5000.0f, hz));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "mix", 1 },    "Mezcla",
        NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.8f, pct));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "output", 1 }, "Salida",
        NormalisableRange<float> (-12.0f, 12.0f, 0.1f), 0.0f, dB));
    p.push_back (std::make_unique<AudioParameterBool> (ParameterID { "auto", 1 },    "Auto-gain", true));

    return { p.begin(), p.end() };
}

SilkSatAudioProcessor::SilkSatAudioProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  AudioChannelSet::stereo(), true)
                        .withOutput ("Output", AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "SilkSatState", createLayout())
{
    pInput   = apvts.getRawParameterValue ("input");
    pDrive   = apvts.getRawParameterValue ("drive");
    pModel   = apvts.getRawParameterValue ("model");
    pSoften  = apvts.getRawParameterValue ("soften");
    pWarmth  = apvts.getRawParameterValue ("warmth");
    pDeharsh = apvts.getRawParameterValue ("deharsh");
    pDhFreq  = apvts.getRawParameterValue ("dhfreq");
    pMix     = apvts.getRawParameterValue ("mix");
    pOutput  = apvts.getRawParameterValue ("output");
    pAuto    = apvts.getRawParameterValue ("auto");
}

bool SilkSatAudioProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto out = l.getMainOutputChannelSet();
    return (out == AudioChannelSet::mono() || out == AudioChannelSet::stereo())
           && out == l.getMainInputChannelSet();
}

//==============================================================================
static float driveToGain (float d) { return 1.0f + 9.0f * d * d; }

void SilkSatAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
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

    for (auto* f : { &preLow, &preHigh, &postLow, &postHigh, &lowPass, &warmthShelf, &dcBlock })
        f->reset();
    dcBlock.setHighPass1 (sr, 15.0);

    deharsh.prepare (sampleRate);

    const double ramp = 0.04;
    inSm.reset (sampleRate, ramp);      inSm.setCurrentAndTargetValue (Decibels::decibelsToGain (pInput->load()));
    outSm.reset (sampleRate, ramp);     outSm.setCurrentAndTargetValue (Decibels::decibelsToGain (pOutput->load()));
    mixSm.reset (sampleRate, ramp);     mixSm.setCurrentAndTargetValue (pMix->load());
    driveSm.reset (sampleRate * 4.0, ramp); driveSm.setCurrentAndTargetValue (driveToGain (pDrive->load()));
    softenSm.reset (sampleRate, 0.05);  softenSm.setCurrentAndTargetValue (pSoften->load());
    warmthSm.reset (sampleRate, 0.05);  warmthSm.setCurrentAndTargetValue (pWarmth->load());
    deharshSm.reset (sampleRate, 0.05); deharshSm.setCurrentAndTargetValue (pDeharsh->load());

    msIn = msOut = 0.0f;
    agGain = 1.0f;
}

void SilkSatAudioProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer&)
{
    ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    const int nch = jmin (buffer.getNumChannels(), 2);
    if (n == 0 || nch == 0) return;
    for (int c = getTotalNumInputChannels(); c < getTotalNumOutputChannels(); ++c)
        buffer.clear (c, 0, n);

    // ---- parámetros del bloque
    const int model = jlimit (0, 3, (int) pModel->load());
    const auto spec = silk::specFor (model);
    inSm.setTargetValue (Decibels::decibelsToGain (pInput->load()));
    outSm.setTargetValue (Decibels::decibelsToGain (pOutput->load()));
    mixSm.setTargetValue (pMix->load());
    driveSm.setTargetValue (driveToGain (pDrive->load()));
    softenSm.setTargetValue (pSoften->load());
    warmthSm.setTargetValue (pWarmth->load());
    deharshSm.setTargetValue (pDeharsh->load());
    const float soften  = softenSm.skip (n);
    const float warmth  = warmthSm.skip (n);
    const float deharshAmt = deharshSm.skip (n);
    const bool  autoOn  = pAuto->load() > 0.5f;
    const float bias    = spec.bias * (0.5f + warmth);

    // ---- señal seca (compensada en latencia)
    if (dryBuffer.getNumSamples() < n || dryBuffer.getNumChannels() < nch)
        dryBuffer.setSize (2, n, false, false, true);
    for (int c = 0; c < nch; ++c) dryBuffer.copyFrom (c, 0, buffer, c, 0, n);
    {
        dsp::AudioBlock<float> db (dryBuffer.getArrayOfWritePointers(), (size_t) nch, (size_t) n);
        dryDelay.process (dsp::ProcessContextReplacing<float> (db));
    }

    // ---- ganancia de entrada
    for (int i = 0; i < n; ++i)
    {
        const float g = inSm.getNextValue();
        for (int c = 0; c < nch; ++c) buffer.getWritePointer (c)[i] *= g;
    }
    float msInBlock = 0.0f;
    for (int c = 0; c < nch; ++c) { const float r = buffer.getRMSLevel (c, 0, n); msInBlock += r * r; }
    msInBlock /= (float) nch;

    // ---- filtros de énfasis / de-énfasis del modelo
    const float hiDb = soften * 9.0f + spec.hiEmphDb;
    preHigh.setHighShelf  (sr, 4000.0, +hiDb);
    postHigh.setHighShelf (sr, 4000.0, -hiDb);
    preLow.setLowShelf    (sr, 120.0, +spec.lowEmphDb);
    postLow.setLowShelf   (sr, 120.0, -spec.lowEmphDb);
    lowPass.setLowPass1   (sr, spec.lpHz);
    warmthShelf.setLowShelf (sr, 150.0, warmth * 2.5f);

    for (int c = 0; c < nch; ++c)
    {
        float* x = buffer.getWritePointer (c);
        for (int i = 0; i < n; ++i)
            x[i] = preHigh.process (c, preLow.process (c, x[i]));
    }

    // ---- saturación con sobremuestreo 4x (evita aliasing)
    {
        dsp::AudioBlock<float> blk (buffer.getArrayOfWritePointers(), (size_t) nch, (size_t) n);
        auto up = oversampler.processSamplesUp (blk);
        const size_t nUp = up.getNumSamples();
        for (size_t i = 0; i < nUp; ++i)
        {
            const float g = driveSm.getNextValue();
            for (size_t c = 0; c < (size_t) nch; ++c)
            {
                float* x = up.getChannelPointer (c);
                x[i] = silk::shape (x[i], g, bias, spec.algebraic);
            }
        }
        oversampler.processSamplesDown (blk);
    }

    // ---- de-énfasis, techo de agudos, DC y calidez
    for (int c = 0; c < nch; ++c)
    {
        float* x = buffer.getWritePointer (c);
        for (int i = 0; i < n; ++i)
        {
            float s = postLow.process (c, x[i]);
            s = postHigh.process (c, s);
            s = lowPass.process (c, s);
            s = dcBlock.process (c, s);
            s = warmthShelf.process (c, s);
            x[i] = s;
        }
    }

    // ---- reductor de filo dinámico
    deharsh.process (buffer.getArrayOfWritePointers(), nch, n, pDhFreq->load(), deharshAmt);

    // ---- auto-gain (iguala el nivel RMS de lo procesado con el de entrada)
    float msOutBlock = 0.0f;
    for (int c = 0; c < nch; ++c) { const float r = buffer.getRMSLevel (c, 0, n); msOutBlock += r * r; }
    msOutBlock /= (float) nch;
    const float a = std::exp (-(float) n / (float) (sr * 0.4));
    msIn  = a * msIn  + (1.0f - a) * msInBlock;
    msOut = a * msOut + (1.0f - a) * msOutBlock;
    float target = 1.0f;
    if (autoOn && msIn > 1.0e-10f && msOut > 1.0e-10f)
        target = jlimit (0.1f, 8.0f, std::sqrt (msIn / msOut));
    for (int c = 0; c < nch; ++c) buffer.applyGainRamp (c, 0, n, agGain, target);
    agGain = target;

    // ---- mezcla seca/procesada y ganancia de salida
    for (int i = 0; i < n; ++i)
    {
        const float m = mixSm.getNextValue();
        const float og = outSm.getNextValue();
        for (int c = 0; c < nch; ++c)
        {
            float* w = buffer.getWritePointer (c);
            const float d = dryBuffer.getReadPointer (c)[i];
            float y = (d * (1.0f - m) + w[i] * m) * og;
            if (! std::isfinite (y)) y = 0.0f;
            w[i] = y;
        }
    }
}

//==============================================================================
AudioProcessorEditor* SilkSatAudioProcessor::createEditor() { return new SilkSatEditor (*this); }

void SilkSatAudioProcessor::getStateInformation (MemoryBlock& dest)
{
    if (auto xml = apvts.copyState().createXml()) copyXmlToBinary (*xml, dest);
}

void SilkSatAudioProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (ValueTree::fromXml (*xml));
}

#ifndef SILKSAT_TEST_BUILD
AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new SilkSatAudioProcessor(); }
#endif
