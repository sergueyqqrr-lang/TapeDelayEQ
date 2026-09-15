#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    void setupKnob (juce::Slider& s, juce::Label& l, const juce::String& text, juce::Component& parent)
    {
        s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 18);
        parent.addAndMakeVisible (s);

        l.setText (text, juce::dontSendNotification);
        l.setJustificationType (juce::Justification::centred);
        l.setFont (juce::Font (12.0f, juce::Font::bold));
        parent.addAndMakeVisible (l);
    }

    // Dibuja un panel "hundido" tipo bisel de rack (usado para agrupar secciones)
    void drawInsetPanel (juce::Graphics& g, juce::Rectangle<float> bounds, const juce::String& title)
    {
        g.setColour (juce::Colour (0xff1a1b1d));
        g.fillRoundedRectangle (bounds, 8.0f);

        g.setColour (juce::Colours::black.withAlpha (0.8f));
        g.drawRoundedRectangle (bounds, 8.0f, 1.5f);
        g.setColour (juce::Colours::white.withAlpha (0.05f));
        g.drawRoundedRectangle (bounds.reduced (1.5f), 7.0f, 1.0f);

        if (title.isNotEmpty())
        {
            g.setColour (juce::Colour (0xff8a8d93));
            g.setFont (juce::Font (11.0f, juce::Font::bold));
            g.drawText (title.toUpperCase(), bounds.getX() + 10, bounds.getY() + 4,
                        bounds.getWidth() - 20, 14, juce::Justification::centredLeft);
        }
    }
}

