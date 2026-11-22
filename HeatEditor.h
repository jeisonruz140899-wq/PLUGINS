#pragma once
#include "HeatProcessor.h"
#include "../DthLook.h"

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
    void addKnob (Knob&, const juce::String& id, const juce::String& text, juce::Colour colour, int ticks);
    void setParam (const char* id, float plain);

    HeatAudioProcessor& proc;
    dth::Skin skin = dth::emberNight();
    dth::Look look { skin };
    juce::Image backdrop;
    Knob drive, tone, input, mix, output;
    juce::TextButton preset { "PRESET   DRIVE -2  /  TONE +2" };
    juce::ToggleButton autoGain { "AUTO-GAIN" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> autoAtt;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HeatEditor)
};
