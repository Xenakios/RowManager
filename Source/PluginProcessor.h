#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <span>
#include "juce_audio_basics/juce_audio_basics.h"
#include "juce_core/juce_core.h"
#include "row_engine.h"
#include "containers/choc_SingleReaderSingleWriterFIFO.h"

struct MessageToUI
{
    int opcode = 0;
    int pitchclassplaypos = 0;
    int soundingpitch = 0;
    int octaveplaypos = 0;
    int velocityplaypos = 0;
    int polyatplaypos = 0;
};

using namespace xenakios;

class AudioPluginAudioProcessor final : public juce::AudioProcessor
{
  public:
    //==============================================================================
    AudioPluginAudioProcessor();
    ~AudioPluginAudioProcessor() override;

    //==============================================================================
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout &layouts) const override;

    void processBlock(juce::AudioBuffer<float> &, juce::MidiBuffer &) override;
    using AudioProcessor::processBlock;

    //==============================================================================
    juce::AudioProcessorEditor *createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String &newName) override;

    //==============================================================================
    void getStateInformation(juce::MemoryBlock &destData) override;
    void setStateInformation(const void *data, int sizeInBytes) override;
    std::array<Row, 4> rows;
    std::array<Row::Iterator, 4> rowIterators;
    std::array<size_t, 4> rowRepeats;
    void transformRow(size_t whichRow, int transpose, bool invert, bool reverse);

    void setRow(size_t which, Row r);
    juce::MidiKeyboardState keyboardState;
    juce::CriticalSection cs;
    std::vector<std::tuple<int, int>> playingNotes;
    juce::MidiBuffer generatedMessages;
    int playpos = 0;
    int pulselen = 11025;
    choc::fifo::SingleReaderSingleWriterFIFO<MessageToUI> fifo_to_ui;
    std::atomic<bool> row_was_changed{false};
    std::atomic<bool> selfSequence{true};

  private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioPluginAudioProcessor)
};
