#include "PluginEditor.h"
using namespace juce;

SilkSatEditor::SilkSatEditor (SilkSatAudioProcessor& p) : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&look);
    const Colour lav (0xffb9a2ff);
    addKnob (input,   "input",   "ENTRADA",    lav);
    addKnob (drive,   "drive",   CharPointer_UTF8 ("SATURACI\xC3\x93N"), skin.accent);
    addKnob (soften,  "soften",  "SUAVIZAR",   skin.accent);
    addKnob (warmth,  "warmth",  "CALIDEZ",    skin.accent);
    addKnob (deharsh, "deharsh", "ANTI-FILO",  skin.accent2);
    addKnob (dhfreq,  "dhfreq",  "ZONA FILO",  skin.accent2);
    addKnob (mix,     "mix",     "MEZCLA",     lav);
    addKnob (output,  "output",  "SALIDA",     lav);

    model.addItemList ({ "Tubo", "Discreto", "Transformador", "Cinta" }, 1);
    addAndMakeVisible (model);
    modelAtt = std::make_unique<AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, "model", model);

    addAndMakeVisible (autoGain);
    addAndMakeVisible (listen);
    autoAtt   = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, "auto", autoGain);
    listenAtt = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, "listen", listen);
    setSize (720, 500);
}

SilkSatEditor::~SilkSatEditor() { setLookAndFeel (nullptr); }

void SilkSatEditor::addKnob (Knob& k, const String& id, const String& text, Colour colour)
{
    k.slider.setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    k.slider.setRotaryParameters (MathConstants<float>::pi * 1.2f, MathConstants<float>::pi * 2.8f, true);
    k.slider.setTextBoxStyle (Slider::TextBoxBelow, false, 90, 20);
    k.slider.setColour (Slider::rotarySliderFillColourId, colour);
    addAndMakeVisible (k.slider);
    k.label.setText (text, dontSendNotification);
    k.label.setFont (FontOptions (12.0f, Font::bold).withKerningFactor (0.14f));
    k.label.setJustificationType (Justification::centred);
    k.label.setColour (Label::textColourId, colour.interpolatedWith (Colours::white, 0.35f));
    addAndMakeVisible (k.label);
    k.att = std::make_unique<SliderAtt> (proc.apvts, id, k.slider);
}

void SilkSatEditor::paint (Graphics& g)
{
    g.drawImageAt (backdrop, 0, 0);
    dth::drawTitle (g, "SILKSAT", skin, 26, 16, "D.T.H.   //   VOCAL SMOOTHER", juce::String (juce::CharPointer_UTF8 ("\xE3\x82\xB7\xE3\x83\xAB\xE3\x82\xAF")));
}

void SilkSatEditor::resized()
{
    backdrop = Image (Image::ARGB, getWidth(), getHeight(), true);
    { Graphics g (backdrop); dth::paintBackdrop (g, getLocalBounds(), skin); }

    const int W = getWidth();
    model.setBounds (W - 26 - 180, 24, 180, 30);
    listen.setBounds (W - 26 - 150, 66, 150, 22);
    autoGain.setBounds (W - 26 - 150 - 140, 66, 130, 22);

    auto area = getLocalBounds().reduced (24, 0);
    area.removeFromTop (104);
    area.removeFromBottom (62);
    Knob* all[] = { &input, &drive, &soften, &warmth, &deharsh, &dhfreq, &mix, &output };
    const int cw = area.getWidth() / 4, ch = area.getHeight() / 2;
    for (int i = 0; i < 8; ++i)
    {
        auto cell = Rectangle<int> (area.getX() + (i % 4) * cw, area.getY() + (i / 4) * ch, cw, ch).reduced (8, 2);
        all[i]->label.setBounds (cell.removeFromTop (18));
        all[i]->slider.setBounds (cell);
    }
}
