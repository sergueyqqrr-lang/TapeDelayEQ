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

    // --- EQ de 12 bandas (aplica solo a la señal de delay) ---
    static const char* eqParamIDs[HybridEQ::numBands] = {
        "eqLowShelf", "eqBand2", "eqBand3", "eqBand4", "eqBand5", "eqBand6",
        "eqBand7", "eqBand8", "eqBand9", "eqBand10", "eqBand11", "eqHighShelf"
    };
    static const char* eqShortNames[HybridEQ::numBands] = {
        "80", "120", "220", "380", "650", "1k",
        "1.6k", "2.5k", "4k", "6.5k", "10k", "12k"
    };

    for (int i = 0; i < HybridEQ::numBands; ++i)
    {
        auto& s = eqSliders[(size_t) i];
        auto& l = eqLabels[(size_t) i];

        s.setSliderStyle (juce::Slider::LinearVertical);
        s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 45, 18);
        addAndMakeVisible (s);

        l.setText (eqShortNames[i], juce::dontSendNotification);
        l.setJustificationType (juce::Justification::centred);
        l.setFont (11.0f);
        addAndMakeVisible (l);

        eqAtt[(size_t) i] = std::make_unique<SliderAttachment> (apvts, eqParamIDs[i], s);
    }

    // Saturación del EQ (carácter/calidez), como knob al final de la fila
    auto& eqSatSlider = eqSliders[(size_t) HybridEQ::numBands];
    auto& eqSatLabel  = eqLabels[(size_t) HybridEQ::numBands];
    eqSatSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    eqSatSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 55, 18);
    addAndMakeVisible (eqSatSlider);
    eqSatLabel.setText ("EQ Char.", juce::dontSendNotification);
    eqSatLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (eqSatLabel);
    eqAtt[(size_t) HybridEQ::numBands] = std::make_unique<SliderAttachment> (apvts, "eqSaturation", eqSatSlider);

    setSize (760, 420);
}

void TapeDelayEQAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff1e1e1e));
    g.setColour (juce::Colours::white);
    g.setFont (16.0f);
    g.drawFittedText ("Tape Delay EQ", getLocalBounds().removeFromTop (30),
                       juce::Justification::centred, 1);

    g.setFont (12.0f);
    g.setColour (juce::Colours::grey);
    g.drawFittedText ("EQ (solo señal de delay)", { 0, 195, getWidth(), 20 },
                       juce::Justification::centred, 1);
}

void TapeDelayEQAudioProcessorEditor::resized()
{
    // --- Fila superior: controles del delay ---
    auto area = getLocalBounds().withTrimmedTop (40);
    auto delayArea = area.removeFromTop (150).reduced (10);
    const int knobWidth = delayArea.getWidth() / 6;

    juce::Slider* sliders[] = { &delayTimeSlider, &feedbackSlider, &mixSlider,
                                 &wowSlider, &flutterSlider, &satSlider };
    juce::Label*  labels[]  = { &delayTimeLabel, &feedbackLabel, &mixLabel,
                                 &wowLabel, &flutterLabel, &satLabel };

    for (int i = 0; i < 6; ++i)
    {
        auto col = delayArea.removeFromLeft (knobWidth);
        labels[i]->setBounds (col.removeFromTop (20));
        sliders[i]->setBounds (col.reduced (4));
    }

    // --- Fila inferior: 12 bandas de EQ + carácter ---
    auto eqArea = area.withTrimmedTop (20).reduced (10);
    const int eqWidth = eqArea.getWidth() / numEqControls;

    for (int i = 0; i < numEqControls; ++i)
    {
        auto col = eqArea.removeFromLeft (eqWidth);
        eqLabels[(size_t) i].setBounds (col.removeFromTop (16));
        eqSliders[(size_t) i].setBounds (col.reduced (2));
    }
}
