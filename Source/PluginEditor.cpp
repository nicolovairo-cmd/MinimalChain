#include "PluginEditor.h"

MinimalChainAudioProcessorEditor::MinimalChainAudioProcessorEditor(MinimalChainAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    addKnob("delayTime", "TIME");
    addKnob("delayFeedback", "FEEDBACK");
    addKnob("delayMix", "MIX");
    addKnob("lpCutoff", "LP CUT");
    addKnob("lpRes", "LP RES");
    addKnob("hpCutoff", "HP CUT");
    addKnob("hpRes", "HP RES");
    addKnob("distTone", "TONE");
    addKnob("distAmount", "AMOUNT");
    addKnob("distMix", "MIX");
    addKnob("compInput", "INPUT");
    addKnob("compPeak", "PEAK REDUCTION");
    addKnob("output", "OUTPUT");

    setSize(900, 430);
}

void MinimalChainAudioProcessorEditor::addKnob(const juce::String& id, const juce::String& name)
{
    auto k = std::make_unique<Knob>();
    k->slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    k->slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 90, 20);
    k->slider.setDoubleClickReturnValue(true, true);
    k->label.setText(name, juce::dontSendNotification);
    k->label.setJustificationType(juce::Justification::centred);
    k->attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processor.apvts, id, k->slider);

    addAndMakeVisible(k->slider);
    addAndMakeVisible(k->label);
    knobs.push_back(std::move(k));
}

void MinimalChainAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff111111));
    g.setColour(juce::Colours::white);
    g.setFont(24.0f);
    g.drawText("MINIMAL CHAIN", 30, 18, 400, 30, juce::Justification::left);

    g.setFont(11.0f);
    g.setColour(juce::Colours::grey);
    g.drawText("DELAY", 30, 65, 150, 20, juce::Justification::left);
    g.drawText("FILTER", 240, 65, 150, 20, juce::Justification::left);
    g.drawText("DISTORTION", 450, 65, 150, 20, juce::Justification::left);
    g.drawText("COMPRESSOR", 660, 65, 150, 20, juce::Justification::left);
}

void MinimalChainAudioProcessorEditor::resized()
{
    const int x0 = 20, y = 95, w = 64, h = 125;
    const int gap = 8;

    for (size_t i = 0; i < knobs.size(); ++i)
    {
        const int x = x0 + (int)i * (w + gap);
        knobs[i]->slider.setBounds(x, y, w, 85);
        knobs[i]->label.setBounds(x - 4, y + 88, w + 8, 32);
    }
}