TapeDelayEQAudioProcessorEditor::TapeDelayEQAudioProcessorEditor (TapeDelayEQAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    setLookAndFeel (&hardwareLookAndFeel);

    setupKnob (delayTimeSlider, delayTimeLabel, "Time",     *this);
    setupKnob (feedbackSlider,  feedbackLabel,  "Feedback", *this);
    setupKnob (mixSlider,       mixLabel,       "Mix",      *this);
    setupKnob (wowSlider,       wowLabel,       "Wow",      *this);
    setupKnob (flutterSlider,   flutterLabel,   "Flutter",  *this);
    setupKnob (satSlider,       satLabel,       "Sat.",     *this);
    setupKnob (dryGainSlider,   dryGainLabel,   "Orig. Gain", *this);
    setupKnob (wetGainSlider,   wetGainLabel,   "Delay Gain", *this);
    // Estas dos van en la franja angosta de routing junto a los botones,
    // así que usamos un estilo horizontal compacto en vez de knob rotativo.
    for (auto* s : { &dryGainSlider, &wetGainSlider })
    {
        s->setSliderStyle (juce::Slider::LinearHorizontal);
        s->setTextBoxStyle (juce::Slider::TextBoxRight, false, 42, 18);
    }

    auto& apvts = processorRef.apvts;
    delayTimeAtt = std::make_unique<SliderAttachment> (apvts, "delayTimeMs",     delayTimeSlider);
    feedbackAtt  = std::make_unique<SliderAttachment> (apvts, "feedback",        feedbackSlider);
    mixAtt       = std::make_unique<SliderAttachment> (apvts, "mix",             mixSlider);
    wowAtt       = std::make_unique<SliderAttachment> (apvts, "wowDepth",        wowSlider);
    flutterAtt   = std::make_unique<SliderAttachment> (apvts, "flutterDepth",    flutterSlider);
    satAtt       = std::make_unique<SliderAttachment> (apvts, "saturationDrive", satSlider);
    dryGainAtt   = std::make_unique<SliderAttachment> (apvts, "dryGain",         dryGainSlider);
    wetGainAtt   = std::make_unique<SliderAttachment> (apvts, "wetGain",         wetGainSlider);

    // --- EQ de 12 bandas (aplica solo a la señal de delay) ---
    static const char* eqParamIDs[HybridEQ::numBands] = {
        "eqLowShelf", "eqBand2", "eqBand3", "eqBand4", "eqBand5", "eqBand6",
        "eqBand7", "eqBand8", "eqBand9", "eqBand10", "eqBand11", "eqHighShelf"
    };
    static const char* eqShortNames[HybridEQ::numBands] = {
        "80", "120", "220", "380", "650", "1k",
        "1.6k", "2.5k", "4k", "6.5k", "10k", "12k"
    };

    for (int i = 0; i < HybridEQ::numBands; ++i)
    {
        auto& s = eqSliders[(size_t) i];
        auto& l = eqLabels[(size_t) i];

        s.setSliderStyle (juce::Slider::LinearVertical);
        s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 40, 16);
        addAndMakeVisible (s);

        l.setText (eqShortNames[i], juce::dontSendNotification);
        l.setJustificationType (juce::Justification::centred);
        l.setFont (10.5f);
        addAndMakeVisible (l);

        eqAtt[(size_t) i] = std::make_unique<SliderAttachment> (apvts, eqParamIDs[i], s);
    }

    auto& eqSatSlider = eqSliders[(size_t) HybridEQ::numBands];
    auto& eqSatLabel  = eqLabels[(size_t) HybridEQ::numBands];
    eqSatSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    eqSatSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 50, 16);
    addAndMakeVisible (eqSatSlider);
    eqSatLabel.setText ("Char.", juce::dontSendNotification);
    eqSatLabel.setJustificationType (juce::Justification::centred);
    eqSatLabel.setFont (juce::Font (11.0f, juce::Font::bold));
    addAndMakeVisible (eqSatLabel);
    eqAtt[(size_t) HybridEQ::numBands] = std::make_unique<SliderAttachment> (apvts, "eqSaturation", eqSatSlider);

    // --- Interruptores on/off ---
    dryOnButton.setButtonText ("Original");
    delayOnButton.setButtonText ("Delay");
    eqOnButton.setButtonText ("EQ");
    addAndMakeVisible (dryOnButton);
    addAndMakeVisible (delayOnButton);
    addAndMakeVisible (eqOnButton);

    dryOnAtt   = std::make_unique<ButtonAttachment> (apvts, "dryOn",   dryOnButton);
    delayOnAtt = std::make_unique<ButtonAttachment> (apvts, "delayOn", delayOnButton);
    eqOnAtt    = std::make_unique<ButtonAttachment> (apvts, "eqOn",    eqOnButton);

    // --- Sync / estéreo / modos especiales ---
    syncButton.setButtonText ("Sync");
    pingPongButton.setButtonText ("Ping-Pong");
    freezeButton.setButtonText ("Freeze");
    addAndMakeVisible (syncButton);
    addAndMakeVisible (pingPongButton);
    addAndMakeVisible (freezeButton);

    syncAtt     = std::make_unique<ButtonAttachment> (apvts, "syncOn",   syncButton);
    pingPongAtt = std::make_unique<ButtonAttachment> (apvts, "pingPong", pingPongButton);
    freezeAtt   = std::make_unique<ButtonAttachment> (apvts, "freeze",   freezeButton);

    noteDivisionBox.addItemList (
        { "1/1", "1/2", "1/2.", "1/4", "1/4.", "1/4T", "1/8", "1/8.", "1/8T", "1/16", "1/16.", "1/16T" }, 1);
    noteDivisionBox.setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff2a2b2e));
    noteDivisionBox.setColour (juce::ComboBox::textColourId, juce::Colour (0xffe0e0e0));
    noteDivisionBox.setColour (juce::ComboBox::outlineColourId, juce::Colours::black);
    addAndMakeVisible (noteDivisionBox);
    noteDivisionAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        apvts, "noteDivision", noteDivisionBox);

    setupKnob (duckSlider, duckLabel, "Duck", *this);
    duckAtt = std::make_unique<SliderAttachment> (apvts, "duckAmount", duckSlider);

    // --- Presets de fábrica ---
    presetBox.addItem ("Sin preset", 1);
    int itemId = 2;
    for (const auto& preset : TapeDelayEQAudioProcessor::getFactoryPresets())
        presetBox.addItem (preset.name, itemId++);
    presetBox.setSelectedId (1, juce::dontSendNotification); // "Sin preset" por defecto
    presetBox.setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff2a2b2e));
    presetBox.setColour (juce::ComboBox::textColourId, juce::Colour (0xffd8863a));
    presetBox.setColour (juce::ComboBox::outlineColourId, juce::Colours::black);
    presetBox.onChange = [this]
    {
        const int id = presetBox.getSelectedId();
        if (id >= 2) // 1 = "Sin preset", no hace nada
            processorRef.applyPreset (id - 2);
    };
    addAndMakeVisible (presetBox);

    // --- Multi-tap ---
    tap2OnButton.setButtonText ("Tap 2");
    tap3OnButton.setButtonText ("Tap 3");
    addAndMakeVisible (tap2OnButton);
    addAndMakeVisible (tap3OnButton);
    tap2OnAtt = std::make_unique<ButtonAttachment> (apvts, "tap2On", tap2OnButton);
    tap3OnAtt = std::make_unique<ButtonAttachment> (apvts, "tap3On", tap3OnButton);

    setupKnob (tap2LevelSlider, tap2LevelLabel, "Tap2 Lvl",  *this);
    setupKnob (tap2RatioSlider, tap2RatioLabel, "Tap2 Time", *this);
    setupKnob (tap3LevelSlider, tap3LevelLabel, "Tap3 Lvl",  *this);
    setupKnob (tap3RatioSlider, tap3RatioLabel, "Tap3 Time", *this);
    setupKnob (diffusionSlider, diffusionLabel, "Diffusion", *this);
    tap2LevelAtt = std::make_unique<SliderAttachment> (apvts, "tap2Level", tap2LevelSlider);
    tap2RatioAtt = std::make_unique<SliderAttachment> (apvts, "tap2Ratio", tap2RatioSlider);
    tap3LevelAtt = std::make_unique<SliderAttachment> (apvts, "tap3Level", tap3LevelSlider);
    tap3RatioAtt = std::make_unique<SliderAttachment> (apvts, "tap3Ratio", tap3RatioSlider);
    diffusionAtt = std::make_unique<SliderAttachment> (apvts, "diffusionAmount", diffusionSlider);

    setSize (900, 660);
}

