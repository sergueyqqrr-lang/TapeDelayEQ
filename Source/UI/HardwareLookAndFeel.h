#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

/**
    Look & feel "hardware" para dar sensación de rack analógico:
    knobs metálicos con relieve y sombra, botones tipo interruptor con
    LED, y faders verticales con riel hundido.
*/
class HardwareLookAndFeel : public juce::LookAndFeel_V4
{
public:
    HardwareLookAndFeel()
    {
        setColour (juce::Slider::textBoxTextColourId, juce::Colour (0xffd8d8d8));
        setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        setColour (juce::Label::textColourId, juce::Colour (0xffbfbfbf));
    }

    // ---------------------------------------------------------------
    // Knobs rotativos (Time, Feedback, Mix, Wow, Flutter, Sat, gains...)
    // ---------------------------------------------------------------
    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                            float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                            juce::Slider&) override
    {
        auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height).reduced (4.0f);
        auto radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
        auto centre = bounds.getCentre();
        const float angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

        // Sombra proyectada
        {
            juce::Path shadowCircle;
            shadowCircle.addEllipse (centre.x - radius, centre.y - radius + 3.0f, radius * 2.0f, radius * 2.0f);
            g.setColour (juce::Colours::black.withAlpha (0.5f));
            g.fillPath (shadowCircle);
        }

        // Cuerpo metálico del knob (gradiente radial simulando luz superior-izquierda)
        {
            juce::ColourGradient grad (juce::Colour (0xff5a5d63), centre.x - radius * 0.4f, centre.y - radius * 0.6f,
                                        juce::Colour (0xff1c1d20), centre.x + radius * 0.6f, centre.y + radius * 0.8f,
                                        true);
            grad.addColour (0.5, juce::Colour (0xff37393e));
            g.setGradientFill (grad);
            g.fillEllipse (centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f);
        }

        // Anillo biselado exterior
        g.setColour (juce::Colour (0xff0c0d0e));
        g.drawEllipse (centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f, 1.4f);
        g.setColour (juce::Colours::white.withAlpha (0.08f));
        g.drawEllipse (centre.x - radius + 1.5f, centre.y - radius + 1.5f, radius * 2.0f - 3.0f, radius * 2.0f - 3.0f, 1.0f);

        // Arco de valor (naranja cálido, look "vintage")
        {
            juce::Path arc;
            arc.addCentredArc (centre.x, centre.y, radius + 4.0f, radius + 4.0f, 0.0f,
                                rotaryStartAngle, angle, true);
            g.setColour (juce::Colour (0xffd8863a));
            g.strokePath (arc, juce::PathStrokeType (2.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

            juce::Path track;
            track.addCentredArc (centre.x, centre.y, radius + 4.0f, radius + 4.0f, 0.0f,
                                  rotaryStartAngle, rotaryEndAngle, true);
            g.setColour (juce::Colours::black.withAlpha (0.4f));
            g.strokePath (track, juce::PathStrokeType (1.2f));
        }

        // Indicador (puntero) con pequeño highlight, como un tornillo con marca
        {
            juce::Path pointer;
            const float pointerLength = radius * 0.62f;
            const float pointerThickness = 3.0f;
            pointer.addRoundedRectangle (-pointerThickness * 0.5f, -radius * 0.92f,
                                          pointerThickness, pointerLength, 1.5f);
            pointer.applyTransform (juce::AffineTransform::rotation (angle).translated (centre.x, centre.y));
            g.setColour (juce::Colour (0xfff2e8da));
            g.fillPath (pointer);
        }

        // Punto de luz central (reflejo)
        g.setColour (juce::Colours::white.withAlpha (0.15f));
        g.fillEllipse (centre.x - radius * 0.35f, centre.y - radius * 0.55f, radius * 0.5f, radius * 0.35f);
    }

    // ---------------------------------------------------------------
    // Faders verticales (bandas del EQ)
    // ---------------------------------------------------------------
    void drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                            float sliderPos, float /*minSliderPos*/, float /*maxSliderPos*/,
                            const juce::Slider::SliderStyle style, juce::Slider& slider) override
    {
        if (style != juce::Slider::LinearVertical)
        {
            LookAndFeel_V4::drawLinearSlider (g, x, y, width, height, sliderPos, 0, 0, style, slider);
            return;
        }

        auto trackBounds = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height);
        const float railWidth = 6.0f;
        auto rail = trackBounds.withSizeKeepingCentre (railWidth, trackBounds.getHeight());

        // Riel hundido
        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.fillRoundedRectangle (rail, railWidth * 0.5f);
        g.setColour (juce::Colours::white.withAlpha (0.06f));
        g.drawRoundedRectangle (rail.translated (0, 1.0f), railWidth * 0.5f, 1.0f);

        // Línea de cero (centro, 0 dB)
        const float centreY = trackBounds.getCentreY();
        g.setColour (juce::Colour (0xff555555));
        g.drawLine (rail.getX() - 4.0f, centreY, rail.getRight() + 4.0f, centreY, 1.0f);

        // Cuerpo del fader (cap metálico)
        const float capWidth = (float) width - 2.0f;
        const float capHeight = 16.0f;
        juce::Rectangle<float> cap (trackBounds.getX() + 1.0f, sliderPos - capHeight * 0.5f, capWidth, capHeight);

        g.setColour (juce::Colours::black.withAlpha (0.5f));
        g.fillRoundedRectangle (cap.translated (0, 2.0f), 3.0f);

        juce::ColourGradient capGrad (juce::Colour (0xff6a6d73), cap.getX(), cap.getY(),
                                       juce::Colour (0xff232427), cap.getX(), cap.getBottom(), false);
        g.setGradientFill (capGrad);
        g.fillRoundedRectangle (cap, 3.0f);

        g.setColour (juce::Colour (0xffd8863a));
        g.fillRoundedRectangle (cap.withSizeKeepingCentre (capWidth, 2.0f), 1.0f);

        g.setColour (juce::Colours::black);
        g.drawRoundedRectangle (cap, 3.0f, 1.0f);
    }

    // ---------------------------------------------------------------
    // Botones on/off tipo interruptor con LED
    // ---------------------------------------------------------------
    void drawToggleButton (juce::Graphics& g, juce::ToggleButton& button,
                            bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        auto bounds = button.getLocalBounds().toFloat().reduced (2.0f);
        const bool isOn = button.getToggleState();

        // Cuerpo del interruptor (bisel)
        juce::ColourGradient bodyGrad (juce::Colour (0xff3a3c40), bounds.getX(), bounds.getY(),
                                        juce::Colour (0xff222327), bounds.getX(), bounds.getBottom(), false);
        g.setGradientFill (bodyGrad);
        g.fillRoundedRectangle (bounds, 6.0f);

        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.drawRoundedRectangle (bounds, 6.0f, 1.2f);
        g.setColour (juce::Colours::white.withAlpha (shouldDrawButtonAsDown ? 0.04f : 0.09f));
        g.drawRoundedRectangle (bounds.reduced (1.0f), 5.0f, 1.0f);

        // LED indicador
        const float ledRadius = 5.0f;
        auto ledCentre = juce::Point<float> (bounds.getX() + 14.0f, bounds.getCentreY());

        juce::Colour ledColour = isOn ? juce::Colour (0xff5fd66a) : juce::Colour (0xff3a1f1f);
        if (isOn)
        {
            g.setColour (ledColour.withAlpha (0.35f));
            g.fillEllipse (ledCentre.x - ledRadius * 2.2f, ledCentre.y - ledRadius * 2.2f,
                            ledRadius * 4.4f, ledRadius * 4.4f); // glow
        }
        g.setColour (ledColour);
        g.fillEllipse (ledCentre.x - ledRadius, ledCentre.y - ledRadius, ledRadius * 2.0f, ledRadius * 2.0f);
        g.setColour (juce::Colours::black.withAlpha (0.5f));
        g.drawEllipse (ledCentre.x - ledRadius, ledCentre.y - ledRadius, ledRadius * 2.0f, ledRadius * 2.0f, 0.8f);

        // Texto
        g.setColour (isOn ? juce::Colour (0xfff0f0f0) : juce::Colour (0xff8a8a8a));
        g.setFont (juce::Font (13.0f, juce::Font::bold));
        g.drawFittedText (button.getButtonText(),
                           bounds.getX() + 26.0f, bounds.getY(), bounds.getWidth() - 30.0f, bounds.getHeight(),
                           juce::Justification::centredLeft, 1);

        juce::ignoreUnused (shouldDrawButtonAsHighlighted);
    }
};
