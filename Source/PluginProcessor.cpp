#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "juce_audio_basics/juce_audio_basics.h"
#include "juce_core/juce_core.h"
#include "row_engine.h"

//==============================================================================
AudioPluginAudioProcessor::AudioPluginAudioProcessor()
    : AudioProcessor(BusesProperties()
#if !JucePlugin_IsMidiEffect
#if !JucePlugin_IsSynth
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
#endif
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)
#endif
      )
{
    fifo_to_ui.reset(1024);

    rows[RID_PITCHCLASS].num_active_entries = 12;
    for (int i = 0; i < 12; ++i)
        rows[RID_PITCHCLASS].entries[i] = (i * 7) % 12;
    rows[RID_OCTAVE] = Row::make_from_init_list({3, 2, 1, 0});
    rows[RID_VELOCITY] = Row::make_from_init_list({2, 3, 0, 1});
    rows[RID_POLYAT] = Row::make_from_init_list({2, 3, 0, 1, 5, 4});
    for (size_t i = 0; i < 4; ++i)
    {
        rowIterators[i] = Row::Iterator(rows[i], true);
    }
    playingNotes.reserve(1024);
}

void AudioPluginAudioProcessor::transformRow(size_t whichRow, int transpose, bool invert,
                                             bool reverse)
{
    juce::ScopedLock locker(cs);
    rows[whichRow].setTransform(transpose, invert, reverse);
    row_was_changed = true;
}

void AudioPluginAudioProcessor::setRow(size_t which, Row r)
{
    juce::ScopedLock locker(cs);
    auto oldpos = rowIterators[which].pos;
    rows[which] = r;
    rowIterators[which] = Row::Iterator(rows[which], true);
    rowIterators[which].pos = oldpos;
}

AudioPluginAudioProcessor::~AudioPluginAudioProcessor() {}

//==============================================================================
const juce::String AudioPluginAudioProcessor::getName() const { return JucePlugin_Name; }

bool AudioPluginAudioProcessor::acceptsMidi() const { return true; }

bool AudioPluginAudioProcessor::producesMidi() const
{
#if JucePlugin_ProducesMidiOutput
    return true;
#else
    return false;
#endif
}

bool AudioPluginAudioProcessor::isMidiEffect() const
{
#if JucePlugin_IsMidiEffect
    return true;
#else
    return false;
#endif
}

double AudioPluginAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int AudioPluginAudioProcessor::getNumPrograms()
{
    return 1; // NB: some hosts don't cope very well if you tell them there are 0 programs,
              // so this should be at least 1, even if you're not really implementing programs.
}

int AudioPluginAudioProcessor::getCurrentProgram() { return 0; }

void AudioPluginAudioProcessor::setCurrentProgram(int index) { juce::ignoreUnused(index); }

const juce::String AudioPluginAudioProcessor::getProgramName(int index)
{
    juce::ignoreUnused(index);
    return {};
}

void AudioPluginAudioProcessor::changeProgramName(int index, const juce::String &newName)
{
    juce::ignoreUnused(index, newName);
}

//==============================================================================
void AudioPluginAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {}

void AudioPluginAudioProcessor::releaseResources() {}

bool AudioPluginAudioProcessor::isBusesLayoutSupported(const BusesLayout &layouts) const
{
#if JucePlugin_IsMidiEffect
    juce::ignoreUnused(layouts);
    return true;
#else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono() &&
        layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

        // This checks if the input layout matches the output layout
#if !JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
#endif

    return true;
#endif
}

void AudioPluginAudioProcessor::processBlock(juce::AudioBuffer<float> &buffer,
                                             juce::MidiBuffer &midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ScopedLock locker(cs);
    if (row_was_changed)
    {
        MessageToUI msg;
        msg.opcode = 1;
        fifo_to_ui.push(msg);
        row_was_changed = false;
    }
    generatedMessages.clear();
    keyboardState.processNextMidiBuffer(midiMessages, 0, buffer.getNumSamples(), true);
    bool noteontriggered = false;
    bool noteofftriggered = false;
    for (const juce::MidiMessageMetadata metadata : midiMessages)
    {
        auto msg = metadata.getMessage();
        if (msg.isNoteOn())
        {
            if (msg.getNoteNumber() == 48)
            {
                noteontriggered = true;
            }
        }
        if (msg.isNoteOff())
        {
            if (msg.getNoteNumber() == 48)
            {
                noteofftriggered = true;
            }
        }
    }
    if (selfSequence)
    {
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            if (playpos == 0)
            {
                noteontriggered = true;
                noteofftriggered = true;
            }
            ++playpos;
            if (playpos == pulselen)
                playpos = 0;
        }
    }

    if (noteofftriggered)
    {
        for (auto &e : playingNotes)
        {
            generatedMessages.addEvent(juce::MidiMessage::noteOff(1, std::get<1>(e), 1.0f), 0);
        }
        playingNotes.clear();
    }
    if (noteontriggered)
    {
        MessageToUI msg;
        msg.pitchclassplaypos = rowIterators[RID_PITCHCLASS].pos;
        msg.octaveplaypos = rowIterators[RID_OCTAVE].pos;
        msg.velocityplaypos = rowIterators[RID_VELOCITY].pos;
        msg.polyatplaypos = rowIterators[RID_POLYAT].pos;
        int polyat = rowIterators[RID_POLYAT].next();
        int octave = rowIterators[RID_OCTAVE].next();
        int note = 24 + octave * rows[RID_PITCHCLASS].num_active_entries +
                   rowIterators[RID_PITCHCLASS].next();
        msg.soundingpitch = note;
        fifo_to_ui.push(msg);
        float velo = juce::jmap<float>(rowIterators[RID_VELOCITY].next(), 0,
                                       rows[RID_VELOCITY].num_active_entries - 1, 0.25, 1.0);
        generatedMessages.addEvent(juce::MidiMessage::noteOn(1, note, velo), 0);
        playingNotes.push_back({1, note});
    }
    midiMessages.swapWith(generatedMessages);
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());
}

//==============================================================================
bool AudioPluginAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor *AudioPluginAudioProcessor::createEditor()
{
    return new AudioPluginAudioProcessorEditor(*this);
}

//==============================================================================
void AudioPluginAudioProcessor::getStateInformation(juce::MemoryBlock &destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
    juce::ignoreUnused(destData);
}

void AudioPluginAudioProcessor::setStateInformation(const void *data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
    juce::ignoreUnused(data, sizeInBytes);
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter() { return new AudioPluginAudioProcessor(); }
