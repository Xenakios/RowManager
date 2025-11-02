#include "PluginProcessor.h"
#include "juce_events/juce_events.h"
#include "juce_graphics/juce_graphics.h"
#include "PluginEditor.h"

//==============================================================================
AudioPluginAudioProcessorEditor::AudioPluginAudioProcessorEditor(AudioPluginAudioProcessor &p)
    : AudioProcessorEditor(&p), processorRef(p)
{
    juce::ignoreUnused(processorRef);
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    candidateRow.resize(numrow_elements);
    for (int i = 0; i < numrow_elements; ++i)
        candidateRow[i] = i;
    for (int i = 0; i < numrow_elements; ++i)
    {
        auto slid = std::make_unique<juce::Slider>();
        slid->setNumDecimalPlacesToDisplay(0);
        slid->setRange(0, numrow_elements - 1, 1.0);
        slid->setValue(candidateRow[i], juce::sendNotificationAsync);
        slid->onValueChange = [this]() { updateAndValidateRow(); };
        addAndMakeVisible(slid.get());
        rowsliders.push_back(std::move(slid));
    }
    transposeSlider.setNumDecimalPlacesToDisplay(1);
    transposeSlider.setRange(0, numrow_elements - 1, 1);
    transposeSlider.onValueChange = [this]() {
        processorRef.reng.transpose_row(candidateRow, transposeSlider.getValue());
        updateRowSliders();
    };
    addAndMakeVisible(transposeSlider);
    setSize(500, 450);
}

AudioPluginAudioProcessorEditor::~AudioPluginAudioProcessorEditor() {}

void AudioPluginAudioProcessorEditor::updateRowSliders()
{
    for (int i = 0; i < numrow_elements; ++i)
    {
        rowsliders[i]->setValue(candidateRow[i], juce::dontSendNotification);
    }
}

void AudioPluginAudioProcessorEditor::updateAndValidateRow()
{
    for (int i = 0; i < numrow_elements; ++i)
    {
        candidateRow[i] = rowsliders[i]->getValue();
    }
    rowValid = processorRef.reng.validate_row(candidateRow);
    repaint();
}

//==============================================================================
void AudioPluginAudioProcessorEditor::paint(juce::Graphics &g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    if (rowValid)
        g.fillAll(juce::Colours::green.darker());
    else
        g.fillAll(juce::Colours::red.darker());

    return;
    g.setColour(juce::Colours::white);
    g.setFont(15.0f);
    g.drawFittedText("Hello World!", getLocalBounds(), juce::Justification::centred, 1);
}

void AudioPluginAudioProcessorEditor::resized()
{
    for (int i = 0; i < rowsliders.size(); ++i)
    {
        rowsliders[i]->setBounds(1, 1 + 25 * i, getWidth() - 2, 23);
    }
    transposeSlider.setBounds(1, getBottom() - 25, getWidth() - 2, 23);
}
