#include "HeatEditor.h"
using namespace juce;
static const Colour kBg (0xff1a1412), kOrange (0xffff7a2f);

HeatEditor::HeatEditor (HeatAudioProcessor& p) : AudioProcessorEditor (&p), proc (p)
{
    laf.setColour (Slider::rotarySliderFillColourId, kOrange);
    laf.setColour (Slider::rotarySliderOutlineColourId, Colour (0xff3d322c));
    laf.setColour (Slider::thumbColourId, Colours::white);
    laf.setColour (Slider::textBoxTextColourId, Colours::white);
    laf.setColour (Slider::textBoxOutlineColourId, Colours::transparentBlack);
    laf.setColour (ToggleButton::tickColourId, kOrange);
    setLookAndFeel (&laf);

    addKnob (drive,  "drive",  "DRIVE  (cinta  <  0  >  cinta+tubo)", 80);
    addKnob (tone,   "tone",   "TONE", 80);
    addKnob (input,  "input",  "Entrada", 70);
    addKnob (mix,    "mix",    "Mezcla", 70);
    addKnob (output, "output", "Salida", 70);
    drive.slider.setDoubleClickReturnValue (true, 0.0);
    tone.slider.setDoubleClickReturnValue (true, 0.0);

    addAndMakeVisible (autoGain);
    autoAtt = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, "auto", autoGain);
    setSize (560, 380);
}
HeatEditor::~HeatEditor() { setLookAndFeel (nullptr); }

void HeatEditor::addKnob (Knob& k, const String& id, const String& text, int textW)
{
    k.slider.setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    k.slider.setTextBoxStyle (Slider::TextBoxBelow, false, textW, 18);
    addAndMakeVisible (k.slider);
    k.label.setText (text, dontSendNotification);
    k.label.setJustificationType (Justification::centred);
    k.label.setColour (Label::textColourId, Colours::lightgrey);
    addAndMakeVisible (k.label);
    k.att = std::make_unique<SliderAtt> (proc.apvts, id, k.slider);
}

void HeatEditor::paint (Graphics& g)
{
    g.fillAll (kBg);
    g.setColour (kOrange);
    g.setFont (FontOptions (26.0f, Font::bold));
    g.drawText ("SilkHeat", 20, 10, 200, 36, Justification::centredLeft);
    g.setColour (Colours::grey);
    g.setFont (FontOptions (12.0f));
    g.drawText ("calor analógico: cinta + tubo", 22, 42, 240, 16, Justification::centredLeft);
}

void HeatEditor::resized()
{
    auto area = getLocalBounds().reduced (16);
    auto header = area.removeFromTop (56);
    autoGain.setBounds (header.removeFromRight (110).withSizeKeepingCentre (110, 28));

    auto big = area.removeFromTop (area.getHeight() * 62 / 100);
    auto place = [] (Knob& k, Rectangle<int> r) { k.label.setBounds (r.removeFromTop (20)); k.slider.setBounds (r); };
    place (drive, big.removeFromLeft (big.getWidth() / 2).reduced (8));
    place (tone, big.reduced (8));
    const int cw = area.getWidth() / 3;
    place (input,  area.removeFromLeft (cw).reduced (6));
    place (mix,    area.removeFromLeft (cw).reduced (6));
    place (output, area.reduced (6));
}
