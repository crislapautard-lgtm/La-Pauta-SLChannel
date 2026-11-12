#include "PluginEditor.h"

using namespace juce;

namespace
{
    const Colour kCream (0xffd9d5c3), kRed (0xffc0392b), kGreen (0xff1e7a53),
                 kBlue (0xff1f7fb8), kBlack (0xff2b2b2b), kAmber (0xffe0a526);

    // Posiciones de la cuadrícula
    constexpr int eqX = 56, dynX = 420, cell = 76;
    constexpr int rowFilters = 50, rowHF = 150, rowHMF = 250, rowLMF = 350, rowLF = 450;

    float peakToLevel (float peak)
    {
        const float db = Decibels::gainToDecibels (peak, -60.f);
        return jlimit (0.f, 1.f, (db + 60.f) / 66.f);   // -60 dB .. +6 dB
    }
}

//==============================================================================
LaPautaSLChannelAudioProcessorEditor::LaPautaSLChannelAudioProcessorEditor (LaPautaSLChannelAudioProcessor& p)
    : AudioProcessorEditor (&p), proc (p),
      meterIn ("IN", Colour (0xff7bd34a)), meterOut ("OUT", Colour (0xff7bd34a)),
      meterComp ("COMP", Colour (0xffe0a526), true), meterGate ("GATE", Colour (0xff3aa0d8), true)
{
    setLookAndFeel (&lnf);

    // Filtros
    addKnob (ID::hpf, "HPF", kCream);   addBtn (ID::hpfOn, "HPF IN", kAmber);
    addKnob (ID::lpf, "LPF", kCream);   addBtn (ID::lpfOn, "LPF IN", kAmber);

    // EQ
    addKnob (ID::hfGain, "HF dB", kRed);    addKnob (ID::hfFreq, "HF Freq", kRed);    addBtn (ID::hfBell, "BELL", kAmber);
    addKnob (ID::hmfGain, "HMF dB", kGreen); addKnob (ID::hmfFreq, "HMF Freq", kGreen);
    addKnob (ID::hmfQ, "HMF Q", kGreen);     addBtn (ID::hmfX3, "HMF x3", kAmber);
    addKnob (ID::lmfGain, "LMF dB", kBlue);  addKnob (ID::lmfFreq, "LMF Freq", kBlue);
    addKnob (ID::lmfQ, "LMF Q", kBlue);      addBtn (ID::lmfD3, "LMF /3", kAmber);
    addKnob (ID::lfGain, "LF dB", kBlack);   addKnob (ID::lfFreq, "LF Freq", kBlack);  addBtn (ID::lfBell, "BELL", kAmber);

    // Dinamica
    addKnob (ID::compThr, "THRESHOLD", kCream); addKnob (ID::compRatio, "RATIO", kCream);
    addKnob (ID::compRel, "RELEASE", kCream);   addBtn (ID::fastAtk, "F.ATK", kAmber);
    addKnob (ID::gateThr, "THRESHOLD", kGreen); addKnob (ID::gateRange, "RANGE", kGreen);
    addKnob (ID::gateRel, "RELEASE", kGreen);   addBtn (ID::gateMode, "GATE", kAmber);

    // Ruteo
    addBtn (ID::eqByp, "EQ BYP", Colour (0xffd04a3a));
    addBtn (ID::eqSc, "FLT>DYN SC", kAmber);
    addBtn (ID::dynByp, "DYN BYP", Colour (0xffd04a3a));
    addBtn (ID::dynPost, "CH OUT", kAmber);
    addBtn (ID::analog, "ANALOG", kAmber);
    addBtn (ID::phase, "PHASE", Colour (0xffd04a3a));

    // Ganancia
    addKnob (ID::inGain, "INPUT", kCream);
    addKnob (ID::outGain, "OUTPUT", kCream);

    addAndMakeVisible (meterIn); addAndMakeVisible (meterOut);
    addAndMakeVisible (meterComp); addAndMakeVisible (meterGate);

    // Barra de presets
    addAndMakeVisible (presetBox);
    addAndMakeVisible (prevBtn); addAndMakeVisible (nextBtn); addAndMakeVisible (saveBtn);
    refreshPresetList();
    presetBox.setSelectedId (proc.getCurrentProgram() + 1, dontSendNotification);

    presetBox.onChange = [this]
    {
        const int id = presetBox.getSelectedId();
        if (id <= 0) return;
        if (id < 1000) proc.setCurrentProgram (id - 1);
        else           proc.loadUserPreset (presetBox.getText());
    };
    prevBtn.onClick = [this] { stepPreset (-1); };
    nextBtn.onClick = [this] { stepPreset (+1); };
    saveBtn.onClick = [this] { savePresetDialog(); };

    setSize (750, 565);
    startTimerHz (30);
}

LaPautaSLChannelAudioProcessorEditor::~LaPautaSLChannelAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

