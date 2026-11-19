#pragma once
#include "HeatProcessor.h"

class HeatEditor : public juce::AudioProcessorEditor
{
public:
    explicit HeatEditor (HeatAudioProcessor&);
    ~HeatEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using SliderAtt = juce::AudioProcessorValueTreeState::SliderAttachment;
    struct Knob { juce::Slider slider; juce::Label label; std::unique_ptr<SliderAtt> att; };
    void addKnob (Knob&, const juce::String& id, const juce::String& text, int textW);

    HeatAudioProcessor& proc;
    juce::LookAndFeel_V4 laf;
    Knob drive, tone, input, mix, output;
    juce::ToggleButton autoGain { "Auto-gain" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> autoAtt;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HeatEditor)
};
