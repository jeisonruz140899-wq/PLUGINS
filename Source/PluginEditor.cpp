#include "PluginEditor.h"
using namespace juce;

static const Colour kBg   (0xff15161a);
static const Colour kAmber (0xffe8a33d);

SilkSatEditor::SilkSatEditor (SilkSatAudioProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    laf.setColour (Slider::rotarySliderFillColourId, kAmber);
    laf.setColour (Slider::rotarySliderOutlineColourId, Colour (0xff3a3c44));
    laf.setColour (Slider::thumbColourId, Colours::white);
    laf.setColour (Slider::textBoxTextColourId, Colours::white);
    laf.setColour (Slider::textBoxOutlineColourId, Colours::transparentBlack);
    laf.setColour (ComboBox::backgroundColourId, Colour (0xff23252b));
    laf.setColour (ToggleButton::tickColourId, kAmber);
    setLookAndFeel (&laf);

    addKnob (input,   "input",   "Entrada");
    addKnob (drive,   "drive",   "Saturación");
    addKnob (soften,  "soften",  "Suavizar");
    addKnob (warmth,  "warmth",  "Calidez");
    addKnob (deharsh, "deharsh", "Anti-filo");
    addKnob (dhfreq,  "dhfreq",  "Zona filo");
    addKnob (mix,     "mix",     "Mezcla");
    addKnob (output,  "output",  "Salida");

    model.addItemList ({ "Tubo", "Discreto", "Transformador", "Cinta" }, 1);
    addAndMakeVisible (model);
    modelAtt = std::make_unique<AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, "model", model);

    addAndMakeVisible (autoGain);
    autoAtt = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, "auto", autoGain);

    setSize (600, 400);
}

SilkSatEditor::~SilkSatEditor() { setLookAndFeel (nullptr); }

void SilkSatEditor::addKnob (Knob& k, const String& id, const String& text)
{
    k.slider.setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    k.slider.setTextBoxStyle (Slider::TextBoxBelow, false, 80, 18);
    addAndMakeVisible (k.slider);
    k.label.setText (text, dontSendNotification);
    k.label.setJustificationType (Justification::centred);
    k.label.setColour (Label::textColourId, Colours::lightgrey);
    addAndMakeVisible (k.label);
    k.att = std::make_unique<SliderAtt> (proc.apvts, id, k.slider);
}

void SilkSatEditor::paint (Graphics& g)
{
    g.fillAll (kBg);
    g.setColour (kAmber);
    g.setFont (FontOptions (26.0f, Font::bold));
    g.drawText ("SilkSat", 20, 10, 200, 36, Justification::centredLeft);
    g.setColour (Colours::grey);
    g.setFont (FontOptions (12.0f));
    g.drawText ("suavizador de voces", 22, 42, 200, 16, Justification::centredLeft);
}

void SilkSatEditor::resized()
{
    auto area = getLocalBounds().reduced (16);
    auto header = area.removeFromTop (56);
    autoGain.setBounds (header.removeFromRight (110).withSizeKeepingCentre (110, 28));
    model.setBounds (header.removeFromRight (160).withSizeKeepingCentre (150, 28));

    Knob* all[] = { &input, &drive, &soften, &warmth, &deharsh, &dhfreq, &mix, &output };
    const int cw = area.getWidth() / 4, ch = area.getHeight() / 2;
    for (int i = 0; i < 8; ++i)
    {
        auto cell = Rectangle<int> (area.getX() + (i % 4) * cw, area.getY() + (i / 4) * ch, cw, ch).reduced (6);
        all[i]->label.setBounds (cell.removeFromTop (20));
        all[i]->slider.setBounds (cell);
    }
}
