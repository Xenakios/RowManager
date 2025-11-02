#pragma once

#include "PluginProcessor.h"
#include "juce_audio_basics/juce_audio_basics.h"
#include "juce_audio_utils/juce_audio_utils.h"
#include "juce_core/juce_core.h"
#include "juce_graphics/juce_graphics.h"
#include "juce_gui_basics/juce_gui_basics.h"

class MultiStepComponent : public juce::Component
{
  public:
    MultiStepComponent() { steps.resize(16); }
    bool readonly = true;
    std::function<void()> OnEdited = nullptr;
    void setNumActiveSteps(int numsteps) { numActiveSteps = numsteps; }
    void mouseDown(const juce::MouseEvent &ev) override
    {
        if (readonly)
            return;
        int index = (float)ev.x / 25.0;
        DBG(index);
        if (index >= 0 && index < numActiveSteps)
        {
            draggingIndex = index;
            dragystart = ev.y;
            stepstart = steps[index];
        }
        repaint();
    }
    void mouseUp(const juce::MouseEvent &ev) override
    {
        draggingIndex = -1;
        repaint();
    }
    void mouseDrag(const juce::MouseEvent &ev) override
    {
        if (readonly)
            return;
        if (draggingIndex >= 0)
        {
            float deltay = dragystart - ev.y;
            steps[draggingIndex] += deltay * 0.1;
            steps[draggingIndex] =
                juce::jmap<double>(ev.y, 0.0, getHeight(), numActiveSteps - 1, 0);
            steps[draggingIndex] = juce::jlimit<int>(0, numActiveSteps - 1, steps[draggingIndex]);
            DBG(deltay);
            repaint();
            if (OnEdited)
                OnEdited();
        }
    }
    void paint(juce::Graphics &g) override
    {
        if (RowEngine::validate_row(steps))
            g.fillAll(juce::Colours::black);
        else
            g.fillAll(juce::Colours::red.brighter());
        for (int i = 0; i < numActiveSteps; ++i)
        {
            if (i == draggingIndex)
                g.setColour(juce::Colours::yellow);
            else
                g.setColour(juce::Colours::green);
            float steph = juce::jmap<double>(steps[i], 0, numActiveSteps, getHeight() - 2.0, 0);
            g.fillRect((float)1.0 + i * 25, steph, 24.0, getHeight() - steph);
            g.setColour(juce::Colours::white);
            g.drawText(juce::String(steps[i]), juce::Rectangle<int>(1.0 + i * 25, 0.0, 24, 20.0),
                       juce::Justification::centred);
        }
    }
    std::vector<int> steps;
    int numActiveSteps = 0;
    int draggingIndex = -1;
    int dragystart = 0;
    int stepstart = 0;
};

//==============================================================================
class AudioPluginAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                              public juce::MidiKeyboardStateListener
{
  public:
    explicit AudioPluginAudioProcessorEditor(AudioPluginAudioProcessor &);
    ~AudioPluginAudioProcessorEditor() override;
    void handleNoteOn(juce::MidiKeyboardState *source, int midiChannel, int midiNoteNumber,
                      float velocity) override;

    void handleNoteOff(juce::MidiKeyboardState *source, int midiChannel, int midiNoteNumber,
                       float velocity) override
    {
    }
    //==============================================================================
    void paint(juce::Graphics &) override;
    void resized() override;
    void updateAndValidateRow();

  private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    AudioPluginAudioProcessor &processorRef;
    int numrow_elements = 16;
    juce::Slider transposeSlider;
    MultiStepComponent rowEntryComponent;
    MultiStepComponent transformedRowComponent;
    std::vector<int> candidateRow;
    juce::ToggleButton invertButton;
    juce::ToggleButton reverseButton;
    void updateRowSliders();
    void doTransform();
    bool rowValid = false;
    juce::MidiKeyboardComponent keyboardComponent;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioPluginAudioProcessorEditor)
};
