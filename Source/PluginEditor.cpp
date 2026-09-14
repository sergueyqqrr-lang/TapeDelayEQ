#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    void setupKnob (juce::Slider& s, juce::Label& l, const juce::String& text, juce::Component& parent)
    {
        s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 20);
        parent.addAndMakeVisible (s);

        l.setText (text, juce::dontSendNotification);
        l.setJustificationType (juce::Justification::centred);
        parent.addAndMakeVisible (l);
    }
}

TapeDelayEQAudioProcessorEditor::TapeDelayEQAudioProcessorEditor (TapeDelayEQAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    setupKnob (delayTimeSlider, delayTimeLabel, "Time",     *this);
    setupKnob (feedbackSlider,  feedbackLabel,  "Feedback", *this);
    setupKnob (mixSlider,       mixLabel,       "Mix",      *this);
    setupKnob (wowSlider,       wowLabel,       "Wow",      *this);
    setupKnob (flutterSlider,   flutterLabel,   "Flutter",  *this);
    setupKnob (satSlider,       satLabel,       "Sat.",     *this);

    auto& apvts = processorRef.apvts;
    delayTimeAtt = std::make_unique<SliderAttachment> (apvts, "delayTimeMs",     delayTimeSlider);
    feedbackAtt  = std::make_unique<SliderAttachment> (apvts, "feedback",        feedbackSlider);
    mixAtt       = std::make_unique<SliderAttachment> (apvts, "mix",             mixSlider);
    wowAtt       = std::make_unique<SliderAttachment> (apvts, "wowDepth",        wowSlider);
    flutterAtt   = std::make_unique<SliderAttachment> (apvts, "flutterDepth",    flutterSlider);
    satAtt       = std::make_unique<SliderAttachment> (apvts, "saturationDrive", satSlider);

    // Nota: las 12 bandas del EQ (eqLowShelf, eqBand2..eqBand11, eqHighShelf)
    // y "eqSaturation" ya existen como parámetros en el APVTS; solo falta
    // agregarles sliders aquí igual que los de arriba cuando definas el layout
    // visual del EQ (por ejemplo, con juce::Slider verticales tipo "fader" en fila).

    setSize (480, 260);
}

void TapeDelayEQAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff1e1e1e));
    g.setColour (juce::Colours::white);
    g.setFont (16.0f);
    g.drawFittedText ("Tape Delay EQ", getLocalBounds().removeFromTop (30),
                       juce::Justification::centred, 1);
}

void TapeDelayEQAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().withTrimmedTop (40).reduced (10);
    const int knobWidth = area.getWidth() / 6;

    juce::Slider* sliders[] = { &delayTimeSlider, &feedbackSlider, &mixSlider,
                                 &wowSlider, &flutterSlider, &satSlider };
    juce::Label*  labels[]  = { &delayTimeLabel, &feedbackLabel, &mixLabel,
                                 &wowLabel, &flutterLabel, &satLabel };

    for (int i = 0; i < 6; ++i)
    {
        auto col = area.removeFromLeft (knobWidth);
        labels[i]->setBounds (col.removeFromTop (20));
        sliders[i]->setBounds (col.reduced (4));
    }
}