//==============================================================================
void LaPautaSLChannelAudioProcessorEditor::addKnob (const char* id, const String& name, Colour colour)
{
    auto k = std::make_unique<KnobCell>();
    k->s.setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    k->s.setTextBoxStyle (Slider::TextBoxBelow, false, cell, 14);
    k->s.setColour (Slider::rotarySliderFillColourId, colour);
    k->s.setColour (Slider::textBoxTextColourId, Colours::white.withAlpha (0.8f));
    k->s.setColour (Slider::textBoxOutlineColourId, Colours::transparentBlack);
    k->s.setRotaryParameters (MathConstants<float>::pi * 1.2f, MathConstants<float>::pi * 2.8f, true);

    k->l.setText (name, dontSendNotification);
    k->l.setJustificationType (Justification::centred);
    k->l.setFont (Font (FontOptions (12.f)));
    k->l.setColour (Label::textColourId, Colours::white.withAlpha (0.75f));

    k->att = std::make_unique<SliderAtt> (proc.apvts, id, k->s);
    addAndMakeVisible (k->s);
    addAndMakeVisible (k->l);
    knobs[id] = std::move (k);
}

void LaPautaSLChannelAudioProcessorEditor::addBtn (const char* id, const String& text, Colour onColour)
{
    auto b = std::make_unique<ToggleCell>();
    b->b.setButtonText (text);
    b->b.setClickingTogglesState (true);
    b->b.setColour (TextButton::buttonColourId, Colour (0xff4a4a47));
    b->b.setColour (TextButton::buttonOnColourId, onColour);
    b->b.setColour (TextButton::textColourOffId, Colours::white.withAlpha (0.85f));
    b->b.setColour (TextButton::textColourOnId, Colours::black);
    b->att = std::make_unique<ButtonAtt> (proc.apvts, id, b->b);
    addAndMakeVisible (b->b);
    buttons[id] = std::move (b);
}

void LaPautaSLChannelAudioProcessorEditor::placeKnob (const char* id, int x, int y)
{
    auto& k = *knobs[id];
    k.l.setBounds (x, y, cell, 14);
    k.s.setBounds (x, y + 14, cell, 86);
}

void LaPautaSLChannelAudioProcessorEditor::placeBtn (const char* id, int x, int y, int w, int h)
{
    buttons[id]->b.setBounds (x, y, w, h);
}

//==============================================================================
void LaPautaSLChannelAudioProcessorEditor::resized()
{
    // Barra superior
    prevBtn.setBounds (10, 10, 28, 26);
    presetBox.setBounds (42, 10, 250, 26);
    nextBtn.setBounds (296, 10, 28, 26);
    saveBtn.setBounds (330, 10, 70, 26);

    // --- EQ (columna izquierda) ---
    const int c0 = eqX, c1 = eqX + cell, c2 = eqX + 2 * cell, c3 = eqX + 3 * cell;

    placeKnob (ID::hpf, c0, rowFilters);  placeKnob (ID::lpf, c1, rowFilters);
    placeBtn (ID::hpfOn, c2 + 2, rowFilters + 24);
    placeBtn (ID::lpfOn, c2 + 2, rowFilters + 58);

    placeKnob (ID::hfGain, c0, rowHF);    placeKnob (ID::hfFreq, c1, rowHF);
    placeBtn (ID::hfBell, c2 + 2, rowHF + 40);

    placeKnob (ID::hmfGain, c0, rowHMF);  placeKnob (ID::hmfFreq, c1, rowHMF);
    placeKnob (ID::hmfQ, c2, rowHMF);     placeBtn (ID::hmfX3, c3 + 2, rowHMF + 40);

    placeKnob (ID::lmfGain, c0, rowLMF);  placeKnob (ID::lmfFreq, c1, rowLMF);
    placeKnob (ID::lmfQ, c2, rowLMF);     placeBtn (ID::lmfD3, c3 + 2, rowLMF + 40);

    placeKnob (ID::lfGain, c0, rowLF);    placeKnob (ID::lfFreq, c1, rowLF);
    placeBtn (ID::lfBell, c2 + 2, rowLF + 40);

    // --- Dinamica (columna derecha) ---
    const int d0 = dynX, d1 = dynX + cell, d2 = dynX + 2 * cell, d3 = dynX + 3 * cell;

    placeKnob (ID::compThr, d0, 66);  placeKnob (ID::compRatio, d1, 66);
    placeKnob (ID::compRel, d2, 66);  placeBtn (ID::fastAtk, d3 + 2, 106);

    placeKnob (ID::gateThr, d0, 186); placeKnob (ID::gateRange, d1, 186);
    placeKnob (ID::gateRel, d2, 186); placeBtn (ID::gateMode, d3 + 2, 226);

    // Ruteo
    placeBtn (ID::eqByp,  d0, 308, 72);  placeBtn (ID::eqSc,   d0 + 76, 308, 92);
    placeBtn (ID::dynByp, d0 + 172, 308, 72); placeBtn (ID::dynPost, d0 + 248, 308, 70);
    placeBtn (ID::analog, d0, 342, 72);  placeBtn (ID::phase, d0 + 76, 342, 72);

    // Ganancia + medidores
    placeKnob (ID::inGain, d0, 392);   placeKnob (ID::outGain, d1, 392);
    meterIn.setBounds   (d2 + 4,  388, 34, 160);
    meterOut.setBounds  (d2 + 42, 388, 34, 160);
    meterComp.setBounds (d3 + 4,  388, 34, 160);
    meterGate.setBounds (d3 + 42, 388, 34, 160);
}

