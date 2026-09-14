#pragma once
#include "PluginProcessor.h"
#include "DSP/HybridEQ.h"

class TapeDelayEQAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit TapeDelayEQAudioProcessorEditor (TapeDelayEQAudioProcessor&);

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    TapeDelayEQAudioProcessor& processorRef;

    juce::Slider delayTimeSlider, feedbackSlider, mixSlider, wowSlider, flutterSlider, satSlider;
    juce::Label  delayTimeLabel, feedbackLabel, mixLabel, wowLabel, flutterLabel, satLabel;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<SliderAttachment> delayTimeAtt, feedbackAtt, mixAtt, wowAtt, flutterAtt, satAtt;

    juce::ToggleButton dryOnButton, delayOnButton, eqOnButton;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    std::unique_ptr<ButtonAttachment> dryOnAtt, delayOnAtt, eqOnAtt;

    // 12 bandas del EQ + saturación del EQ
    static constexpr int numEqControls = HybridEQ::numBands + 1;
    std::array<juce::Slider, numEqControls> eqSliders;
    std::array<juce::Label,  numEqControls> eqLabels;
    std::array<std::unique_ptr<SliderAttachment>, numEqControls> eqAtt;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TapeDelayEQAudioProcessorEditor)
};
