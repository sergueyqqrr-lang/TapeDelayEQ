#pragma once
#include <juce_dsp/juce_dsp.h>

/**
    Línea de delay estilo cinta:
      - Buffer circular con lectura fraccional (interpolación).
      - Modulación del tiempo de delay con dos LFOs (wow lento, flutter rápido)
        para simular la inestabilidad mecánica de una cinta real.
      - Saturación suave (tanh) aplicada al ESCRIBIR en el buffer (como una
        cabeza de grabación real saturando la señal entrante), no al leerla.
        Esto es importante: si se saturara al leer, el valor guardado en el
        buffer de feedback podría crecer sin límite en cada repetición y
        recién "aplastarse" de golpe al leerlo, sonando como un recorte
        abrupto en vez de una saturación musical progresiva.

    Esta clase SOLO se encarga del delay + modulación + saturación.
    El EQ se aplica desde fuera (en PluginProcessor), sobre la señal que
    esta clase entrega en cada lectura, para poder insertarlo dentro del loop.
*/
class TapeDelayLine
{
public:
    void prepare (const juce::dsp::ProcessSpec& spec)
    {
        sampleRate = spec.sampleRate;

        // Buffer de hasta 4 segundos de delay
        const int maxDelaySamples = (int) (sampleRate * 4.0) + 4;
        buffer.setSize ((int) spec.numChannels, maxDelaySamples);
        buffer.clear();
        writePos = 0;

        wowLFO.prepare (spec);
        wowLFO.initialise ([] (float x) { return std::sin (x); });
        wowLFO.setFrequency (0.6f); // Hz, lento

        flutterLFO.prepare (spec);
        flutterLFO.initialise ([] (float x) { return std::sin (x); });
        flutterLFO.setFrequency (6.0f); // Hz, rápido

        smoothedDelaySamples.reset (sampleRate, 0.02);
        smoothedDelaySamples.setCurrentAndTargetValue ((float) (0.35 * sampleRate));
    }

    void reset()
    {
        buffer.clear();
        writePos = 0;
        wowLFO.reset();
        flutterLFO.reset();
    }

    void setDelayTimeMs (float ms)
    {
        smoothedDelaySamples.setTargetValue ((float) (ms * 0.001 * sampleRate));
    }

    void setWowDepthMs (float ms)   { wowDepthSamples = (float) (ms * 0.001 * sampleRate); }
    void setFlutterDepthMs (float ms) { flutterDepthSamples = (float) (ms * 0.001 * sampleRate); }
    void setSaturationDrive (float driveAmount) { drive = driveAmount; } // 1 = limpio, >1 = más saturado

    // Lee la muestra retrasada+modulada para un canal dado (SIN saturar:
    // la saturación se aplica al escribir, ver pushSample). NO avanza el
    // buffer (eso lo hace pushSample). Se usa para poder insertar el EQ
    // entre la lectura y la escritura del feedback.
    float readSample (int channel)
    {
        const float wow     = wowLFO.processSample (0.0f)     * wowDepthSamples;
        const float flutter = flutterLFO.processSample (0.0f) * flutterDepthSamples;

        float delaySamples = smoothedDelaySamples.getNextValue() + wow + flutter;
        delaySamples = juce::jlimit (1.0f, (float) (buffer.getNumSamples() - 2), delaySamples);

        const float readPosF = (float) writePos - delaySamples;
        const float readPos  = readPosF < 0.0f ? readPosF + buffer.getNumSamples() : readPosF;

        const int   idx0 = (int) readPos;
        const int   idx1 = (idx0 + 1) % buffer.getNumSamples();
        const float frac = readPos - (float) idx0;

        const float s0 = buffer.getSample (channel, idx0);
        const float s1 = buffer.getSample (channel, idx1);
        return s0 + frac * (s1 - s0);
    }

    // Escribe la señal de feedback (input + eco procesado*feedbackGain) en el
    // buffer, aplicando la saturación tipo cinta AQUÍ (al "grabar"). Así el
    // valor guardado siempre queda acotado, evitando que crezca sin control
    // en repeticiones sucesivas con EQ boosteado.
    // Debe llamarse una vez por canal, en el mismo sample, DESPUÉS de haber
    // leído (readSample) y de haber pasado la señal por el EQ externo.
    void pushSample (int channel, float value)
    {
        const float saturated = std::tanh (value * drive) / std::tanh (drive);
        buffer.setSample (channel, writePos, saturated);
    }

    // Avanza el puntero de escritura una vez que todos los canales fueron escritos.
    void advance()
    {
        writePos = (writePos + 1) % buffer.getNumSamples();
    }

    // Lee una muestra en un tiempo de delay ARBITRARIO (en ms), sin
    // modulación de wow/flutter. Se usa para los "taps" extra del modo
    // multi-tap, que leen el mismo buffer en otros puntos además del
    // tap principal (que sí usa readSample con modulación completa).
    float readAtMs (int channel, float ms) const
    {
        float delaySamples = (float) (ms * 0.001 * sampleRate);
        delaySamples = juce::jlimit (1.0f, (float) (buffer.getNumSamples() - 2), delaySamples);

        const float readPosF = (float) writePos - delaySamples;
        const float readPos  = readPosF < 0.0f ? readPosF + buffer.getNumSamples() : readPosF;

        const int   idx0 = (int) readPos;
        const int   idx1 = (idx0 + 1) % buffer.getNumSamples();
        const float frac = readPos - (float) idx0;

        const float s0 = buffer.getSample (channel, idx0);
        const float s1 = buffer.getSample (channel, idx1);
        return s0 + frac * (s1 - s0);
    }

private:
    juce::AudioBuffer<float> buffer;
    int writePos = 0;
    double sampleRate = 44100.0;

    juce::dsp::Oscillator<float> wowLFO, flutterLFO;
    float wowDepthSamples = 0.0f;
    float flutterDepthSamples = 0.0f;

    juce::SmoothedValue<float> smoothedDelaySamples;
    float drive = 1.0001f; // evitar div/0 en tanh(drive)
};
