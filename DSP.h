#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <cmath>

// Bloques de DSP de SilkSat. Sin asignaciones de memoria en el hilo de audio.
namespace silk
{
//==============================================================================
// Biquad (RBJ) en forma TDF-II, estado independiente por canal (hasta 2).
struct Biquad
{
    float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
    float z1[2] { 0, 0 }, z2[2] { 0, 0 };

    void reset() { z1[0] = z1[1] = z2[0] = z2[1] = 0; }

    inline float process (int ch, float x) noexcept
    {
        const float y = b0 * x + z1[ch];
        z1[ch] = b1 * x - a1 * y + z2[ch];
        z2[ch] = b2 * x - a2 * y;
        return y;
    }

    void setLowShelf (double fs, double f0, double gainDb, double S = 0.8)
    {
        if (std::abs (gainDb) < 1.0e-4) { identity(); return; }
        const double A = std::pow (10.0, gainDb / 40.0);
        const double w0 = juce::MathConstants<double>::twoPi * juce::jmin (f0, 0.45 * fs) / fs;
        const double cw = std::cos (w0), sw = std::sin (w0);
        const double al = sw / 2.0 * std::sqrt ((A + 1.0 / A) * (1.0 / S - 1.0) + 2.0);
        const double sA = 2.0 * std::sqrt (A) * al;
        set (A * ((A + 1) - (A - 1) * cw + sA),
             2 * A * ((A - 1) - (A + 1) * cw),
             A * ((A + 1) - (A - 1) * cw - sA),
             (A + 1) + (A - 1) * cw + sA,
             -2 * ((A - 1) + (A + 1) * cw),
             (A + 1) + (A - 1) * cw - sA);
    }

    void setHighShelf (double fs, double f0, double gainDb, double S = 0.8)
    {
        if (std::abs (gainDb) < 1.0e-4) { identity(); return; }
        const double A = std::pow (10.0, gainDb / 40.0);
        const double w0 = juce::MathConstants<double>::twoPi * juce::jmin (f0, 0.45 * fs) / fs;
        const double cw = std::cos (w0), sw = std::sin (w0);
        const double al = sw / 2.0 * std::sqrt ((A + 1.0 / A) * (1.0 / S - 1.0) + 2.0);
        const double sA = 2.0 * std::sqrt (A) * al;
        set (A * ((A + 1) + (A - 1) * cw + sA),
             -2 * A * ((A - 1) + (A + 1) * cw),
             A * ((A + 1) + (A - 1) * cw - sA),
             (A + 1) - (A - 1) * cw + sA,
             2 * ((A - 1) - (A + 1) * cw),
             (A + 1) - (A - 1) * cw - sA);
    }

    void setLowPass1 (double fs, double f0)
    {
        const double K = std::tan (juce::MathConstants<double>::pi * juce::jmin (f0, 0.45 * fs) / fs);
        set (K, K, 0, 1 + K, K - 1, 0);
    }

    void setHighPass1 (double fs, double f0)
    {
        const double K = std::tan (juce::MathConstants<double>::pi * f0 / fs);
        set (1, -1, 0, 1 + K, K - 1, 0);
    }

private:
    void identity() { b0 = 1; b1 = b2 = a1 = a2 = 0; }
    void set (double B0, double B1, double B2, double A0, double A1, double A2)
    {
        b0 = (float) (B0 / A0); b1 = (float) (B1 / A0); b2 = (float) (B2 / A0);
        a1 = (float) (A1 / A0); a2 = (float) (A2 / A0);
    }
};

//==============================================================================
// Pasa-banda SVF (TPT) con ganancia unidad en el centro.
struct BandPass
{
    void prepare (double sampleRate) { fs = sampleRate; reset(); }
    void reset() { s1 = s2 = 0.0f; }

    void set (float freq, float q)
    {
        freq = juce::jlimit (20.0f, (float) (0.45 * fs), freq);
        g = std::tan (juce::MathConstants<float>::pi * freq / (float) fs);
        k = 1.0f / q;
        a1 = 1.0f / (1.0f + g * (g + k));
    }

    inline float process (float x) noexcept
    {
        const float hp = (x - (g + k) * s1 - s2) * a1;
        const float bp = g * hp + s1;
        s1 = g * hp + bp;
        const float lp = g * bp + s2;
        s2 = g * bp + lp;
        return bp * k;
    }

