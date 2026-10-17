// Atelier Instruments - Copyright (c) 2026 Amirouche Amwanes - AGPL-3.0-or-later
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include "SamplerEngine.h"

namespace atelier
{
enum InstrumentId { Piano = 0, Drums = 1, Strings = 2 };
constexpr int thisInstrument = ATELIER_INSTRUMENT;
juce::StringArray drumBusNames();
}

class AtelierProcessor : public juce::AudioProcessor, private juce::Thread
{
public:
    AtelierProcessor();
    ~AtelierProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 3.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return "Default"; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;
    juce::MidiKeyboardState keyboardState;
    juce::String getStatus() const { const juce::ScopedLock sl (statusLock); return status; }
    std::atomic<bool> ready { false };
    std::atomic<int> voicesPlaying { 0 };
    std::atomic<float> meterLevel { 0.0f };

    static juce::File findInstrumentFolder();

private:
    void run() override; // background sample loading
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    atelier::SamplerEngine engine;
    juce::Reverb reverb;
    juce::CriticalSection statusLock;
    juce::String status { "Chargement des echantillons..." };
    juce::SmoothedValue<float> masterSmoothed;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AtelierProcessor)
};
