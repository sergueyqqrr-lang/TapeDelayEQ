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

    return { params.begin(), params.end() };
}

void TapeDelayEQAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Cada TapeDelayLine es dedicada a UN canal de audio, así que se prepara
    // como mono (numChannels = 1) aunque el plugin sea estéreo.
    juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) samplesPerBlock, 1u };

    for (auto& d : delayLines) d.prepare (spec);
    for (auto& e : eqs)        e.prepare (spec);

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

    static const char* bandNames[HybridEQ::numBands] = {
        "eqLowShelf", "eqBand2", "eqBand3", "eqBand4", "eqBand5", "eqBand6",
        "eqBand7", "eqBand8", "eqBand9", "eqBand10", "eqBand11", "eqHighShelf"
    };

    for (auto& d : delayLines)
    {
        d.setDelayTimeMs (delayTimeMs);
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

void TapeDelayEQAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    updateParameterCache(); // simple; para producción, suaviza por sample con SmoothedValue

    const int numChannels = juce::jmin (buffer.getNumChannels(), maxChannels);
    const int numSamples  = buffer.getNumSamples();

    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* channelData = buffer.getWritePointer (ch);
        auto& delayLine    = delayLines[(size_t) ch];
        auto& eq           = eqs[(size_t) ch];

        for (int n = 0; n < numSamples; ++n)
        {
            const float dry = channelData[n];

            // 1) Leer la señal ya retrasada + modulada (wow/flutter) + saturada tipo cinta
            //    (canal 0 siempre: cada TapeDelayLine es dedicada a un solo canal)
            const float delayed = delayLine.readSample (0);

            // 2) Aplicar el EQ híbrido SOLO a la señal duplicada (nunca al dry)
            const float delayedEQd = eq.processSample (delayed);

            // 3) Esta es la señal "wet" que se escucha
            const float wet = delayedEQd;

            // 4) Señal de feedback: entrada actual + eco ya procesado * feedback.
            //    Como se re-escribe delayedEQd (ya pasado por EQ), en la SIGUIENTE
            //    repetición se vuelve a leer y se le vuelve a aplicar el EQ:
            //    el color se acumula repetición tras repetición, como en una cinta real.
            const float feedbackSignal = dry + delayedEQd * feedback;
            delayLine.pushSample (0, feedbackSignal);
            delayLine.advance();

            // 5) Mezcla final dry/wet (la señal original NUNCA pasa por el EQ ni el delay)
            channelData[n] = dry * (1.0f - mix) + wet * mix;
        }
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
