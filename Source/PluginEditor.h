#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
class MinimalChainAudioProcessorEditor:public juce::AudioProcessorEditor{
public: explicit MinimalChainAudioProcessorEditor(MinimalChainAudioProcessor&);
void paint(juce::Graphics&)override;void resized()override;
private: MinimalChainAudioProcessor& p;struct K{juce::Slider s;juce::Label l;std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>a;};std::vector<std::unique_ptr<K>> ks;void add(const juce::String&,const juce::String&);JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MinimalChainAudioProcessorEditor)};