    double fs = 44100.0;
    float g = 0.1f, k = 1.0f, a1 = 1.0f, s1 = 0.0f, s2 = 0.0f;
};

//==============================================================================
// Reductor dinámico de "filo" (stereo-linked). Detecta la energía de la banda
// RELATIVA al nivel total (así funciona igual con voces fuertes o bajitas) y
// resta parte de esa banda cuando se pasa de lo normal.
class DeHarsh
{
public:
    void prepare (double sampleRate)
    {
        fs = sampleRate;
        for (auto& b : bp) b.prepare (sampleRate);
        eb = ef = grSm = 0.0f;
        aAtt  = coef (0.0015);
        aRel  = coef (0.050);
        aFull = coef (0.150);
        aGr   = coef (0.004);
        lastFreq = -1.0f;
    }

    void process (float* const* ch, int nch, int n, float freq, float amount, bool listen = false)
    {
        if (freq != lastFreq) { for (auto& b : bp) b.set (freq, 1.1f); lastFreq = freq; }
        if (listen)   // monitor: solo se oye la banda donde trabaja el anti-filo
        {
            for (int i = 0; i < n; ++i)
                for (int c = 0; c < nch; ++c) ch[c][i] = bp[c].process (ch[c][i]);
            return;
        }
        if (amount < 0.001f) { grSm = 0.0f; return; }

        const float slope = 0.25f + 0.60f * amount;   // fracción del exceso que se quita
        const float maxGr = 12.0f * amount;           // dB máx. de reducción
        constexpr float refDb = -14.0f;               // relación banda/total "normal"

        for (int i = 0; i < n; ++i)
        {
            float bpv[2] { 0.0f, 0.0f };
            float bandP = 0.0f, fullP = 0.0f;
            for (int c = 0; c < nch; ++c)
            {
                const float x = ch[c][i];
                bpv[c] = bp[c].process (x);
                bandP += bpv[c] * bpv[c];
                fullP += x * x;
            }
            const float bandL = std::sqrt (bandP / (float) nch);
            const float fullL = std::sqrt (fullP / (float) nch);

            eb += (bandL > eb ? aAtt : aRel) * (bandL - eb);
            ef += aFull * (fullL - ef);

            float target = 0.0f;
            if (ef > 5.0e-4f)
            {
                const float ratioDb = 20.0f * std::log10 ((eb + 1.0e-9f) / (ef + 1.0e-9f));
                const float excess = ratioDb - refDb;
                if (excess > 0.0f) target = juce::jmin (maxGr, excess * slope);
            }
            grSm += aGr * (target - grSm);
            const float g = std::pow (10.0f, -grSm / 20.0f);

            for (int c = 0; c < nch; ++c)
                ch[c][i] -= (1.0f - g) * bpv[c];
        }
    }

private:
    float coef (double seconds) const { return (float) (1.0 - std::exp (-1.0 / (seconds * fs))); }

    double fs = 44100.0;
    BandPass bp[2];
    float eb = 0, ef = 0, grSm = 0, aAtt = 0, aRel = 0, aFull = 0, aGr = 0, lastFreq = -1.0f;
};

//==============================================================================
// Modelos de saturación: todos usan "énfasis -> no linealidad -> de-énfasis".
// El énfasis de agudos hace que los picos duros (el filo) sean lo que más se
// redondea, mientras el cuerpo de la voz queda casi limpio.
struct ModelSpec
{
    float bias;        // asimetría -> armónicos pares ("calidez")
    float lowEmphDb;   // énfasis de graves (transformador/cinta)
    float hiEmphDb;    // énfasis extra de agudos propio del modelo
    float lpHz;        // techo suave de agudos
    bool  algebraic;   // curva x/(1+|x|) en vez de tanh
};

inline ModelSpec specFor (int model)
{
    switch (model)
    {
        case 0:  return { 0.10f, 0.0f, 0.0f, 20000.0f, false }; // Tubo
        case 1:  return { 0.05f, 0.0f, 1.5f, 20000.0f, true  }; // Discreto
        case 2:  return { 0.00f, 6.0f, 0.0f, 19000.0f, false }; // Transformador
        default: return { 0.03f, 2.0f, 3.0f, 15500.0f, false }; // Cinta
    }
}

// Ganancia pequeña-señal = 1: con drive bajo la señal pasa casi intacta.
inline float shape (float x, float g, float bias, bool algebraic) noexcept
{
    if (algebraic)
    {
        const float u = g * (x + bias), u0 = g * bias;
        return (u / (1.0f + std::fabs (u)) - u0 / (1.0f + std::fabs (u0))) / g;
    }
    return (std::tanh (g * (x + bias)) - std::tanh (g * bias)) / g;
}
} // namespace silk
