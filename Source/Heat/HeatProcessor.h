#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "../DSP.h"

// SilkHeat: emulación por comportamiento del HEAT de Pro Tools (Drive bipolar + Tone).
//  Drive al centro  = neutro (sin armónicos)
//  Drive a la izq.  = cinta: armónicos impares (3º, 5º...), engorda graves/medios, suaviza transitorios agudos
//  Drive a la der.  = cinta + tubo: suma armónicos pares (triodo) -> más crujiente y vivo
class HeatAudioProcessor : public juce::AudioProcessor
{
public:
    HeatAudioProcessor();
    ~HeatAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "SilkHeat"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::AudioProcessorValueTreeState apvts;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    std::atomic<float> *pInput, *pDrive, *pTone, *pMix, *pOutput, *pAuto;
    double sr = 44100.0;

    juce::dsp::Oversampling<float> oversampler { 2, 2,
        juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple, true, true };
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::None> dryDelay { 8192 };
    juce::AudioBuffer<float> dryBuffer;

    silk::Biquad preLow, preHigh, postLow, postHigh, lowPass, toneShelf, dcBlock;
    juce::SmoothedValue<float> inSm, outSm, mixSm, driveSm, toneSm, g1Sm, g2Sm, biasSm;
    float msIn = 0.0f, msOut = 0.0f, agGain = 1.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HeatAudioProcessor)
};
