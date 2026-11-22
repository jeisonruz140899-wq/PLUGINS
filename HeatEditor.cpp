#include "HeatEditor.h"
using namespace juce;

HeatEditor::HeatEditor (HeatAudioProcessor& p) : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&look);
    const Colour soft (0xffffd9b0);
    addKnob (drive,  "drive",  "DRIVE",   skin.accent,  5);
    addKnob (tone,   "tone",   "TONE",    skin.accent2, 3);
    addKnob (input,  "input",  "ENTRADA", soft, 0);
    addKnob (mix,    "mix",    "MEZCLA",  soft, 0);
    addKnob (output, "output", "SALIDA",  soft, 0);
    drive.slider.setDoubleClickReturnValue (true, 0.0);
    tone.slider.setDoubleClickReturnValue (true, 0.0);

    addAndMakeVisible (preset);
    preset.onClick = [this] { setParam ("drive", -0.4f); setParam ("tone", 2.0f / 3.0f); };
    addAndMakeVisible (autoGain);
    autoAtt = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, "auto", autoGain);
    setSize (680, 520);
}

HeatEditor::~HeatEditor() { setLookAndFeel (nullptr); }

void HeatEditor::setParam (const char* id, float plain)
{
    if (auto* par = proc.apvts.getParameter (id))
    {
        par->beginChangeGesture();
        par->setValueNotifyingHost (par->convertTo0to1 (plain));
        par->endChangeGesture();
    }
}

void HeatEditor::addKnob (Knob& k, const String& id, const String& text, Colour colour, int ticks)
{
    k.slider.setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    k.slider.setRotaryParameters (MathConstants<float>::pi * 1.2f, MathConstants<float>::pi * 2.8f, true);
    k.slider.setTextBoxStyle (Slider::TextBoxBelow, false, 90, 22);
    k.slider.setColour (Slider::rotarySliderFillColourId, colour);
    if (ticks > 0) k.slider.getProperties().set ("ticks", ticks);
    addAndMakeVisible (k.slider);
    k.label.setText (text, dontSendNotification);
    k.label.setFont (FontOptions (ticks > 0 ? 15.0f : 12.0f, Font::bold).withKerningFactor (0.2f));
    k.label.setJustificationType (Justification::centred);
    k.label.setColour (Label::textColourId, colour.interpolatedWith (Colours::white, 0.35f));
    addAndMakeVisible (k.label);
    k.att = std::make_unique<SliderAtt> (proc.apvts, id, k.slider);
}

void HeatEditor::paint (Graphics& g)
{
    g.drawImageAt (backdrop, 0, 0);
    dth::drawTitle (g, "SILKHEAT", skin, 26, 16, "D.T.H.   //   CINTA + TUBO", juce::String (juce::CharPointer_UTF8 ("\xE3\x83\x92\xE3\x83\xBC\xE3\x83\x88")));
}

void HeatEditor::resized()
{
    backdrop = Image (Image::ARGB, getWidth(), getHeight(), true);
    { Graphics g (backdrop); dth::paintBackdrop (g, getLocalBounds(), skin); }

    const int W = getWidth();
    preset.setBounds (W - 26 - 250, 26, 250, 30);
    autoGain.setBounds (W - 26 - 130, 66, 130, 22);

    auto area = getLocalBounds().reduced (24, 0);
    area.removeFromTop (100);
    area.removeFromBottom (60);
    auto big = area.removeFromTop (area.getHeight() * 64 / 100);
    auto place = [] (Knob& k, Rectangle<int> r) { k.label.setBounds (r.removeFromTop (22)); k.slider.setBounds (r); };
    place (drive, big.removeFromLeft (big.getWidth() / 2).reduced (10, 0));
    place (tone, big.reduced (10, 0));
    const int cw = area.getWidth() / 3;
    place (input,  area.removeFromLeft (cw).reduced (30, 4));
    place (mix,    area.removeFromLeft (cw).reduced (30, 4));
    place (output, area.reduced (30, 4));
}
