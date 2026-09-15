#pragma once
#include "PluginProcessor.h"
#include "DSP/HybridEQ.h"
#include "UI/HardwareLookAndFeel.h"

class TapeDelayEQAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit TapeDelayEQAudioProcessorEditor (TapeDelayEQAudioProcessor&);
    ~TapeDelayEQAudioProcessorEditor() override;

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

    juce::Slider dryGainSlider, wetGainSlider;
    juce::Label  dryGainLabel, wetGainLabel;
    std::unique_ptr<SliderAttachment> dryGainAtt, wetGainAtt;

    // Sync / estéreo / modos especiales / ducking
    juce::ToggleButton syncButton, pingPongButton, freezeButton;
    juce::ComboBox noteDivisionBox;
    juce::Slider duckSlider;
    juce::Label  duckLabel;
    std::unique_ptr<ButtonAttachment> syncAtt, pingPongAtt, freezeAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> noteDivisionAtt;
    std::unique_ptr<SliderAttachment> duckAtt;

    HardwareLookAndFeel hardwareLookAndFeel;

    // Presets de fábrica
    juce::ComboBox presetBox;

    // Multi-tap + difusión
    juce::ToggleButton tap2OnButton, tap3OnButton;
    juce::Slider tap2LevelSlider, tap3LevelSlider, diffusionSlider;
    juce::Slider tap2RatioSlider, tap3RatioSlider;
    juce::Label  tap2LevelLabel, tap3LevelLabel, diffusionLabel;
    juce::Label  tap2RatioLabel, tap3RatioLabel;
    std::unique_ptr<ButtonAttachment> tap2OnAtt, tap3OnAtt;
    std::unique_ptr<SliderAttachment> tap2LevelAtt, tap3LevelAtt, diffusionAtt;
    std::unique_ptr<SliderAttachment> tap2RatioAtt, tap3RatioAtt;

    // 12 bandas del EQ + saturación del EQ
    static constexpr int numEqControls = HybridEQ::numBands + 1;
    std::array<juce::Slider, numEqControls> eqSliders;
    std::array<juce::Label,  numEqControls> eqLabels;
    std::array<std::unique_ptr<SliderAttachment>, numEqControls> eqAtt;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TapeDelayEQAudioProcessorEditor)
};
