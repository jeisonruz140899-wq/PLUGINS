#include "../Source/Heat/HeatProcessor.h"
#include <cstdio>

static void setParam (HeatAudioProcessor& p, const char* id, float v)
{ auto* par = p.apvts.getParameter (id); par->setValueNotifyingHost (par->convertTo0to1 (v)); }

static double goertzelDb (const std::vector<float>& x, double fs, double f, size_t start)
{
    double re = 0, im = 0; const size_t N = x.size() - start;
    for (size_t i = 0; i < N; ++i) { const double w = 2 * juce::MathConstants<double>::pi * f * (double) i / fs;
        const double win = 0.5 - 0.5 * std::cos (2 * juce::MathConstants<double>::pi * (double) i / (double) N);
        re += win * x[start + i] * std::cos (w); im += win * x[start + i] * std::sin (w); }
    return 20 * std::log10 (2 * std::sqrt (re * re + im * im) / (double) N * 2.0 + 1e-12);
}

int main()
{
    juce::ScopedJuceInitialiser_GUI init;
    const double fs = 48000; const int blk = 480; const size_t N = (size_t) fs * 3;
    std::vector<float> in (N);
    for (size_t i = 0; i < N; ++i) in[i] = 0.3f * (float) std::sin (2 * juce::MathConstants<double>::pi * 1000.0 * (double) i / fs);

    bool bad = false;
    std::printf ("Seno 1 kHz (-10 dBFS). Armonicos en dB relativos al fundamental:\n");
    std::printf ("%8s | %7s %7s %7s %7s %7s | lat\n", "drive", "H2", "H3", "H4", "H5", "H7");
    for (float d : { -1.0f, -0.5f, 0.0f, 0.5f, 1.0f })
    {
        HeatAudioProcessor p; p.setPlayConfigDetails (2, 2, fs, blk); p.prepareToPlay (fs, blk);
        setParam (p, "drive", d); setParam (p, "auto", 0.0f);
        std::vector<float> out (N); juce::MidiBuffer midi;
        for (size_t pos = 0; pos + blk <= N; pos += blk)
        {
            juce::AudioBuffer<float> b (2, blk);
            for (int c = 0; c < 2; ++c) std::copy (in.begin() + (long) pos, in.begin() + (long) pos + blk, b.getWritePointer (c));
            p.processBlock (b, midi);
            for (int i = 0; i < blk; ++i) { const float v = b.getReadPointer (0)[i]; if (! std::isfinite (v) || std::abs (v) > 4) bad = true; out[pos + (size_t) i] = v; }
        }
        const size_t s0 = (size_t) fs;
        const double h1 = goertzelDb (out, fs, 1000, s0);
        auto rel = [&] (int h) { return goertzelDb (out, fs, 1000.0 * h, s0) - h1; };
        std::printf ("%8.1f | %7.1f %7.1f %7.1f %7.1f %7.1f | %d\n", d * 5, rel (2), rel (3), rel (4), rel (5), rel (7), p.getLatencySamples());
        if (d == 0.0f) { double err = 0; const int lat = p.getLatencySamples();
            for (size_t i = s0; i < N; ++i) err = std::max (err, (double) std::abs (out[i] - in[i - (size_t) lat]));
            std::printf ("         neutro: error max vs entrada retrasada = %.2e\n", err); if (err > 1e-5) bad = true; }
    }
    std::printf (bad ? "RESULTADO: HAY PROBLEMAS\n" : "RESULTADO: OK\n");
    return bad ? 1 : 0;
}
