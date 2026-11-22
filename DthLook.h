#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_graphics/juce_graphics.h>

// Estética D.T.H.: noche cinematográfica, neón de trap, pantallas de manga, ciudad anime.
namespace dth
{
using namespace juce;

struct Skin { Colour bgTop, bgBottom, accent, accent2, text, dim; };

inline Skin violetNight() { return { Colour (0xff09071a), Colour (0xff231040), Colour (0xffff3d9a), Colour (0xff35e6ff), Colour (0xffeae4ff), Colour (0xff8379b5) }; }
inline Skin emberNight()  { return { Colour (0xff0c0608), Colour (0xff321019), Colour (0xffff5a36), Colour (0xffffc15a), Colour (0xfffff0e6), Colour (0xffa57b74) }; }

//==============================================================================
class Look : public LookAndFeel_V4
{
public:
    explicit Look (const Skin& sk) : skin (sk)
    {
        setColour (Slider::textBoxTextColourId, skin.text);
        setColour (Slider::textBoxOutlineColourId, Colours::transparentBlack);
        setColour (Slider::textBoxBackgroundColourId, Colours::transparentBlack);
        setColour (Slider::textBoxHighlightColourId, skin.accent.withAlpha (0.4f));
        setColour (Label::textColourId, skin.text);
        setColour (Label::outlineColourId, skin.accent2.withAlpha (0.25f));
        setColour (Label::backgroundColourId, Colour (0xff0d0919).withAlpha (0.6f));
        setColour (ComboBox::backgroundColourId, Colour (0xff120d24));
        setColour (ComboBox::outlineColourId, skin.accent.withAlpha (0.7f));
        setColour (ComboBox::textColourId, skin.text);
        setColour (ComboBox::arrowColourId, skin.accent);
        setColour (PopupMenu::backgroundColourId, Colour (0xff120d24));
        setColour (PopupMenu::textColourId, skin.text);
        setColour (PopupMenu::highlightedBackgroundColourId, skin.accent.withAlpha (0.35f));
        setColour (PopupMenu::highlightedTextColourId, Colours::white);
        setColour (TextButton::buttonColourId, Colour (0xff120d24));
        setColour (TextButton::textColourOffId, skin.text);
        setColour (TextButton::textColourOnId, skin.text);
    }

    void drawRotarySlider (Graphics& g, int x, int y, int w, int h, float pos,
                           float a0, float a1, Slider& s) override
    {
        const auto bounds = Rectangle<int> (x, y, w, h).toFloat();
        const int ticks = (int) s.getProperties().getWithDefault ("ticks", 0);
        const float margin = ticks > 0 ? 20.0f : 5.0f;
        const float R = jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f - margin;
        const auto c = bounds.getCentre();
        const Colour acc = s.findColour (Slider::rotarySliderFillColourId);
        const float angle = a0 + pos * (a1 - a0);
        const float zeroAngle = a0 + 0.5f * (a1 - a0);
        const bool bipolar = s.getMinimum() < 0.0 && s.getMaximum() > 0.0;

        // pista
        Path track; track.addCentredArc (c.x, c.y, R, R, 0.0f, a0, a1, true);
        g.setColour (skin.dim.withAlpha (0.30f));
        g.strokePath (track, PathStrokeType (3.0f, PathStrokeType::curved, PathStrokeType::rounded));

        // arco de valor con resplandor
        const float from = bipolar ? zeroAngle : a0;
        if (std::abs (angle - from) > 0.002f)
        {
            Path val; val.addCentredArc (c.x, c.y, R, R, 0.0f, jmin (from, angle), jmax (from, angle), true);
            const float widths[] = { 12.0f, 8.0f, 5.0f, 3.0f };
            const float alphas[] = { 0.06f, 0.11f, 0.22f, 1.0f };
            for (int i = 0; i < 4; ++i)
            {
                g.setColour (acc.withAlpha (alphas[i]));
                g.strokePath (val, PathStrokeType (widths[i], PathStrokeType::curved, PathStrokeType::rounded));
            }
        }

        // marcas
        if (ticks > 0)
        {
            const int cur = (int) std::round (s.getValue() * ticks);
            g.setFont (FontOptions (11.0f, Font::bold));
            for (int i = -ticks; i <= ticks; ++i)
            {
                const float ang = zeroAngle + ((float) i / (float) ticks) * 0.5f * (a1 - a0);
                const float rr = R + 13.0f;
                const Point<float> p (c.x + rr * std::sin (ang), c.y - rr * std::cos (ang));
                g.setColour (i == cur ? acc : (i == 0 ? skin.text : skin.dim));
                g.drawText ((i > 0 ? "+" : "") + String (i), Rectangle<float> (26.0f, 14.0f).withCentre (p),
                            Justification::centred);
            }
        }
        else
        {
            g.setColour (skin.dim.withAlpha (0.5f));
            for (int i = 0; i <= 10; ++i)
            {
                const float ang = a0 + (a1 - a0) * (float) i / 10.0f;
                const float rr = R + 8.0f;
                g.fillEllipse (c.x + rr * std::sin (ang) - 1.0f, c.y - rr * std::cos (ang) - 1.0f, 2.0f, 2.0f);
            }
        }

        // cuerpo
        const float br = R - 9.0f;
        g.setGradientFill (ColourGradient (Colour (0xff2c2450), c.x, c.y - br, Colour (0xff090614), c.x, c.y + br, false));
        g.fillEllipse (c.x - br, c.y - br, br * 2.0f, br * 2.0f);
        g.setColour (skin.accent2.withAlpha (0.28f));
        g.drawEllipse (c.x - br, c.y - br, br * 2.0f, br * 2.0f, 1.2f);
        g.setColour (Colours::white.withAlpha (0.05f));
        g.drawEllipse (c.x - br + 4.0f, c.y - br + 4.0f, (br - 4.0f) * 2.0f, (br - 4.0f) * 2.0f, 1.0f);

        // aguja
        const Point<float> d (std::sin (angle), -std::cos (angle));
        const Point<float> p0 = c + d * (br * 0.32f), p1 = c + d * (br * 0.88f);
        g.setColour (acc.withAlpha (0.45f));
        g.drawLine (Line<float> (p0, p1), 6.0f);
        g.setColour (Colours::white);
        g.drawLine (Line<float> (p0, p1), 2.6f);
    }

