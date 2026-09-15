#include "PluginProcessor.h"
#include "PluginEditor.h"

TapeDelayEQAudioProcessor::TapeDelayEQAudioProcessor()
    : AudioProcessor (BusesProperties()
        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
        .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
}

juce::AudioProcessorValueTreeState::ParameterLayout
TapeDelayEQAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // --- Delay principal ---
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        "delayTimeMs", "Delay Time",
        juce::NormalisableRange<float> (10.0f, 2000.0f, 0.1f, 0.4f), 350.0f, "ms"));

    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        "feedback", "Feedback",
        juce::NormalisableRange<float> (0.0f, 0.95f), 0.35f));

    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        "mix", "Mix (Dry/Wet)",
        juce::NormalisableRange<float> (0.0f, 1.0f), 0.5f));

    // --- Carácter de cinta ---
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        "wowDepth", "Wow Depth", juce::NormalisableRange<float> (0.0f, 8.0f), 2.0f, "ms"));

    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        "flutterDepth", "Flutter Depth", juce::NormalisableRange<float> (0.0f, 3.0f), 0.4f, "ms"));

    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        "saturationDrive", "Tape Saturation",
        juce::NormalisableRange<float> (1.0f, 8.0f), 2.0f));

    // --- EQ de 12 bandas (aplica SOLO a la señal de delay) ---
    static const char* bandNames[HybridEQ::numBands] = {
        "eqLowShelf", "eqBand2", "eqBand3", "eqBand4", "eqBand5", "eqBand6",
        "eqBand7", "eqBand8", "eqBand9", "eqBand10", "eqBand11", "eqHighShelf"
    };

    for (auto* id : bandNames)
    {
        params.push_back (std::make_unique<juce::AudioParameterFloat>(
            id, id, juce::NormalisableRange<float> (-15.0f, 15.0f, 0.1f), 0.0f, "dB"));
    }

    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        "eqSaturation", "EQ Character", juce::NormalisableRange<float> (0.0f, 1.0f), 0.3f));

    // --- Interruptores on/off ---
    params.push_back (std::make_unique<juce::AudioParameterBool>(
        "dryOn", "Original On", true));

    params.push_back (std::make_unique<juce::AudioParameterBool>(
        "delayOn", "Delay On", true));

    params.push_back (std::make_unique<juce::AudioParameterBool>(
        "eqOn", "EQ On", true));

    // --- Ganancias independientes ---
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        "dryGain", "Original Gain",
        juce::NormalisableRange<float> (-24.0f, 12.0f, 0.1f), 0.0f, "dB"));

    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        "wetGain", "Delay Gain",
        juce::NormalisableRange<float> (-24.0f, 12.0f, 0.1f), 0.0f, "dB"));

    // --- Sync a tempo ---
    params.push_back (std::make_unique<juce::AudioParameterBool>(
        "syncOn", "Sync", false));

    params.push_back (std::make_unique<juce::AudioParameterChoice>(
        "noteDivision", "Note Division",
        juce::StringArray { "1/1", "1/2", "1/2.", "1/4", "1/4.", "1/4T",
                             "1/8", "1/8.", "1/8T", "1/16", "1/16.", "1/16T" },
        3)); // 1/4 por defecto

    // --- Estéreo y modos especiales ---
    params.push_back (std::make_unique<juce::AudioParameterBool>(
        "pingPong", "Ping-Pong", false));

    params.push_back (std::make_unique<juce::AudioParameterBool>(
        "freeze", "Freeze", false));

    // --- Ducking ---
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        "duckAmount", "Duck Amount", juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));

    return { params.begin(), params.end() };
}

void TapeDelayEQAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Cada TapeDelayLine es dedicada a UN canal de audio, así que se prepara
    // como mono (numChannels = 1) aunque el plugin sea estéreo.
    juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) samplesPerBlock, 1u };

    for (auto& d : delayLines) d.prepare (spec);
    for (auto& e : eqs)        e.prepare (spec);

    duckEnvelope = 0.0f;
    updateParameterCache();
}

