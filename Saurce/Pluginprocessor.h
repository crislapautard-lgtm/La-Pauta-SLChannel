#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "Dsp.h"
#include "Params.h"

class LaPautaSLChannelAudioProcessor : public juce::AudioProcessor
{
public:
    LaPautaSLChannelAudioProcessor();
    ~LaPautaSLChannelAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "La Pauta SL Channel"; }
    bool acceptsMidi() const override  { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    // Programas = presets de fábrica (visibles desde el host)
    int getNumPrograms() override { return (int) factoryPresets().size(); }
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // Presets de usuario (archivos .lpsl en la carpeta de datos del usuario)
    void applyFactoryPreset (int index);
    void saveUserPreset (const juce::String& name);
    bool loadUserPreset (const juce::String& name);
    juce::StringArray getUserPresetNames() const;
    static juce::File getUserPresetDir();

    juce::AudioProcessorValueTreeState apvts;

    // Medidores (los lee el editor)
    std::atomic<float> inPeak { 0.f }, outPeak { 0.f }, compGrDb { 0.f }, gateGrDb { 0.f };

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    double fs = 44100.0;
    int currentProgram = 0;
    float prevIn = 1.f, prevOut = 1.f;
    uint32_t noiseState = 22222u;

    lp::EqChain eq, scEq;
    lp::Dynamics dyn;
    juce::AudioBuffer<float> scBuf;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LaPautaSLChannelAudioProcessor)
};
