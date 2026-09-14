#include "PluginProcessor.h"
#include "PluginEditor.h"

NoiseReducerAudioProcessorEditor::NoiseReducerAudioProcessorEditor (NoiseReducerAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setSize (500, 320);

    // Modo Selector ComboBox
    m_modeComboBox.addItem("Noise Gate", 1);
    m_modeComboBox.addItem("Spectral Subtraction", 2);
    m_modeComboBox.addItem("Wiener Filter", 3);
    addAndMakeVisible(m_modeComboBox);

    m_modeLabel.setText("Algorithm Mode:", juce::dontSendNotification);
    m_modeLabel.attachToComponent(&m_modeComboBox, false);
    addAndMakeVisible(m_modeLabel);

    // Sliders
    auto setupSlider = [this](juce::Slider& slider, juce::Label& label, const juce::String& text) {
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 20);
        addAndMakeVisible(slider);
        label.setText(text, juce::dontSendNotification);
        label.attachToComponent(&slider, false);
        addAndMakeVisible(label);
    };

    setupSlider(m_thresholdSlider, m_thresholdLabel, "Threshold (dB)");
    setupSlider(m_intensitySlider, m_intensityLabel, "Intensity");
    setupSlider(m_dryWetSlider, m_dryWetLabel, "Dry / Wet");

    // Learn Toggle Button
    m_learnButton.setButtonText("Learn Noise Profile");
    addAndMakeVisible(m_learnButton);

    // Attachments con APVTS
    m_modeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.getAPVTS(), "mode", m_modeComboBox);
    m_thresholdAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.getAPVTS(), "threshold", m_thresholdSlider);
    m_intensityAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.getAPVTS(), "intensity", m_intensitySlider);
    m_dryWetAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.getAPVTS(), "drywet", m_dryWetSlider);
    m_learnAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(audioProcessor.getAPVTS(), "learn", m_learnButton);
}

NoiseReducerAudioProcessorEditor::~NoiseReducerAudioProcessorEditor()
{
}

void NoiseReducerAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff1e1e24)); // Fondo oscuro moderno

    g.setColour (juce::Colours::white);
    g.setFont (juce::FontOptions (20.0f, juce::Font::bold));
    g.drawFittedText ("Dynamic Noise Reducer", getLocalBounds().removeFromTop(40), juce::Justification::centred, 1);

    g.setColour (juce::Colour (0xff3a3a44));
    g.drawRect (getLocalBounds().reduced(10), 2);
}

void NoiseReducerAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(20);
    area.removeFromTop(30); // Título

    // Selector de modo arriba
    auto topRow = area.removeFromTop(50);
    m_modeComboBox.setBounds(topRow.removeFromRight(220).reduced(5));

    // Botón de aprendizaje abajo
    auto bottomRow = area.removeFromBottom(40);
    m_learnButton.setBounds(bottomRow.removeFromRight(180).reduced(5));

    // Sliders en el centro
    auto sliderArea = area;
    int sliderWidth = sliderArea.getWidth() / 3;

    m_thresholdSlider.setBounds(sliderArea.removeFromLeft(sliderWidth).reduced(10));
    m_intensitySlider.setBounds(sliderArea.removeFromLeft(sliderWidth).reduced(10));
    m_dryWetSlider.setBounds(sliderArea.reduced(10));
}
