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
    rowEntryComponent.steps = candidateRow;
    transposeSlider.setNumDecimalPlacesToDisplay(0);
    transposeSlider.setRange(0, numrow_elements - 1, 1);
    transposeSlider.onValueChange = [this]() {
        transformedRowComponent.steps =
            RowEngine::transpose_row(rowEntryComponent.steps, transposeSlider.getValue());
        transformedRowComponent.repaint();
    };
    addAndMakeVisible(transposeSlider);
    addAndMakeVisible(transformedRowComponent);
    addAndMakeVisible(rowEntryComponent);
    rowEntryComponent.setNumActiveSteps(numrow_elements);
    transformedRowComponent.setNumActiveSteps(numrow_elements);
    transformedRowComponent.steps = rowEntryComponent.steps;
    invertButton.setButtonText("Invert");
    invertButton.onClick = [this]() {
        if (invertButton.getToggleState())
            transformedRowComponent.steps = RowEngine::invert_row(rowEntryComponent.steps);
        else
            transformedRowComponent.steps = rowEntryComponent.steps;
        transformedRowComponent.repaint();
    };
    addAndMakeVisible(invertButton);
    setSize(700, 550);
}

AudioPluginAudioProcessorEditor::~AudioPluginAudioProcessorEditor() {}

void AudioPluginAudioProcessorEditor::updateRowSliders() {}

void AudioPluginAudioProcessorEditor::updateAndValidateRow()
{
    for (int i = 0; i < numrow_elements; ++i)
    {
        candidateRow[i] = rowEntryComponent.steps[i];
    }
    rowValid = processorRef.reng.validate_row(candidateRow);
    repaint();
}

//==============================================================================
void AudioPluginAudioProcessorEditor::paint(juce::Graphics &g)
{
    g.fillAll(juce::Colours::darkgrey);
    return;
    g.setColour(juce::Colours::white);
    g.setFont(15.0f);
    g.drawFittedText("Hello World!", getLocalBounds(), juce::Justification::centred, 1);
}

void AudioPluginAudioProcessorEditor::resized()
{
    rowEntryComponent.setBounds(1, 1, getWidth() - 2, 149);
    transposeSlider.setBounds(1, rowEntryComponent.getBottom() + 1, getWidth() - 2, 25);
    invertButton.setBounds(1, transposeSlider.getBottom(), 80, 24);
    transformedRowComponent.setBounds(1, getBottom() - 150, getWidth() - 2, 149);
}
