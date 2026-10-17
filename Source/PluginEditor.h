// Atelier Instruments - Copyright (c) 2026 Amirouche Amwanes - AGPL-3.0-or-later
#pragma once
#include "PluginProcessor.h"

class AtelierLookAndFeel : public juce::LookAndFeel_V4
{
public:
    AtelierLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos, float start, float end, juce::Slider&) override;
    void drawLinearSlider (juce::Graphics&, int x, int y, int w, int h, float pos, float minPos, float maxPos, juce::Slider::SliderStyle, juce::Slider&) override;
    juce::Colour accent;
};

class AtelierEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit AtelierEditor (AtelierProcessor&);
    ~AtelierEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    struct Knob
    {
        std::unique_ptr<juce::Slider> slider;
        std::unique_ptr<juce::Label> label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };
    void addKnob (std::vector<Knob>& list, const juce::String& id, const juce::String& text, bool vertical);

    AtelierProcessor& proc;
    AtelierLookAndFeel lnf;
    std::vector<Knob> knobs, faders;
    std::unique_ptr<juce::MidiKeyboardComponent> keyboard;
    juce::OwnedArray<juce::TextButton> pads;
    juce::Label status;
    float meter = 0;
};
