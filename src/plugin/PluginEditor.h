#ifndef PLUGIN_EDITOR_H
#define PLUGIN_EDITOR_H

#include "PluginProcessor.h"

class NoiseReducerAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    NoiseReducerAudioProcessorEditor (NoiseReducerAudioProcessor&);
    ~NoiseReducerAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    NoiseReducerAudioProcessor& audioProcessor;

    juce::ComboBox m_modeComboBox;
    juce::Slider m_thresholdSlider;
    juce::Slider m_intensitySlider;
    juce::Slider m_dryWetSlider;
    juce::ToggleButton m_learnButton;

    juce::Label m_modeLabel;
    juce::Label m_thresholdLabel;
    juce::Label m_intensityLabel;
    juce::Label m_dryWetLabel;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> m_modeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> m_thresholdAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> m_intensityAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> m_dryWetAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> m_learnAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NoiseReducerAudioProcessorEditor)
};

#endif // PLUGIN_EDITOR_H