TapeDelayEQAudioProcessorEditor::~TapeDelayEQAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

void TapeDelayEQAudioProcessorEditor::paint (juce::Graphics& g)
{
    // Fondo tipo "metal cepillado": gradiente vertical + líneas finas
    juce::ColourGradient bg (juce::Colour (0xff35373b), 0.0f, 0.0f,
                              juce::Colour (0xff1c1d1f), 0.0f, (float) getHeight(), false);
    g.setGradientFill (bg);
    g.fillAll();

    g.setColour (juce::Colours::white.withAlpha (0.02f));
    for (int yy = 0; yy < getHeight(); yy += 3)
        g.drawHorizontalLine (yy, 0.0f, (float) getWidth());

    // Borde biselado general (efecto "caja" 3D)
    g.setColour (juce::Colours::black);
    g.drawRect (getLocalBounds(), 2);
    g.setColour (juce::Colours::white.withAlpha (0.08f));
    g.drawRect (getLocalBounds().reduced (2), 1);

    // Título grabado (efecto relieve: sombra clara abajo, oscura arriba)
    auto titleArea = juce::Rectangle<int> (0, 6, getWidth(), 26);
    g.setFont (juce::Font (18.0f, juce::Font::bold));
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.drawFittedText ("TAPE DELAY EQ", titleArea.translated (0, 1), juce::Justification::centred, 1);
    g.setColour (juce::Colour (0xffd8863a));
    g.drawFittedText ("TAPE DELAY EQ", titleArea, juce::Justification::centred, 1);

    // Paneles con bisel agrupando secciones (se recalculan igual que en resized())
    auto routingPanel = getLocalBounds().withTrimmedTop (36).removeFromTop (90).reduced (10, 4);
    drawInsetPanel (g, routingPanel.toFloat(), "Routing");

    auto syncPanel = getLocalBounds().withTrimmedTop (128).removeFromTop (64).reduced (10, 4);
    drawInsetPanel (g, syncPanel.toFloat(), "Sync / Modo");

    auto knobPanel = getLocalBounds().withTrimmedTop (196).removeFromTop (160).reduced (10, 4);
    drawInsetPanel (g, knobPanel.toFloat(), "Delay");

    auto tapsPanel = getLocalBounds().withTrimmedTop (360).removeFromTop (110).reduced (10, 4);
    drawInsetPanel (g, tapsPanel.toFloat(), "Multi-Tap / Space");

    auto eqPanel = getLocalBounds().withTrimmedTop (474).reduced (10, 4);
    drawInsetPanel (g, eqPanel.toFloat(), "EQ (solo senal de delay)");
}

