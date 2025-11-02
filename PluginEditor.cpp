#include "PluginProcessor.h"
#include "juce_audio_utils/juce_audio_utils.h"
#include "juce_events/juce_events.h"
#include "juce_graphics/juce_graphics.h"
#include "PluginEditor.h"

//==============================================================================
AudioPluginAudioProcessorEditor::AudioPluginAudioProcessorEditor(AudioPluginAudioProcessor &p)
    : AudioProcessorEditor(&p), processorRef(p),
      keyboardComponent(processorRef.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    juce::ignoreUnused(processorRef);

    addAndMakeVisible(keyboardComponent);
    processorRef.keyboardState.addListener(this);

    candidateRow.resize(numrow_elements);
    for (int i = 0; i < numrow_elements; ++i)
        candidateRow[i] = i;
    rowEntryComponent.steps = candidateRow;
    transposeSlider.setNumDecimalPlacesToDisplay(0);
    transposeSlider.setRange(0, numrow_elements - 1, 1);
    transposeSlider.onValueChange = [this]() { doTransform(); };
    addAndMakeVisible(transposeSlider);
    addAndMakeVisible(transformedRowComponent);
    addAndMakeVisible(rowEntryComponent);
    rowEntryComponent.setNumActiveSteps(numrow_elements);
    rowEntryComponent.readonly = false;

    transformedRowComponent.setNumActiveSteps(numrow_elements);
    transformedRowComponent.steps = rowEntryComponent.steps;

    invertButton.setButtonText("Invert");
    invertButton.onClick = [this]() { doTransform(); };
    addAndMakeVisible(invertButton);

    reverseButton.setButtonText("Reverse");
    reverseButton.onClick = [this]() { doTransform(); };
    addAndMakeVisible(reverseButton);

    rowEntryComponent.OnEdited = [this]() { doTransform(); };

    setSize(700, 550);
}

AudioPluginAudioProcessorEditor::~AudioPluginAudioProcessorEditor()
{
    processorRef.keyboardState.removeListener(this);
}

void AudioPluginAudioProcessorEditor::handleNoteOn(juce::MidiKeyboardState *source, int midiChannel,
                                                   int midiNoteNumber, float velocity)
{
    if (midiNoteNumber >= 60 && midiNoteNumber < 76)
    {
        transposeSlider.setValue(midiNoteNumber - 60);
    }
    if (midiNoteNumber == 58)
    {
        invertButton.triggerClick();
    }
    if (midiNoteNumber == 59)
    {
        reverseButton.triggerClick();
    }
}

void AudioPluginAudioProcessorEditor::updateRowSliders() {}

void AudioPluginAudioProcessorEditor::doTransform()
{
    transformedRowComponent.steps =
        RowEngine::transform_row(rowEntryComponent.steps, transposeSlider.getValue(),
                                 invertButton.getToggleState(), reverseButton.getToggleState());
    transformedRowComponent.repaint();
}

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
    reverseButton.setBounds(invertButton.getRight() + 2, transposeSlider.getBottom(), 80, 24);
    keyboardComponent.setBounds(1, reverseButton.getBottom(), getWidth() - 2, 50);
    transformedRowComponent.setBounds(1, getBottom() - 150, getWidth() - 2, 149);
}
