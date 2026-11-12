#pragma once
#include <map>
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

//==============================================================================
// Perillas propias (estilo consola con código de color por banda).
class LaPautaLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider& s) override
    {
        auto b = juce::Rectangle<int> (x, y, w, h).toFloat().reduced (5.f);
        const float d = juce::jmin (b.getWidth(), b.getHeight());
        const auto c = b.getCentre();
        const float r = d * 0.5f;

        g.setColour (juce::Colours::black.withAlpha (0.55f));
        g.fillEllipse (c.x - r + 2.f, c.y - r + 3.f, d, d);

        const auto base = s.findColour (juce::Slider::rotarySliderFillColourId);
        g.setGradientFill (juce::ColourGradient (base.brighter (0.35f), c.x - r * 0.4f, c.y - r * 0.5f,
                                                 base.darker (0.5f), c.x + r * 0.6f, c.y + r * 0.8f, true));
        g.fillEllipse (c.x - r, c.y - r, d, d);

        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.drawEllipse (c.x - r, c.y - r, d, d, 1.5f);

        const float a = startAngle + pos * (endAngle - startAngle);
        const juce::Point<float> dir (std::sin (a), -std::cos (a));
        g.setColour (juce::Colours::white.withAlpha (0.95f));
        g.drawLine (juce::Line<float> (c + dir * (r * 0.2f), c + dir * (r * 0.88f)), 3.f);
    }
};

//==============================================================================
class MeterBar : public juce::Component
{
public:
    MeterBar (juce::String n, juce::Colour c, bool fromTop = false)
        : name (std::move (n)), colour (c), downward (fromTop) {}

    void setLevel (float v01)
    {
        v01 = juce::jlimit (0.f, 1.f, v01);
        if (std::abs (v01 - level) > 0.002f) { level = v01; repaint(); }
    }

    void paint (juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat();
        auto label = b.removeFromBottom (16.f);
        g.setColour (juce::Colour (0xff0e0f0e));
        g.fillRoundedRectangle (b, 2.f);

        const float h = b.getHeight() * level;
        g.setColour (colour);
        if (downward) g.fillRect (b.withHeight (h));
        else          g.fillRect (b.withTop (b.getBottom() - h));

        g.setColour (juce::Colours::white.withAlpha (0.75f));
        g.setFont (11.f);
        g.drawText (name, label, juce::Justification::centred);
    }

private:
    juce::String name;
    juce::Colour colour;
    bool downward;
    float level = 0.f;
};

//==============================================================================
class LaPautaSLChannelAudioProcessorEditor : public juce::AudioProcessorEditor,
                                             private juce::Timer
{
public:
    explicit LaPautaSLChannelAudioProcessorEditor (LaPautaSLChannelAudioProcessor&);
    ~LaPautaSLChannelAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using SliderAtt = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAtt = juce::AudioProcessorValueTreeState::ButtonAttachment;

    struct KnobCell   { juce::Slider s; juce::Label l; std::unique_ptr<SliderAtt> att; };
    struct ToggleCell { juce::TextButton b; std::unique_ptr<ButtonAtt> att; };

    void addKnob (const char* id, const juce::String& name, juce::Colour colour);
    void addBtn (const char* id, const juce::String& text, juce::Colour onColour);
    void placeKnob (const char* id, int x, int y);
    void placeBtn (const char* id, int x, int y, int w = 70, int h = 26);

    void refreshPresetList();
    void savePresetDialog();
    void stepPreset (int delta);
    void timerCallback() override;

    LaPautaSLChannelAudioProcessor& proc;
    LaPautaLookAndFeel lnf;

    std::map<juce::String, std::unique_ptr<KnobCell>> knobs;
    std::map<juce::String, std::unique_ptr<ToggleCell>> buttons;

    juce::ComboBox presetBox;
    juce::TextButton prevBtn { "<" }, nextBtn { ">" }, saveBtn { "Guardar" };

    MeterBar meterIn, meterOut, meterComp, meterGate;
    float dispIn = 0.f, dispOut = 0.f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LaPautaSLChannelAudioProcessorEditor)
};
