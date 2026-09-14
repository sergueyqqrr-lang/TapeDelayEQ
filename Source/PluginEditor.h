#pragma once
#include "PluginProcessor.h"

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

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TapeDelayEQAudioProcessorEditor)
};
