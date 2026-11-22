#include "../Source/PluginProcessor.h"
#include <random>
#include <cstdio>

static void setParam (SilkSatAudioProcessor& p, const char* id, float plain)
{
    auto* par = p.apvts.getParameter (id);
    par->setValueNotifyingHost (par->convertTo0to1 (plain));
}

static double bandRatioDb (const std::vector<float>& x, double fs, float freq)
{
    silk::BandPass bp; bp.prepare (fs); bp.set (freq, 1.1f);
    double pb = 0, pt = 0;
    for (size_t i = (size_t) (fs * 0.5); i < x.size(); ++i) { const float b = bp.process (x[i]); pb += b * b; pt += x[i] * x[i]; }
    return 10.0 * std::log10 ((pb + 1e-20) / (pt + 1e-20));
}
static double rmsDb (const std::vector<float>& x)
{
    double s = 0; size_t k = x.size() / 2;
    for (size_t i = k; i < x.size(); ++i) s += x[i] * x[i];
    return 10.0 * std::log10 (s / (double) (x.size() - k) + 1e-20);
}

static std::vector<float> run (SilkSatAudioProcessor& p, const std::vector<float>& in, int blk, bool& bad)
{
    std::vector<float> out (in.size(), 0.0f);
    juce::MidiBuffer midi;
    for (size_t pos = 0; pos + (size_t) blk <= in.size(); pos += (size_t) blk)
    {
        juce::AudioBuffer<float> b (2, blk);
        for (int c = 0; c < 2; ++c) std::copy (in.begin() + (long) pos, in.begin() + (long) pos + blk, b.getWritePointer (c));
        p.processBlock (b, midi);
        for (int i = 0; i < blk; ++i)
        {
            const float v = b.getReadPointer (0)[i];
            if (! std::isfinite (v) || std::abs (v) > 4.0f) bad = true;
            out[pos + (size_t) i] = v;
        }
    }
    return out;
}

int main()
{
    juce::ScopedJuceInitialiser_GUI init;
    const double fs = 48000.0; const int blk = 480;
    const size_t N = (size_t) (fs * 4);

    // Señal "voz dura": armónicos de 180 Hz + banda de ruido a ~4.5 kHz (filo de mic barato)
    std::vector<float> in (N);
    std::mt19937 rng (1); std::normal_distribution<float> nd (0.0f, 1.0f);
    silk::BandPass harsh; harsh.prepare (fs); harsh.set (4500.0f, 1.5f);
    for (size_t i = 0; i < N; ++i)
    {
        const double t = (double) i / fs; double v = 0;
        for (int h = 1; h <= 30; ++h) v += std::sin (2.0 * juce::MathConstants<double>::pi * 180.0 * h * t) / h;
        const float env = 0.6f + 0.4f * (float) std::sin (2.0 * juce::MathConstants<double>::pi * 3.0 * t);
        in[i] = env * (0.10f * (float) v + 0.45f * harsh.process (nd (rng)) * 0.5f);
    }
    std::printf ("ENTRADA: rms %.1f dB, banda 5k/total %.1f dB\n", rmsDb (in), bandRatioDb (in, fs, 5000.0f));

    const char* names[] = { "Tubo", "Discreto", "Transformador", "Cinta" };
    bool anyBad = false;
    for (int m = 0; m < 4; ++m)
    {
        SilkSatAudioProcessor p;
        p.setPlayConfigDetails (2, 2, fs, blk);
        p.prepareToPlay (fs, blk);
        setParam (p, "model", (float) m);
        setParam (p, "mix", 1.0f);
        setParam (p, "drive", 0.5f);
        bool bad = false;
        auto out = run (p, in, blk, bad);
        anyBad |= bad;
        std::printf ("%-14s lat %d | rms %.1f dB | banda 5k/total %.1f dB (delta %+.1f)%s\n",
                     names[m], p.getLatencySamples(), rmsDb (out), bandRatioDb (out, fs, 5000.0f),
                     bandRatioDb (out, fs, 5000.0f) - bandRatioDb (in, fs, 5000.0f), bad ? "  <-- NaN/clip!" : "");
    }

    // Anti-filo solo (saturación 0): cuánto baja la banda
    {
        SilkSatAudioProcessor p; p.setPlayConfigDetails (2, 2, fs, blk); p.prepareToPlay (fs, blk);
        setParam (p, "drive", 0.0f); setParam (p, "soften", 0.0f); setParam (p, "warmth", 0.0f);
        setParam (p, "mix", 1.0f); setParam (p, "deharsh", 0.7f);
        bool bad = false; auto out = run (p, in, blk, bad); anyBad |= bad;
        std::printf ("Solo anti-filo 70%%: rms %.1f dB | banda 5k/total %.1f dB (delta %+.1f)\n",
                     rmsDb (out), bandRatioDb (out, fs, 5000.0f), bandRatioDb (out, fs, 5000.0f) - bandRatioDb (in, fs, 5000.0f));
    }

    // Prueba de nulidad: mezcla 0 => salida = entrada retrasada por la latencia
    {
        SilkSatAudioProcessor p; p.setPlayConfigDetails (2, 2, fs, blk); p.prepareToPlay (fs, blk);
        setParam (p, "mix", 0.0f);
        bool bad = false; auto out = run (p, in, blk, bad);
        const int lat = p.getLatencySamples(); double err = 0;
        for (size_t i = (size_t) (fs * 0.5); i < N; ++i) err = std::max (err, (double) std::abs (out[i] - in[i - (size_t) lat]));
        std::printf ("Null test (mix 0): error max %.2e\n", err);
        anyBad |= err > 1e-5;
    }

    // Seno puro: THD aproximado con drive alto (debe haber armónicos, sin aliasing grave)
    {
        SilkSatAudioProcessor p; p.setPlayConfigDetails (2, 2, fs, blk); p.prepareToPlay (fs, blk);
        setParam (p, "drive", 1.0f); setParam (p, "mix", 1.0f); setParam (p, "deharsh", 0.0f);
        std::vector<float> s (N);
        for (size_t i = 0; i < N; ++i) s[i] = 0.5f * (float) std::sin (2.0 * juce::MathConstants<double>::pi * 7000.0 * (double) i / fs);
        bool bad = false; auto out = run (p, s, blk, bad); anyBad |= bad;
        std::printf ("Seno 7 kHz a drive 100%%: rms %.1f dB (in %.1f) %s\n", rmsDb (out), rmsDb (s), bad ? "FALLO" : "ok");
    }

    std::printf (anyBad ? "RESULTADO: HAY PROBLEMAS\n" : "RESULTADO: OK\n");
    return anyBad ? 1 : 0;
}
