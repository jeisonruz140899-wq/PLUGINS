#pragma once
#include "PluginProcessor.h"
#include "DthLook.h"

class SilkSatEditor : public juce::AudioProcessorEditor
{
public:
    explicit SilkSatEditor (SilkSatAudioProcessor&);
    ~SilkSatEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using SliderAtt = juce::AudioProcessorValueTreeState::SliderAttachment;
    struct Knob { juce::Slider slider; juce::Label label; std::unique_ptr<SliderAtt> att; };
    void addKnob (Knob&, const juce::String& id, const juce::String& text, juce::Colour colour);

    SilkSatAudioProcessor& proc;
    dth::Skin skin = dth::violetNight();
    dth::Look look { skin };
    juce::Image backdrop;
    Knob input, drive, soften, warmth, deharsh, dhfreq, mix, output;
    juce::ComboBox model;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> modelAtt;
    juce::ToggleButton autoGain { "AUTO-GAIN" }, listen { "ESCUCHAR FILO" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> autoAtt, listenAtt;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SilkSatEditor)
};
