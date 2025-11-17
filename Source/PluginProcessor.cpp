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
    pending_rows.reserve(64);
    fifo_to_ui.reset(1024);
    fifo_to_processor.reset(1024);
    rows[RID_PITCHCLASS] = Row::make_all_interval(12);
    // rows[RID_PITCHCLASS].num_active_entries = 12;
    // for (int i = 0; i < 12; ++i)
    //     rows[RID_PITCHCLASS].entries[i] = (i * 7) % 12;
    rows[RID_DELTATIME] = Row::make_from_init_list({4, 3, 2, 0, 1});
    rows[RID_OCTAVE] = Row::make_from_init_list({3, 2, 1, 0});
    rows[RID_VELOCITY] = Row::make_from_init_list({2, 3, 0, 1});
    rows[RID_POLYAT] = Row::make_from_init_list({2, 3, 0, 1, 5, 4});
    rowRepeats = {1, 1, 1, 1};
    for (size_t i = 0; i < max_poly_voices; ++i)
    {
        for (size_t j = 0; j < RID_LAST; ++j)
        {
            voices[i].rowIterators[j] = Row::Iterator(rows[j], RowTransform());
            if (j == RID_OCTAVE)
            {
                voices[i].rowIterators[j].repetitions = 6;
            }
        }
    }

    playingNotes.reserve(1024);
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
void AudioPluginAudioProcessor::prepareToPlay(double /*sampleRate*/, int /*samplesPerBlock*/)
{
    send_ui_updates = true;
}

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
    jassert(num_active_voices <= max_poly_voices);
    juce::ScopedNoDenormals noDenormals;
    ph = getPlayHead();
    if (ph)
    {
        auto pos = ph->getPosition();
        if (pos)
        {
            curBPM = pos->getBpm().orFallback(120.0);
            curPPQPos = pos->getPpqPosition().orFallback(0.0);
        }
    }
    generatedMessages.clear();
    keyboardState.processNextMidiBuffer(midiMessages, 0, buffer.getNumSamples(), true);
    std::array<int, max_poly_voices> triggerstatuses;
    std::fill(triggerstatuses.begin(), triggerstatuses.end(), 0);

    for (const juce::MidiMessageMetadata metadata : midiMessages)
    {
        auto msg = metadata.getMessage();
        if (msg.isNoteOn())
        {
            if (msg.getNoteNumber() == 48)
            {
                // triggerstatus = 1;
            }
        }
        if (msg.isNoteOff())
        {
            if (msg.getNoteNumber() == 48)
            {
                // triggerstatus = 3;
            }
        }
    }
    MessageToProcessor amsg;
    while (fifo_to_processor.pop(amsg))
    {
        if (amsg.opcode == MessageToProcessor::OP_ChangeRow)
        {
            rows[amsg.row_index] = amsg.row;

            auto oldpos = voices[amsg.voice_index].rowIterators[amsg.row_index].pos;
            voices[amsg.voice_index].rowIterators[amsg.row_index] =
                Row::Iterator(rows[amsg.row_index], amsg.transform);
            voices[amsg.voice_index].rowIterators[amsg.row_index].pos = oldpos;
        }
        if (amsg.opcode == MessageToProcessor::OP_ChangeIntParameter)
        {
            if (amsg.par_index == 0)
            {
                if (amsg.par_ivalue == 0)
                {
                    selfSequence = false;
                    // triggerstatus = 3;
                }
                if (amsg.par_ivalue == 1)
                {
                    selfSequence = true;
                }
            }
            if (amsg.par_index == 1)
            {
                velocityLow = amsg.par_ivalue;
            }
        }
    }
    for (auto &pm : playingNotes)
    {
        pm.duration -= buffer.getNumSamples();
        // if (triggerstatus == 3 || pm.duration <= 0)
        if (pm.duration <= 0)
        {
            generatedMessages.addEvent(juce::MidiMessage::noteOff(pm.chan, pm.note, 0.0f), 0);
            pm.chan = -1;
        }
    }
    if (selfSequence)
    {
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            for (int j = 0; j < num_active_voices; ++j)
            {
                if (voices[j].playpos == 0)
                {
                    triggerstatuses[j] = 2;
                    // noteofftriggered = true;
                }
                ++voices[j].playpos;
                if (voices[j].playpos == voices[j].pulselen)
                    voices[j].playpos = 0;
            }
        }
    }
    if (send_ui_updates)
    {
        MessageToUI msg;
        msg.opcode = MessageToUI::OP_VoiceCountChanged;
        msg.par0 = num_active_voices;
        fifo_to_ui.push(msg);
        msg.opcode = MessageToUI::OP_RowTransformChanged;
        fifo_to_ui.push(msg);
        send_ui_updates = false;
    }
    double bpm = 120.0;
    for (size_t i = 0; i < num_active_voices; ++i)
    {
        if (triggerstatuses[i] == 1 || triggerstatuses[i] == 2)
        {
            MessageToUI msg;
            msg.opcode = MessageToUI::OP_StepPositionChanged;
            msg.voice_index = i;
            for (int rid = 0; rid < RID_LAST; ++rid)
            {
                msg.playpositions[rid] = voices[i].rowIterators[rid].pos;
            }

            int polyat = voices[i].rowIterators[RID_POLYAT].next();
            double plen = (1 + voices[i].rowIterators[RID_DELTATIME].next());

            plen = (60.0 / bpm / 4.0) * plen;
            voices[i].pulselen = static_cast<int>(getSampleRate() * plen);
            int octave = voices[i].rowIterators[RID_OCTAVE].next() - 3;
            int note = 60 + octave * rows[RID_PITCHCLASS].num_active_entries +
                       voices[i].rowIterators[RID_PITCHCLASS].next();
            msg.soundingpitch = note;
            fifo_to_ui.push(msg);
            float velo =
                juce::jmap<float>(voices[i].rowIterators[RID_VELOCITY].next(), 0,
                                  rows[RID_VELOCITY].num_active_entries - 1, velocityLow, 127);
            generatedMessages.addEvent(juce::MidiMessage::noteOn(1 + i, note, (juce::uint8)velo),
                                       0);
            int lentouse = notelen;
            if (triggerstatuses[i] == 1)
                lentouse = 100000000;
            playingNotes.push_back({int(1 + i), note, lentouse});
        }
    }
    std::erase_if(playingNotes, [](const auto &t) { return t.chan == -1; });
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
    send_ui_updates = true;
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