void TapeDelayEQAudioProcessorEditor::resized()
{
    // --- Selector de presets, arriba a la derecha del título ---
    presetBox.setBounds (getWidth() - 174, 8, 160, 22);

    // --- Panel Routing: 2 filas, botón + slider LARGO de ganancia en cada una ---
    auto routingPanel = getLocalBounds().withTrimmedTop (36).removeFromTop (90).reduced (10, 4);
    routingPanel.removeFromTop (18); // espacio para el título del panel
    auto routingArea = routingPanel.reduced (12, 2);
    const int rowH = routingArea.getHeight() / 2;

    auto dryRow = routingArea.removeFromTop (rowH);
    dryOnButton.setBounds (dryRow.removeFromLeft (110).reduced (4));
    dryGainSlider.setBounds (dryRow.reduced (6, 10)); // el resto del ancho: mucho más recorrido
    dryGainLabel.setVisible (false);

    auto delayRow = routingArea;
    delayOnButton.setBounds (delayRow.removeFromLeft (110).reduced (4));
    wetGainSlider.setBounds (delayRow.reduced (6, 10));
    wetGainLabel.setVisible (false);

    // El botón de EQ lo dejamos flotando en la esquina del panel de EQ (ver abajo)

    // --- Panel de Sync / Modo (combo de compás más grande y legible) ---
    auto syncPanel = getLocalBounds().withTrimmedTop (128).removeFromTop (64).reduced (10, 4);
    syncPanel.removeFromTop (18); // espacio para el título del panel
    auto syncArea = syncPanel.reduced (8, 2);

    const int syncCellW = syncArea.getWidth() / 5;
    auto syncBtnCell = syncArea.removeFromLeft (syncCellW);
    syncButton.setBounds (syncBtnCell.removeFromTop (syncBtnCell.getHeight() / 2).reduced (3));

    auto noteDivCell = syncArea.removeFromLeft (syncCellW);
    noteDivisionBox.setBounds (noteDivCell.reduced (3, 4)); // más alto y más ancho que antes

    pingPongButton.setBounds (syncArea.removeFromLeft (syncCellW).reduced (3, 12));
    freezeButton.setBounds   (syncArea.removeFromLeft (syncCellW).reduced (3, 12));
    duckLabel.setBounds      (syncArea.removeFromTop (14));
    duckSlider.setBounds     (syncArea.reduced (2));

    // --- Panel de knobs del delay ---
    auto area = getLocalBounds().withTrimmedTop (196).removeFromTop (160).reduced (10, 4);
    area.removeFromTop (18); // espacio para el título del panel
    auto delayArea = area.reduced (10, 6);
    const int knobWidth = delayArea.getWidth() / 6;

    juce::Slider* sliders[] = { &delayTimeSlider, &feedbackSlider, &mixSlider,
                                 &wowSlider, &flutterSlider, &satSlider };
    juce::Label*  labels[]  = { &delayTimeLabel, &feedbackLabel, &mixLabel,
                                 &wowLabel, &flutterLabel, &satLabel };

    for (int i = 0; i < 6; ++i)
    {
        auto col = delayArea.removeFromLeft (knobWidth);
        labels[i]->setBounds (col.removeFromTop (16));
        sliders[i]->setBounds (col.reduced (4));
    }

    // --- Panel de Multi-Tap / Space (3 columnas: Tap2, Tap3, Diffusion) ---
    {
        auto col = getLocalBounds().withTrimmedTop (360).removeFromTop (110).reduced (10, 4);
        col.removeFromTop (18); // espacio para el título del panel
        auto full = col.reduced (10, 4);
        const int w = full.getWidth() / 3;

        auto c2 = full.removeFromLeft (w);
        tap2OnButton.setBounds (c2.removeFromTop (20).reduced (20, 0));
        auto c2Left = c2.removeFromLeft (c2.getWidth() / 2);
        tap2LevelLabel.setBounds (c2Left.removeFromTop (14));
        tap2LevelSlider.setBounds (c2Left.reduced (2));
        tap2RatioLabel.setBounds (c2.removeFromTop (14));
        tap2RatioSlider.setBounds (c2.reduced (2));

        auto c3 = full.removeFromLeft (w);
        tap3OnButton.setBounds (c3.removeFromTop (20).reduced (20, 0));
        auto c3Left = c3.removeFromLeft (c3.getWidth() / 2);
        tap3LevelLabel.setBounds (c3Left.removeFromTop (14));
        tap3LevelSlider.setBounds (c3Left.reduced (2));
        tap3RatioLabel.setBounds (c3.removeFromTop (14));
        tap3RatioSlider.setBounds (c3.reduced (2));

        auto cD = full;
        diffusionLabel.setBounds (cD.removeFromTop (14));
        diffusionSlider.setBounds (cD.reduced (10, 2));
    }

    // --- Panel de EQ ---
    auto eqPanel = getLocalBounds().withTrimmedTop (474).reduced (10, 4);
    eqOnButton.setBounds (eqPanel.getRight() - 90, eqPanel.getY() + 2, 84, 20);
    eqPanel.removeFromTop (24); // espacio para título + botón EQ

    auto eqArea = eqPanel.reduced (10, 6);
    const int eqWidth = eqArea.getWidth() / numEqControls;

    for (int i = 0; i < numEqControls; ++i)
    {
        auto col = eqArea.removeFromLeft (eqWidth);
        eqLabels[(size_t) i].setBounds (col.removeFromTop (14));
        eqSliders[(size_t) i].setBounds (col.reduced (2));
    }
}