void TapeDelayEQAudioProcessor::updateParameterCache()
{
    delayTimeMs     = apvts.getRawParameterValue ("delayTimeMs")->load();
    feedback        = apvts.getRawParameterValue ("feedback")->load();
    mix             = apvts.getRawParameterValue ("mix")->load();
    wowDepthMs      = apvts.getRawParameterValue ("wowDepth")->load();
    flutterDepthMs  = apvts.getRawParameterValue ("flutterDepth")->load();
    saturationDrive = apvts.getRawParameterValue ("saturationDrive")->load();
    eqSaturation    = apvts.getRawParameterValue ("eqSaturation")->load();

    dryOn   = apvts.getRawParameterValue ("dryOn")->load()   > 0.5f;
    delayOn = apvts.getRawParameterValue ("delayOn")->load() > 0.5f;
    eqOn    = apvts.getRawParameterValue ("eqOn")->load()    > 0.5f;

    dryGain = juce::Decibels::decibelsToGain (apvts.getRawParameterValue ("dryGain")->load());
    wetGain = juce::Decibels::decibelsToGain (apvts.getRawParameterValue ("wetGain")->load());

    syncOn           = apvts.getRawParameterValue ("syncOn")->load() > 0.5f;
    noteDivisionIndex = (int) apvts.getRawParameterValue ("noteDivision")->load();
    pingPong          = apvts.getRawParameterValue ("pingPong")->load() > 0.5f;
    freeze            = apvts.getRawParameterValue ("freeze")->load()   > 0.5f;
    duckAmount        = apvts.getRawParameterValue ("duckAmount")->load();

    // BPM del host, si está disponible (DAW enviándolo vía playhead)
    if (auto* playHead = getPlayHead())
    {
        if (auto position = playHead->getPosition())
        {
            if (auto bpm = position->getBpm())
                lastKnownBpm = *bpm;
        }
    }

    static const char* bandNames[HybridEQ::numBands] = {
        "eqLowShelf", "eqBand2", "eqBand3", "eqBand4", "eqBand5", "eqBand6",
        "eqBand7", "eqBand8", "eqBand9", "eqBand10", "eqBand11", "eqHighShelf"
    };

    const float effectiveDelayMs = getEffectiveDelayMs();

    for (auto& d : delayLines)
    {
        d.setDelayTimeMs (effectiveDelayMs);
        d.setWowDepthMs (wowDepthMs);
        d.setFlutterDepthMs (flutterDepthMs);
        d.setSaturationDrive (saturationDrive);
    }

    for (auto& e : eqs)
    {
        for (int b = 0; b < HybridEQ::numBands; ++b)
            e.setBandGainDb (b, apvts.getRawParameterValue (bandNames[b])->load());
        e.setSaturationAmount (eqSaturation);
    }
}

float TapeDelayEQAudioProcessor::getEffectiveDelayMs() const
{
    if (! syncOn)
        return delayTimeMs;

    const float beats = noteDivisionBeats[(size_t) juce::jlimit (0, 11, noteDivisionIndex)];
    const double msPerBeat = 60000.0 / juce::jmax (20.0, lastKnownBpm);
    return (float) (msPerBeat * beats);
}

namespace
{
    // Limitador suave de seguridad: transparente por debajo del umbral,
    // comprime con una curva tanh por encima para evitar un clip digital
    // duro si el EQ + feedback empujan la señal muy fuerte.
    inline float safetyLimit (float x)
    {
        constexpr float threshold = 0.9f;
        const float absX = std::abs (x);
        if (absX <= threshold)
            return x;

        const float sign = x < 0.0f ? -1.0f : 1.0f;
        const float over = absX - threshold;
        return sign * (threshold + std::tanh (over) * (1.0f - threshold));
    }
}

void TapeDelayEQAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    updateParameterCache(); // simple; para producción, suaviza por sample con SmoothedValue

    const int numChannels = juce::jmin (buffer.getNumChannels(), maxChannels);
    const int numSamples  = buffer.getNumSamples();
    const bool stereo = numChannels >= 2;

    auto* left  = buffer.getWritePointer (0);
    auto* right = stereo ? buffer.getWritePointer (1) : left;

    for (int n = 0; n < numSamples; ++n)
    {
        const float dryL = left[n];
        const float dryR = right[n];

        // 1) Leer ambos canales retrasados+modulados (sin saturar: eso pasa al escribir)
        const float delayedL = delayLines[0].readSample (0);
        const float delayedR = delayLines[1].readSample (0);

        // 2) EQ híbrido, SOLO sobre la señal duplicada, uno por canal
        const float eqL = eqOn ? eqs[0].processSample (delayedL) : delayedL;
        const float eqR = eqOn ? eqs[1].processSample (delayedR) : delayedR;

        // 3) Señal de feedback a re-escribir.
        //    - Normal: cada canal se realimenta consigo mismo.
        //    - Ping-Pong: el eco de un canal alimenta el OTRO canal, creando
        //      el clásico rebote L-R-L-R (solo tiene sentido en estéreo).
        //    - Freeze: no se inyecta señal nueva (dry) y el feedback pasa a
        //      1:1 sobre lo que ya está en el buffer, para que el loop actual
        //      se sostenga indefinidamente en vez de decaer o crecer.
        float feedbackL, feedbackR;

        if (freeze)
        {
            feedbackL = eqL;
            feedbackR = eqR;
        }
        else if (pingPong && stereo)
        {
            feedbackL = dryL + eqR * feedback;
            feedbackR = dryR + eqL * feedback;
        }
        else
        {
            feedbackL = dryL + eqL * feedback;
            feedbackR = dryR + eqR * feedback;
        }

        delayLines[0].pushSample (0, feedbackL);
        delayLines[1].pushSample (0, feedbackR);
        delayLines[0].advance();
        delayLines[1].advance();

        // 4) Ducking: un envelope follower sigue el nivel de la señal dry
        //    (ataque rápido, liberación lenta) y reduce el volumen del delay
        //    cuando hay señal fuerte en la entrada, para que no ensucie la
        //    mezcla mientras tocas/cantas.
        const float inputLevel = juce::jmax (std::abs (dryL), std::abs (dryR));
        constexpr float attackCoeff  = 0.6f;  // reacciona casi instantáneo
        constexpr float releaseCoeff = 0.9995f; // suelta lento (~varios cientos de ms)
        if (inputLevel > duckEnvelope)
            duckEnvelope = attackCoeff * duckEnvelope + (1.0f - attackCoeff) * inputLevel;
        else
            duckEnvelope = releaseCoeff * duckEnvelope + (1.0f - releaseCoeff) * inputLevel;

        const float duckGain = 1.0f - duckAmount * juce::jlimit (0.0f, 1.0f, duckEnvelope * 3.0f);

        // 5) Mezcla final + limitador de seguridad
        const float dryOutL = dryOn   ? dryL * (1.0f - mix) * dryGain : 0.0f;
        const float dryOutR = dryOn   ? dryR * (1.0f - mix) * dryGain : 0.0f;
        const float wetOutL = delayOn ? eqL  * mix          * wetGain * duckGain : 0.0f;
        const float wetOutR = delayOn ? eqR  * mix          * wetGain * duckGain : 0.0f;

        left[n]  = safetyLimit (dryOutL + wetOutL);
        if (stereo)
            right[n] = safetyLimit (dryOutR + wetOutR);
    }
}

juce::AudioProcessorEditor* TapeDelayEQAudioProcessor::createEditor()
{
    return new TapeDelayEQAudioProcessorEditor (*this);
}

void TapeDelayEQAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto state = apvts.copyState(); true)
    {
        std::unique_ptr<juce::XmlElement> xml (state.createXml());
        copyXmlToBinary (*xml, destData);
    }
}

void TapeDelayEQAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new TapeDelayEQAudioProcessor();
}
