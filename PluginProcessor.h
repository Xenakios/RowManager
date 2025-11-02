#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

class RowEngine
{
  public:
    RowEngine() {}
    static bool validate_row(std::vector<int> &row)
    {
        std::set<int> seen;
        for (auto &e : row)
        {
            if (seen.count(e))
                return false;
            seen.insert(e);
        }
        return true;
    }
    static std::vector<int> transpose_row(std::vector<int> &row, int transpose)
    {
        std::vector<int> result;
        for (auto &e : row)
        {
            result.push_back((e + transpose) % row.size());
        }
        return result;
    }
    static std::vector<int> invert_row(std::vector<int> &row)
    {
        //x = (len(row) - x) % len(row)
        std::vector<int> result;
        for (auto &e : row)
        {
            result.push_back((row.size()-e) % row.size());
        }
        return result;
    }
};

//==============================================================================
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

    RowEngine reng;

  private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioPluginAudioProcessor)
};
