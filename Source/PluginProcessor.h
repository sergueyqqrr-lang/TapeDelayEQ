#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "DSP/TapeDelayLine.h"
#include "DSP/HybridEQ.h"
#include "DSP/Diffuser.h"
#include <map>
#include <vector>

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

    // --- Presets de fábrica ---
    struct FactoryPreset
    {
        juce::String name;
        std::map<juce::String, float> values; // paramID -> valor real (no normalizado)
    };
    static const std::vector<FactoryPreset>& getFactoryPresets();
    void applyPreset (int index); // aplica un preset por índice (ver getFactoryPresets)

    // Multiplicadores en "beats de negra" para cada división de nota del combo
    // de sync. Índice = valor del AudioParameterChoice "noteDivision".
    static constexpr std::array<float, 12> noteDivisionBeats {
        4.0f,       // 1/1
        2.0f,       // 1/2
        3.0f,       // 1/2 con puntillo
        1.0f,       // 1/4
        1.5f,       // 1/4 con puntillo
        2.0f/3.0f,  // 1/4 tresillo
        0.5f,       // 1/8
        0.75f,      // 1/8 con puntillo
        1.0f/3.0f,  // 1/8 tresillo
        0.25f,      // 1/16
        0.375f,     // 1/16 con puntillo
        1.0f/6.0f   // 1/16 tresillo
    };

private:
    static constexpr int maxChannels = 2;

    std::array<TapeDelayLine, maxChannels> delayLines;
    std::array<HybridEQ, maxChannels> eqs;
    std::array<Diffuser, maxChannels> diffusers;

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

    bool syncOn      = false;
    int  noteDivisionIndex = 3; // 1/4 por defecto
    bool pingPong    = false;
    bool freeze      = false;

    float duckAmount = 0.0f;
    float duckEnvelope = 0.0f; // estado del envelope follower (persiste entre bloques)

    // --- Multi-tap ---
    bool  tap2On    = false;
    float tap2Level = 0.5f;
    float tap2Ratio = 0.5f;
    bool  tap3On    = false;
    float tap3Level = 0.35f;
    float tap3Ratio = 1.5f;
    float cachedDelayMs = 350.0f; // delay efectivo (con o sin sync) del bloque actual

    // --- Difusión / reverb en las repeticiones ---
    float diffusionAmount = 0.0f;

    double lastKnownBpm = 120.0;

    void updateParameterCache();
    float getEffectiveDelayMs() const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TapeDelayEQAudioProcessor)
};
