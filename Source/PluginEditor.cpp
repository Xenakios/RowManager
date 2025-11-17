#include "PluginProcessor.h"
#include "juce_audio_utils/juce_audio_utils.h"
#include "juce_events/juce_events.h"
#include "juce_graphics/juce_graphics.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include "row_engine.h"
#include <cstdint>
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
        MessageToProcessor msg;
        msg.opcode = MessageToProcessor::OP_ChangeIntParameter;
        msg.par_index = 0;
        msg.par_ivalue = selfSequenceToggle.getToggleState();
        processorRef.fifo_to_processor.push(msg);
    };

    addAndMakeVisible(debugLabel);

    rowComponents.push_back(std::make_unique<RowComponent>("Pitch Class", RID_PITCHCLASS,
                                                           processorRef.rows[RID_PITCHCLASS],
                                                           processorRef.fifo_to_processor));
    rowComponents.push_back(std::make_unique<RowComponent>("Onset difference", RID_DELTATIME,
                                                           processorRef.rows[RID_DELTATIME],
                                                           processorRef.fifo_to_processor));
    rowComponents.push_back(std::make_unique<RowComponent>(
        "Octave", RID_OCTAVE, processorRef.rows[RID_OCTAVE], processorRef.fifo_to_processor));

    rowComponents.push_back(std::make_unique<VelocityRowComponent>(
        "Velocity", RID_VELOCITY, processorRef.rows[RID_VELOCITY], processorRef.fifo_to_processor));
    rowComponents.push_back(std::make_unique<RowComponent>(
        "PolyAT", RID_POLYAT, processorRef.rows[RID_POLYAT], processorRef.fifo_to_processor));
    for (size_t i = 0; i < rowComponents.size(); ++i)
    {
        addAndMakeVisible(rowComponents[i].get());
        rowComponents[i]->OnEdited = [this, i](size_t id) {
            for (int j = 0; j < max_poly_voices; ++j)
            {
                MessageToProcessor msg;
                msg.opcode = MessageToProcessor::OP_ChangeRow;
                msg.row_index = id;
                msg.voice_index = j;
                msg.row = rowComponents[i]->stepComponent.steps;
                msg.transform = rowComponents[i]->stepComponent.row_iterators[j].transform;
                processorRef.fifo_to_processor.push(msg);
            }
        };
    }
    setSize(1000, 800);
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
        if (msg.opcode == MessageToUI::OP_StepPositionChanged)
        {
            for (auto &c : rowComponents)
            {
                c->stepComponent.setPlayingStep(msg.voice_index, msg.playpositions[c->rowid]);
            }
        }
        if (msg.opcode == MessageToUI::OP_VoiceCountChanged)
        {
            for (auto &c : rowComponents)
            {
                c->stepComponent.num_active_voices = msg.par0;
            }
        }
        if (msg.opcode == MessageToUI::OP_RowTransformChanged)
        {
            for (auto &c : rowComponents)
            {
                // c->
            }
        }
    }
    juce::String txt;
    txt << processorRef.playingNotes.size() << " playing notes ";
    txt << processorRef.pending_rows.size() << " pending row changes, BPM ";
    txt << processorRef.curBPM;
    txt << " cur PPQ Pos " << processorRef.curPPQPos;
    debugLabel.setText(txt, juce::dontSendNotification);
}



//==============================================================================
void AudioPluginAudioProcessorEditor::paint(juce::Graphics &g)
{
    g.fillAll(juce::Colours::darkgrey);
}

void AudioPluginAudioProcessorEditor::resized()
{
    int yoffs = 1;
    selfSequenceToggle.setBounds(1, yoffs, 120, 24);
    debugLabel.setBounds(selfSequenceToggle.getRight() + 1, 1, getWidth() - 125, 24);
    yoffs += 25;
    rowComponents[0]->setBounds(1, yoffs, getWidth() - 2, 175);
    yoffs += 178;
    rowComponents[1]->setBounds(1, yoffs, getWidth() - 2, 175);
    yoffs += 178;
    rowComponents[2]->setBounds(1, yoffs, getWidth() / 2 - 2, 175);
    rowComponents[3]->setBounds(1 + getWidth() / 2, yoffs, getWidth() / 2 - 2, 175);
    yoffs += 178;
    rowComponents[4]->setBounds(1, yoffs, getWidth(), 175);
    keyboardComponent.setBounds(1, getBottom() - 50, getWidth() - 2, 50);
}
