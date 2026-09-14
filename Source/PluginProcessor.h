#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "DSP/TapeDelayLine.h"
#include "DSP/HybridEQ.h"

class TapeDelayEQAudioProcessor : public juce::AudioProcessor
{
public:
    TapeDelayEQAudioProcessor();
    ~TapeDelayEQAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Tape Delay EQ"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 4.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    juce::AudioProcessorValueTreeState apvts { *this, nullptr, "PARAMS", createParameterLayout() };

private:
    static constexpr int maxChannels = 2;

    std::array<TapeDelayLine, maxChannels> delayLines;
    std::array<HybridEQ, maxChannels> eqs;

    // Cacheados por bloque para no leer atómicos por sample
    float delayTimeMs = 350.0f;
    float feedback = 0.35f;
    float mix = 0.5f;
    float wowDepthMs = 2.0f;
    float flutterDepthMs = 0.4f;
    float saturationDrive = 2.0f;
    float eqSaturation = 0.3f;

    bool dryOn   = true;
    bool delayOn = true;
    bool eqOn    = true;

    float dryGain = 1.0f; // lineal, cacheado desde dB
    float wetGain = 1.0f;

    void updateParameterCache();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TapeDelayEQAudioProcessor)
};
