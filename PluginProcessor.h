#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "DSP.h"

class SilkSatAudioProcessor : public juce::AudioProcessor
{
public:
    SilkSatAudioProcessor();
    ~SilkSatAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "SilkSat"; }
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

    std::atomic<float> *pInput, *pDrive, *pModel, *pSoften, *pWarmth,
                       *pDeharsh, *pDhFreq, *pMix, *pOutput, *pAuto, *pListen;

    double sr = 44100.0;
    static constexpr int oversampleOrder = 2; // 2^2 = 4x

    juce::dsp::Oversampling<float> oversampler { 2, oversampleOrder,
        juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple, true, true };
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::None> dryDelay { 8192 };
    juce::AudioBuffer<float> dryBuffer;

    silk::Biquad preLow, preHigh, postLow, postHigh, lowPass, warmthShelf, dcBlock;
    silk::DeHarsh deharsh;

    juce::SmoothedValue<float> inSm, outSm, mixSm, driveSm, softenSm, warmthSm, deharshSm;
    float msIn = 0.0f, msOut = 0.0f, agGain = 1.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SilkSatAudioProcessor)
};