    void drawToggleButton (Graphics& g, ToggleButton& b, bool, bool) override
    {
        const auto r = b.getLocalBounds().toFloat();
        const bool on = b.getToggleState();
        const float pw = 34.0f, ph = 16.0f;
        const Rectangle<float> pill (r.getX() + 1.0f, r.getCentreY() - ph * 0.5f, pw, ph);
        if (on) { g.setColour (skin.accent.withAlpha (0.25f)); g.fillRoundedRectangle (pill.expanded (3.0f), ph); }
        g.setColour (on ? skin.accent : Colour (0xff2a2347));
        g.fillRoundedRectangle (pill, ph * 0.5f);
        g.setColour (Colours::white);
        g.fillEllipse (on ? pill.getRight() - ph + 2.0f : pill.getX() + 2.0f, pill.getY() + 2.0f, ph - 4.0f, ph - 4.0f);
        g.setColour (on ? skin.text : skin.dim);
        g.setFont (FontOptions (12.0f, Font::bold).withKerningFactor (0.06f));
        g.drawText (b.getButtonText(), r.withTrimmedLeft (pw + 9.0f), Justification::centredLeft);
    }

    void drawComboBox (Graphics& g, int w, int h, bool, int, int, int, int, ComboBox& box) override
    {
        const auto r = Rectangle<float> (0, 0, (float) w, (float) h).reduced (0.5f);
        g.setColour (Colour (0xff120d24)); g.fillRoundedRectangle (r, 6.0f);
        g.setColour (skin.accent.withAlpha (0.75f)); g.drawRoundedRectangle (r, 6.0f, 1.2f);
        Path arrow; const float ax = (float) w - 18.0f, ay = (float) h * 0.5f;
        arrow.addTriangle (ax, ay - 3.0f, ax + 9.0f, ay - 3.0f, ax + 4.5f, ay + 3.5f);
        g.setColour (box.findColour (ComboBox::arrowColourId)); g.fillPath (arrow);
    }

    Font getComboBoxFont (ComboBox&) override { return Font (FontOptions (14.0f, Font::bold)); }

    void drawButtonBackground (Graphics& g, Button& b, const Colour&, bool over, bool down) override
    {
        const auto r = b.getLocalBounds().toFloat().reduced (0.5f);
        g.setColour (Colour (0xff120d24).interpolatedWith (skin.accent, down ? 0.35f : (over ? 0.18f : 0.0f)));
        g.fillRoundedRectangle (r, 6.0f);
        g.setColour (skin.accent2.withAlpha (0.8f));
        g.drawRoundedRectangle (r, 6.0f, 1.2f);
    }

