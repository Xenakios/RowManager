#include "PluginProcessor.h"
#include "juce_audio_utils/juce_audio_utils.h"
#include "juce_events/juce_events.h"
#include "juce_graphics/juce_graphics.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include "row_engine.h"
#include "PluginEditor.h"

//==============================================================================
AudioPluginAudioProcessorEditor::AudioPluginAudioProcessorEditor(AudioPluginAudioProcessor &p)
    : AudioProcessorEditor(&p), processorRef(p),
      keyboardComponent(processorRef.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    addAndMakeVisible(keyboardComponent);
    processorRef.keyboardState.addListener(this);

    rowComponents.push_back(std::make_unique<RowComponent>("Pitch Class", RID_PITCHCLASS,
                                                           processorRef.rows[RID_PITCHCLASS]));
    rowComponents.push_back(
        std::make_unique<RowComponent>("Octave", RID_OCTAVE, processorRef.rows[RID_OCTAVE]));

    rowComponents.push_back(
        std::make_unique<RowComponent>("Velocity", RID_VELOCITY, processorRef.rows[RID_VELOCITY]));

    for (size_t i = 0; i < rowComponents.size(); ++i)
    {
        addAndMakeVisible(rowComponents[i].get());
        rowComponents[i]->OnEdited = [this, i](size_t id) {
            processorRef.setRow(id, rowComponents[i]->stepComponent.steps);
        };
    }
    transposeSlider.setNumDecimalPlacesToDisplay(0);
    transposeSlider.setRange(0, numrow_elements - 1, 1);
    transposeSlider.onValueChange = [this]() { doTransform(); };
    // addAndMakeVisible(transposeSlider);

    invertButton.setButtonText("Invert");
    invertButton.onClick = [this]() { doTransform(); };
    // addAndMakeVisible(invertButton);

    reverseButton.setButtonText("Reverse");
    reverseButton.onClick = [this]() { doTransform(); };
    // addAndMakeVisible(reverseButton);

    // addAndMakeVisible(velocityMenuButton);
    velocityMenuButton.setButtonText("Velo...");
    velocityMenuButton.onClick = [this]() { showMenuForRow(1); };

    // addAndMakeVisible(octaveMenuButton);
    octaveMenuButton.setButtonText("Octave...");
    octaveMenuButton.onClick = [this]() { showMenuForRow(2); };

    setSize(900, 600);
    startTimer(100);
}

void AudioPluginAudioProcessorEditor::showMenuForRow(int which)
{
    /*
    juce::PopupMenu menu;
    int rowsize = 0;
    if (which == 1)
        rowsize = processorRef.rowOctave.num_active_entries;
    if (which == 2)
        rowsize = processorRef.rowVelocity.num_active_entries;
    for (int i = 0; i < rowsize; ++i)
    {
        menu.addItem("Prime " + juce::String(i),
                     [this, i, which]() { processorRef.transformRow(which, i, false, false); });
    }
    for (int i = 0; i < rowsize; ++i)
    {
        menu.addItem("Retrograde " + juce::String(i),
                     [this, i, which]() { processorRef.transformRow(which, i, false, true); });
    }
    for (int i = 0; i < rowsize; ++i)
    {
        menu.addItem("Inversion " + juce::String(i),
                     [this, i, which]() { processorRef.transformRow(which, i, true, false); });
    }
    for (int i = 0; i < rowsize; ++i)
    {
        menu.addItem("Retrograde Inversion " + juce::String(i),
                     [this, i, which]() { processorRef.transformRow(which, i, true, true); });
    }
    menu.showMenuAsync(juce::PopupMenu::Options{});
    */
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
    if (midiNoteNumber == 57)
    {
        // processorRef.transformVelocityRow(0, bool invert, bool reverse)
    }
}

void AudioPluginAudioProcessorEditor::timerCallback()
{
    MessageToUI msg;
    while (processorRef.fifo_to_ui.pop(msg))
    {
        if (msg.opcode == 0)
        {
            rowComponents[RID_PITCHCLASS]->stepComponent.setPlayingStep(msg.pitchclassplaypos);
            rowComponents[RID_OCTAVE]->stepComponent.setPlayingStep(msg.octaveplaypos);
            rowComponents[RID_VELOCITY]->stepComponent.setPlayingStep(msg.velocityplaypos);
        }
        if (msg.opcode == 1)
        {
            //octaveRowComponent.steps = processorRef.rowOctave;
            //octaveRowComponent.repaint();
        }
    }
        
}

void AudioPluginAudioProcessorEditor::doTransform()
{
    // if (!rowEntryComponent.steps.isValid())
    //     return;
    /*
    processorRef.setRow(0, rowEntryComponent.steps);
    processorRef.transformRow(0, (int)transposeSlider.getValue(), invertButton.getToggleState(),
                              reverseButton.getToggleState());
    rowEntryComponent.steps = processorRef.rowPitchClass;
    rowEntryComponent.repaint();
    */
}

//==============================================================================
void AudioPluginAudioProcessorEditor::paint(juce::Graphics &g)
{
    g.fillAll(juce::Colours::darkgrey);
}

void AudioPluginAudioProcessorEditor::resized()
{
    for (size_t i = 0; i < rowComponents.size(); ++i)
    {
        rowComponents[i]->setBounds(1, 175 * i, getWidth() - 2, 175);
    }
    keyboardComponent.setBounds(1, getBottom() - 50, getWidth() - 2, 50);
}
