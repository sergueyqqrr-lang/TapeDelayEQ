#pragma once
#include <juce_core/juce_core.h>
#include <array>
#include <vector>
#include <cmath>

/**
    Filtro allpass de un solo delay (celda de Schroeder), la unidad básica
    de un difusor. No cambia el timbre (allpass = respuesta en frecuencia
    plana) pero "esparce" la señal en el tiempo, dando sensación de espacio.
*/
class AllpassFilter
{
public:
    void prepare (double sampleRate, float delayMs)
    {
        bufferSize = juce::jmax (4, (int) (delayMs * 0.001 * sampleRate));
        buffer.assign ((size_t) bufferSize, 0.0f);
        writePos = 0;
    }

    void reset()
    {
        std::fill (buffer.begin(), buffer.end(), 0.0f);
        writePos = 0;
    }

    float processSample (float x, float g)
    {
        const float bufOut = buffer[(size_t) writePos];
        const float y = -g * x + bufOut;
        buffer[(size_t) writePos] = x + g * bufOut;
        writePos = (writePos + 1) % bufferSize;
        return y;
    }

private:
    std::vector<float> buffer;
    int bufferSize = 1;
    int writePos = 0;
};

/**
    Cadena de 4 allpass en serie con tiempos distintos (valores clásicos
    de difusores tipo Schroeder/Moorer), para dar sensación de
    "reverb corto" en las repeticiones del delay sin ser una reverb
    completa. Una instancia por canal.
*/
class Diffuser
{
public:
    void prepare (double sampleRate)
    {
        static constexpr std::array<float, 4> delayTimesMs { 4.7f, 3.6f, 2.5f, 1.7f };
        for (size_t i = 0; i < stages.size(); ++i)
            stages[i].prepare (sampleRate, delayTimesMs[i]);
    }

    void reset()
    {
        for (auto& s : stages) s.reset();
    }

    // amount: 0 = sin difusión (transparente), 1 = máxima difusión
    float processSample (float x, float amount)
    {
        const float g = juce::jlimit (0.0f, 0.65f, amount * 0.65f);
        float y = x;
        for (auto& s : stages)
            y = s.processSample (y, g);
        return y;
    }

private:
    std::array<AllpassFilter, 4> stages;
};