void LaPautaSLChannelAudioProcessorEditor::paint (Graphics& g)
{
    g.setGradientFill (ColourGradient (Colour (0xff2f312f), 0.f, 0.f,
                                       Colour (0xff1c1d1c), 0.f, (float) getHeight(), false));
    g.fillAll();

    g.setColour (Colours::white.withAlpha (0.15f));
    g.drawLine (390.f, 46.f, 390.f, (float) getHeight() - 12.f, 1.5f);
    for (int y : { 148, 248, 348, 448 })
        g.drawLine (10.f, (float) y, 380.f, (float) y, 1.f);

    // Titulo
    g.setColour (Colours::white.withAlpha (0.9f));
    g.setFont (Font (FontOptions (22.f, Font::bold)));
    g.drawText ("LA PAUTA  SL CHANNEL", 420, 8, 320, 30, Justification::centredRight);

    // Rotulos de fila
    g.setFont (Font (FontOptions (11.f, Font::bold)));
    g.setColour (Colours::white.withAlpha (0.6f));
    auto row = [&] (const char* t, int y) { g.drawText (t, 6, y + 40, 50, 16, Justification::centred); };
    row ("FILTROS", rowFilters); row ("HF", rowHF); row ("HMF", rowHMF);
    row ("LMF", rowLMF);         row ("LF", rowLF);

    auto title = [&] (const char* t, int y) { g.drawText (t, dynX, y, 320, 16, Justification::left); };
    title ("COMPRESS", 48);  title ("EXPAND / GATE", 168);
    title ("RUTEO", 290);    title ("GAIN / METERS", 372);
}

//==============================================================================
void LaPautaSLChannelAudioProcessorEditor::timerCallback()
{
    dispIn  = jmax (peakToLevel (proc.inPeak.load()),  dispIn  - 0.02f);
    dispOut = jmax (peakToLevel (proc.outPeak.load()), dispOut - 0.02f);
    meterIn.setLevel (dispIn);
    meterOut.setLevel (dispOut);
    meterComp.setLevel (proc.compGrDb.load() / 20.f);
    meterGate.setLevel (proc.gateGrDb.load() / 40.f);
}

//==============================================================================
void LaPautaSLChannelAudioProcessorEditor::refreshPresetList()
{
    presetBox.clear (dontSendNotification);
    presetBox.addSectionHeading ("Fabrica");
    for (int i = 0; i < proc.getNumPrograms(); ++i)
        presetBox.addItem (proc.getProgramName (i), i + 1);

    auto users = proc.getUserPresetNames();
    if (users.size() > 0)
    {
        presetBox.addSeparator();
        presetBox.addSectionHeading ("Usuario");
        for (int j = 0; j < users.size(); ++j)
            presetBox.addItem (users[j], 1000 + j + 1);
    }
}

void LaPautaSLChannelAudioProcessorEditor::stepPreset (int delta)
{
    const int count = proc.getNumPrograms();
    const int next = (proc.getCurrentProgram() + delta + count) % count;
    proc.setCurrentProgram (next);
    presetBox.setSelectedId (next + 1, dontSendNotification);
}

void LaPautaSLChannelAudioProcessorEditor::savePresetDialog()
{
    auto* w = new AlertWindow ("Guardar preset", "Nombre del preset:", MessageBoxIconType::NoIcon);
    w->addTextEditor ("name", "Mi preset");
    w->addButton ("Guardar", 1, KeyPress (KeyPress::returnKey));
    w->addButton ("Cancelar", 0, KeyPress (KeyPress::escapeKey));

    Component::SafePointer<LaPautaSLChannelAudioProcessorEditor> safe (this);
    w->enterModalState (true, ModalCallbackFunction::create ([safe, w] (int result)
    {
        if (result == 1 && safe != nullptr)
        {
            const auto name = w->getTextEditorContents ("name").trim();
            if (name.isNotEmpty())
            {
                safe->proc.saveUserPreset (name);
                safe->refreshPresetList();
                safe->presetBox.setText (name, dontSendNotification);
            }
        }
    }), true);
}
