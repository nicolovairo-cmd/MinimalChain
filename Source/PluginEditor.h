#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class MinimalChainAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit MinimalChainAudioProcessorEditor(MinimalChainAudioProcessor&);
    ~MinimalChainAudioProcessorEditor() override = default;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    MinimalChainAudioProcessor& processor;

    struct Knob
    {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };

    std::vector<std::unique_ptr<Knob>> knobs;

    void addKnob(const juce::String& id, const juce::String& name);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MinimalChainAudioProcessorEditor)
};
