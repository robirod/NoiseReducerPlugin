#include "PluginProcessor.h"
#include "PluginEditor.h"

NoiseReducerAudioProcessor::NoiseReducerAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
       m_apvts(*this, nullptr, "Parameters", createParameterLayout())
#else
     : m_apvts(*this, nullptr, "Parameters", createParameterLayout())
#endif
{
}

NoiseReducerAudioProcessor::~NoiseReducerAudioProcessor()
{
}

juce::AudioProcessorValueTreeState::ParameterLayout NoiseReducerAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{"mode", 1}, "Algorithm Mode",
        juce::StringArray{"Noise Gate", "Spectral Subtraction", "Wiener Filter"}, 1));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"threshold", 1}, "Threshold (dB)",
        juce::NormalisableRange<float>(-60.0f, 0.0f, 0.1f), -30.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"intensity", 1}, "Reduction Intensity",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 1.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"drywet", 1}, "Dry / Wet",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 1.0f));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{"learn", 1}, "Learn Noise Profile", false));

    return { params.begin(), params.end() };
}

void NoiseReducerAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    int numChannels = getNumInputChannels();
    m_dspCores.resize(std::max(1, numChannels));

    for (auto& core : m_dspCores) {
        core.prepare(sampleRate, static_cast<size_t>(samplesPerBlock));
    }
}

void NoiseReducerAudioProcessor::releaseResources()
{
    for (auto& core : m_dspCores) {
        core.reset();
    }
}

bool NoiseReducerAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;

    return true;
}

void NoiseReducerAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    int mode = static_cast<int>(*m_apvts.getRawParameterValue("mode"));
    float threshold = *m_apvts.getRawParameterValue("threshold");
    float intensity = *m_apvts.getRawParameterValue("intensity");
    float drywet = *m_apvts.getRawParameterValue("drywet");
    bool learn = *m_apvts.getRawParameterValue("learn") > 0.5f;

    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        if (static_cast<size_t>(channel) < m_dspCores.size()) {
            auto& core = m_dspCores[channel];
            core.setAlgorithm(static_cast<NoiseReducer::ReductionAlgorithm>(mode));
            core.setThresholdDb(threshold);
            core.setIntensity(intensity);
            core.setDryWet(drywet);
            core.setNoiseLearn(learn);

            float* channelData = buffer.getWritePointer (channel);
            core.processBuffer(channelData, channelData, static_cast<size_t>(buffer.getNumSamples()));
        }
    }
}

juce::AudioProcessorEditor* NoiseReducerAudioProcessor::createEditor()
{
    return new NoiseReducerAudioProcessorEditor (*this);
}

void NoiseReducerAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = m_apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void NoiseReducerAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName (m_apvts.state.getType()))
            m_apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new NoiseReducerAudioProcessor();
}
