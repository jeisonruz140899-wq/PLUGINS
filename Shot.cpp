#include "../Source/PluginProcessor.h"
#include "../Source/Heat/HeatProcessor.h"
static void save (juce::AudioProcessor& p, const char* path)
{
    std::unique_ptr<juce::AudioProcessorEditor> e (p.createEditor());
    auto img = e->createComponentSnapshot (e->getLocalBounds(), true, 1.0f);
    juce::File f (path); f.deleteFile();
    juce::FileOutputStream os (f);
    juce::PNGImageFormat().writeImageToStream (img, os);
}
int main()
{
    juce::ScopedJuceInitialiser_GUI init;
    { SilkSatAudioProcessor p; save (p, "/tmp/silksat.png"); }
    { HeatAudioProcessor p;    save (p, "/tmp/silkheat.png"); }
    return 0;
}
