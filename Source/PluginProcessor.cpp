#include "PluginProcessor.h"
#include "PluginEditor.h"

MinimalChainAudioProcessor::MinimalChainAudioProcessor()
    : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
                                      .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "PARAMETERS", createParameterLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout MinimalChainAudioProcessor::createParameterLayout()
{
    using P = juce::AudioParameterFloat;
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;

    p.push_back(std::make_unique<P>("delayTime", "Delay Time", juce::NormalisableRange<float>(1.0f, 2000.0f, 0.1f), 350.0f, " ms"));
    p.push_back(std::make_unique<P>("delayFeedback", "Delay Feedback", 0.0f, 0.95f, 0.35f));
    p.push_back(std::make_unique<P>("delayMix", "Delay Mix", 0.0f, 1.0f, 0.25f));

    p.push_back(std::make_unique<P>("lpCutoff", "Low Pass Cutoff", juce::NormalisableRange<float>(100.0f, 20000.0f, 1.0f, 0.25f), 12000.0f, " Hz"));
    p.push_back(std::make_unique<P>("lpRes", "Low Pass Resonance", 0.1f, 1.0f, 0.707f));

    p.push_back(std::make_unique<P>("hpCutoff", "High Pass Cutoff", juce::NormalisableRange<float>(20.0f, 5000.0f, 1.0f, 0.3f), 40.0f, " Hz"));
    p.push_back(std::make_unique<P>("hpRes", "High Pass Resonance", 0.1f, 1.0f, 0.707f));

    p.push_back(std::make_unique<P>("distTone", "Distortion Tone", 0.0f, 1.0f, 0.5f));
    p.push_back(std::make_unique<P>("distAmount", "Distortion Amount", 0.0f, 1.0f, 0.15f));
    p.push_back(std::make_unique<P>("distMix", "Distortion Mix", 0.0f, 1.0f, 0.35f));

    p.push_back(std::make_unique<P>("compInput", "Compressor Input", 0.0f, 1.0f, 0.5f));
    p.push_back(std::make_unique<P>("compPeak", "Peak Reduction", 0.0f, 1.0f, 0.25f));

    p.push_back(std::make_unique<P>("output", "Output", juce::NormalisableRange<float>(-24.0f, 12.0f, 0.01f), 0.0f, " dB"));
    return { p.begin(), p.end() };
}

void MinimalChainAudioProcessor::prepareToPlay(double sr, int samplesPerBlock)
{
    sampleRate = (float)sr;
    delay.setMaximumDelayInSamples((int)std::ceil(sr * 2.1));
    delay.reset();
    lpf.reset();
    hpf.reset();
    compEnvelope = 0.0f;
    compGain = 1.0f;

    juce::dsp::ProcessSpec spec { sr, (juce::uint32)samplesPerBlock, 2 };
    lpf.prepare(spec);
    hpf.prepare(spec);
    lpf.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
    hpf.setType(juce::dsp::StateVariableTPTFilterType::highpass);
}

bool MinimalChainAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    return layouts.getMainInputChannelSet() == juce::AudioChannelSet::mono()
        || layouts.getMainInputChannelSet() == juce::AudioChannelSet::stereo();
}

void MinimalChainAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    const int ch = buffer.getNumChannels();

    const float dTime = apvts.getRawParameterValue("delayTime")->load();
    const float dFb   = apvts.getRawParameterValue("delayFeedback")->load();
    const float dMix  = apvts.getRawParameterValue("delayMix")->load();

    const float lpF = apvts.getRawParameterValue("lpCutoff")->load();
    const float lpR = apvts.getRawParameterValue("lpRes")->load();
    const float hpF = apvts.getRawParameterValue("hpCutoff")->load();
    const float hpR = apvts.getRawParameterValue("hpRes")->load();

    const float tone = apvts.getRawParameterValue("distTone")->load();
    const float amt  = apvts.getRawParameterValue("distAmount")->load();
    const float dmix = apvts.getRawParameterValue("distMix")->load();

    const float cin  = apvts.getRawParameterValue("compInput")->load();
    const float peak = apvts.getRawParameterValue("compPeak")->load();
    const float outDb = apvts.getRawParameterValue("output")->load();

    const int delaySamples = juce::jlimit(1, (int)(sampleRate * 2.0f), (int)(dTime * 0.001f * sampleRate));
    lpf.setCutoffFrequency(lpF); lpf.setResonance(lpR);
    hpf.setCutoffFrequency(hpF); hpf.setResonance(hpR);

    for (int i = 0; i < n; ++i)
    {
        float mono = 0.0f;
        for (int c = 0; c < ch; ++c) mono += buffer.getSample(c, i);
        mono /= (float)juce::jmax(1, ch);

        for (int c = 0; c < ch; ++c)
        {
            float x = buffer.getSample(c, i);

            const float delayed = delay.popSample((int)c, (float)delaySamples);
            delay.pushSample((int)c, x + delayed * dFb);
            x = x * (1.0f - dMix) + delayed * dMix;

            buffer.setSample(c, i, x);
        }
    }

    juce::dsp::AudioBlock<float> block(buffer);
    lpf.process(juce::dsp::ProcessContextReplacing<float>(block));
    hpf.process(juce::dsp::ProcessContextReplacing<float>(block));

    for (int c = 0; c < ch; ++c)
    {
        auto* data = buffer.getWritePointer(c);
        for (int i = 0; i < n; ++i)
        {
            const float dry = data[i];
            // Soft clip, with a tone-dependent pre/post emphasis.
            const float pre = dry * (1.0f + amt * 8.0f);
            float wet = std::tanh(pre);
            wet = wet * (0.65f + tone * 0.35f);
            data[i] = dry * (1.0f - dmix) + wet * dmix;
        }
    }

    // LA-2A-inspired optical compressor:
    // fixed program-dependent attack/release, user controls only Input + Peak Reduction.
    const float inputGain = juce::Decibels::decibelsToGain(juce::jmap(cin, 0.0f, 1.0f, -12.0f, 18.0f));
    const float targetGR = peak * 18.0f;
    const float attackCoeff = std::exp(-1.0f / (0.010f * sampleRate));
    const float releaseCoeff = std::exp(-1.0f / (0.35f * sampleRate));

    for (int i = 0; i < n; ++i)
    {
        float peakIn = 0.0f;
        for (int c = 0; c < ch; ++c) peakIn = juce::jmax(peakIn, std::abs(buffer.getSample(c, i) * inputGain));

        const float db = juce::Decibels::gainToDecibels(peakIn + 1.0e-9f);
        const float desired = juce::jmax(0.0f, db - targetGR);
        const float desiredGR = juce::jmax(0.0f, db - desired);

        if (desiredGR > compEnvelope)
            compEnvelope = attackCoeff * compEnvelope + (1.0f - attackCoeff) * desiredGR;
        else
            compEnvelope = releaseCoeff * compEnvelope + (1.0f - releaseCoeff) * desiredGR;

        const float grGain = juce::Decibels::decibelsToGain(-compEnvelope * 0.75f);

        for (int c = 0; c < ch; ++c)
            buffer.setSample(c, i, buffer.getSample(c, i) * inputGain * grGain);
    }

    const float outGain = juce::Decibels::decibelsToGain(outDb);
    buffer.applyGain(outGain);
}

void MinimalChainAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    if (auto xml = state.createXml())
        copyXmlToBinary(*xml, destData);
}

void MinimalChainAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary(data, sizeInBytes))
        if (xml->hasTagName(apvts.state.getType()))
            apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessorEditor* MinimalChainAudioProcessor::createEditor()
{
    return new MinimalChainAudioProcessorEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MinimalChainAudioProcessor();
}
