#pragma once
#include <juce_dsp/juce_dsp.h>
#include <array>

/**
    EQ híbrido de 12 bandas (mismo concepto que KickForge EQ):
      - Banda 1:  Low Shelf
      - Bandas 2-11: 10 bandas paramétricas (peaking) repartidas en escala
                      logarítmica entre ~80 Hz y ~12 kHz
      - Banda 12: High Shelf
      - Etapa final de saturación suave para el "carácter analógico"

    En este plugin se instancia UNA vez por canal y se usa SOLO sobre la
    señal duplicada (el delay), nunca sobre la señal seca.
*/
class HybridEQ
{
public:
    static constexpr int numBands = 12;

    void prepare (const juce::dsp::ProcessSpec& spec)
    {
        sampleRate = spec.sampleRate;

        // Frecuencias centrales de las 10 bandas paramétricas (log-spaced)
        constexpr std::array<float, 10> peakFreqs {
            120.f, 220.f, 380.f, 650.f, 1000.f,
            1600.f, 2500.f, 4000.f, 6500.f, 10000.f
        };

        for (int i = 0; i < numBands; ++i)
        {
            filters[i].prepare (spec);
            filters[i].reset();
        }

        for (int i = 0; i < 10; ++i)
            peakFreq[i] = peakFreqs[i];

        updateAllCoefficients();
    }

    void reset()
    {
        for (auto& f : filters)
            f.reset();
    }

    // gainDb en el rango típico [-15, +15]. band = 0 (low shelf) .. 11 (high shelf)
    void setBandGainDb (int band, float gainDb)
    {
        jassert (band >= 0 && band < numBands);
        bandGainDb[(size_t) band] = gainDb;
        updateCoefficients (band);
    }

    void setSaturationAmount (float amount) { saturationAmount = amount; } // 0..1

    // Una instancia de HybridEQ = un canal. Se crean tantas instancias
    // como canales tenga el plugin (ver PluginProcessor).
    float processSample (float x)
    {
        float y = x;
        for (auto& f : filters)
            y = f.processSample (y);

        if (saturationAmount > 0.0f)
        {
            const float driven = y * (1.0f + saturationAmount * 3.0f);
            y = juce::jmap (saturationAmount, 0.0f, 1.0f, y, std::tanh (driven));
        }
        return y;
    }

private:
    void updateAllCoefficients()
    {
        for (int i = 0; i < numBands; ++i)
            updateCoefficients (i);
    }

    void updateCoefficients (int band)
    {
        const float gain = juce::Decibels::decibelsToGain (bandGainDb[(size_t) band]);

        juce::dsp::IIR::Coefficients<float>::Ptr coeffs;

        if (band == 0)
        {
            // Low shelf ~80 Hz
            coeffs = juce::dsp::IIR::Coefficients<float>::makeLowShelf (
                sampleRate, 80.0f, 0.71f, gain);
        }
        else if (band == numBands - 1)
        {
            // High shelf ~12 kHz
            coeffs = juce::dsp::IIR::Coefficients<float>::makeHighShelf (
                sampleRate, 12000.0f, 0.71f, gain);
        }
        else
        {
            // Peaking bands 1..10 -> índice 0..9 en peakFreq
            const float freq = peakFreq[(size_t) (band - 1)];
            coeffs = juce::dsp::IIR::Coefficients<float>::makePeakFilter (
                sampleRate, freq, 1.0f, gain);
        }

        *filters[(size_t) band].coefficients = *coeffs;
    }

    double sampleRate = 44100.0;
    std::array<juce::dsp::IIR::Filter<float>, numBands> filters;
    std::array<float, numBands> bandGainDb {};
    std::array<float, 10> peakFreq {};
    float saturationAmount = 0.3f;
};
