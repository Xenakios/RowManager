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

    addAndMakeVisible(selfSequenceToggle);
    selfSequenceToggle.setButtonText("Self sequence");
    selfSequenceToggle.setToggleState(processorRef.selfSequence, juce::dontSendNotification);
    selfSequenceToggle.onClick = [this]() {
        processorRef.selfSequence = selfSequenceToggle.getToggleState();
    };

    rowComponents.push_back(std::make_unique<RowComponent>("Pitch Class", RID_PITCHCLASS,
                                                           processorRef.rows[RID_PITCHCLASS]));
    rowComponents.push_back(
        std::make_unique<RowComponent>("Octave", RID_OCTAVE, processorRef.rows[RID_OCTAVE]));

    rowComponents.push_back(
        std::make_unique<RowComponent>("Velocity", RID_VELOCITY, processorRef.rows[RID_VELOCITY]));
    rowComponents.push_back(
        std::make_unique<RowComponent>("PolyAT", RID_POLYAT, processorRef.rows[RID_POLYAT]));
    for (size_t i = 0; i < rowComponents.size(); ++i)
    {
        addAndMakeVisible(rowComponents[i].get());
        rowComponents[i]->OnEdited = [this, i](size_t id) {
            processorRef.setRow(id, rowComponents[i]->stepComponent.steps);
        };
    }
    setSize(900, 620);
    startTimer(100);
}

AudioPluginAudioProcessorEditor::~AudioPluginAudioProcessorEditor()
{
    processorRef.keyboardState.removeListener(this);
}

void AudioPluginAudioProcessorEditor::handleNoteOn(juce::MidiKeyboardState *source, int midiChannel,
                                                   int midiNoteNumber, float velocity)
{
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
            rowComponents[RID_POLYAT]->stepComponent.setPlayingStep(msg.polyatplaypos);
        }
        if (msg.opcode == 1)
        {
        }
    }
}

void AudioPluginAudioProcessorEditor::doTransform() {}

//==============================================================================
void AudioPluginAudioProcessorEditor::paint(juce::Graphics &g)
{
    g.fillAll(juce::Colours::darkgrey);
}

void AudioPluginAudioProcessorEditor::resized()
{
    int yoffs = 1;
    selfSequenceToggle.setBounds(1, yoffs, 120, 24);
    yoffs += 25;
    rowComponents[0]->setBounds(1, yoffs, getWidth() - 2, 175);
    yoffs += 178;
    rowComponents[1]->setBounds(1, yoffs, getWidth() / 2 - 2, 175);
    rowComponents[2]->setBounds(1 + getWidth() / 2, yoffs, getWidth() / 2 - 2, 175);
    yoffs += 178;
    rowComponents[3]->setBounds(1, yoffs, getWidth(), 175);
    keyboardComponent.setBounds(1, getBottom() - 50, getWidth() - 2, 50);
}
