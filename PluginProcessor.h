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
    
    static std::vector<int> transform_row(const std::vector<int> &row, int transpose, bool inverted,
                                          bool reversed)
    {
        std::vector<int> result;
        for (auto &e : row)
        {
            int elem = (e + transpose) % row.size();
            if (inverted)
                elem = (row.size() - elem) % row.size();
            result.push_back(elem);
        }
        if (reversed)
            std::reverse(result.begin(), result.end());
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
    juce::MidiKeyboardState keyboardState;
  private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioPluginAudioProcessor)
};