    Skin skin;
};

//==============================================================================
inline void paintBackdrop (Graphics& g, Rectangle<int> b, const Skin& s)
{
    const auto bf = b.toFloat();
    const float W = bf.getWidth(), H = bf.getHeight();
    g.setGradientFill (ColourGradient (s.bgTop, 0, 0, s.bgBottom, 0, H, false));
    g.fillRect (bf);

    // luna con cortes (estilo synthwave / anime nocturno)
    const float mr = jmin (W, H) * 0.36f; const Point<float> mc (W * 0.5f, H * 0.50f);
    g.setGradientFill (ColourGradient (s.accent2.withAlpha (0.13f), mc.x, mc.y - mr, s.accent.withAlpha (0.05f), mc.x, mc.y + mr, false));
    g.fillEllipse (mc.x - mr, mc.y - mr, mr * 2.0f, mr * 2.0f);
    {
        Graphics::ScopedSaveState ss (g);
        Path p; p.addEllipse (mc.x - mr, mc.y - mr, mr * 2.0f, mr * 2.0f);
        g.reduceClipRegion (p);
        for (int i = 0; i < 6; ++i)
        {
            const float y = mc.y + mr * 0.10f + (float) i * mr * 0.15f, th = 2.0f + (float) i * 2.2f;
            g.setColour (s.bgTop.interpolatedWith (s.bgBottom, y / H));
            g.fillRect (mc.x - mr, y, mr * 2.0f, th);
        }
    }
    g.setColour (s.accent2.withAlpha (0.14f));
    g.drawEllipse (mc.x - mr, mc.y - mr, mr * 2.0f, mr * 2.0f, 1.0f);

    // trama de puntos tipo manga (esquina inferior izquierda)
    const float baseY = H - 52.0f;
    for (int row = 0; row < 26; ++row)
        for (int col = 0; col < 40; ++col)
        {
            const float px = (float) col * 9.0f + (row & 1 ? 4.5f : 0.0f), py = baseY - (float) row * 8.0f;
            const float dist = std::sqrt (px * px + (baseY - py) * (baseY - py)) / 230.0f;
            const float rad = 3.3f * (1.0f - dist);
            if (rad > 0.4f) { g.setColour (s.accent.withAlpha (0.20f)); g.fillEllipse (px - rad, py - rad, rad * 2.0f, rad * 2.0f); }
        }

    // ciudad al fondo
    Random rnd (7);
    float bx = 0.0f;
    while (bx < W)
    {
        const float bw = 14.0f + rnd.nextFloat() * 22.0f, bh = 10.0f + rnd.nextFloat() * 30.0f;
        const float by = H - 8.0f - bh;
        g.setColour (Colour (0xff06040d).withAlpha (0.92f)); g.fillRect (bx, by, bw, bh);
        for (float wy = by + 4.0f; wy < H - 12.0f; wy += 6.0f)
            for (float wx = bx + 3.0f; wx < bx + bw - 3.0f; wx += 5.0f)
                if (rnd.nextFloat() > 0.72f)
                { g.setColour ((rnd.nextFloat() > 0.8f ? s.accent : s.accent2).withAlpha (0.55f)); g.fillRect (wx, wy, 2.0f, 2.0f); }
        bx += bw + 1.0f;
    }

    // líneas de escaneo + viñeta + barras de cine
    g.setColour (Colours::black.withAlpha (0.05f));
    for (float yy = 0.0f; yy < H; yy += 3.0f) g.fillRect (0.0f, yy, W, 1.0f);
    g.setGradientFill (ColourGradient (Colours::transparentBlack, W * 0.5f, H * 0.5f,
                                       Colours::black.withAlpha (0.55f), 0.0f, 0.0f, true));
    g.fillRect (bf);
    g.setColour (Colours::black); g.fillRect (0.0f, 0.0f, W, 8.0f); g.fillRect (0.0f, H - 8.0f, W, 8.0f);
    g.setColour (s.accent.withAlpha (0.55f)); g.fillRect (0.0f, 8.0f, W, 1.0f);
}

inline void drawTitle (Graphics& g, const String& text, const Skin& s, int x, int y, const String& sub, const String& katakana)
{
    const Font f (FontOptions (40.0f, Font::bold).withKerningFactor (0.10f));
    g.setFont (f);
    const Rectangle<int> r (x, y, 420, 46);
    g.setColour (s.accent2.withAlpha (0.85f)); g.drawText (text, r.translated (-2, 0), Justification::centredLeft);
    g.setColour (s.accent.withAlpha (0.85f));  g.drawText (text, r.translated (2, 0),  Justification::centredLeft);
    g.setColour (Colours::white);              g.drawText (text, r, Justification::centredLeft);

    GlyphArrangement ga; ga.addLineOfText (f, text, 0.0f, 0.0f);
    const int tw = (int) ga.getBoundingBox (0, -1, true).getWidth();

    g.setFont (FontOptions ("Yu Gothic UI", 17.0f, Font::bold));
    g.setColour (s.accent2.withAlpha (0.9f));
    g.drawText (katakana, x + tw + 16, y + 8, 120, 24, Justification::centredLeft);

    g.setFont (FontOptions (11.5f, Font::bold).withKerningFactor (0.28f));
    g.setColour (s.dim);
    g.drawText (sub, x + 3, y + 46, 420, 16, Justification::centredLeft);
}
} // namespace dth
