#pragma once

#include "PluginProcessor.h"
#include "juce_audio_basics/juce_audio_basics.h"
#include "juce_audio_utils/juce_audio_utils.h"
#include "juce_core/juce_core.h"
#include "juce_graphics/juce_graphics.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include "row_engine.h"

class MultiStepComponent : public juce::Component
{
  public:
    MultiStepComponent() {}
    bool readonly = true;
    int playingstep = -1;
    void setPlayingStep(int i)
    {
        playingstep = i;
        repaint();
    }
    std::function<void()> OnEdited = nullptr;
    void setNumActiveSteps(size_t numsteps) { steps.num_active_entries; }
    void mouseDown(const juce::MouseEvent &ev) override
    {
        if (readonly)
            return;
        int index = (float)ev.x / 25.0;
        DBG(index);
        if (index >= 0 && index < steps.num_active_entries)
        {
            draggingIndex = index;
            dragystart = ev.y;
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
            // steps[draggingIndex] += deltay * 0.1;
            steps.entries[draggingIndex] =
                juce::jmap<double>(ev.y, 0.0, getHeight(), steps.num_active_entries - 1, 0);
            steps.entries[draggingIndex] =
                juce::jlimit<int>(0, steps.num_active_entries - 1, steps.entries[draggingIndex]);
            // steps.setTransform(steps.tprops.transpose, steps.tprops.inverted,
            //                    steps.tprops.reversed);
            //  DBG(deltay);
            repaint();
            if (OnEdited)
                OnEdited();
        }
    }
    void paint(juce::Graphics &g) override
    {
        if (steps.isValid())
            g.fillAll(juce::Colours::black);
        else
            g.fillAll(juce::Colours::red.darker());
        for (size_t i = 0; i < steps.num_active_entries; ++i)
        {
            if (i == draggingIndex)
                g.setColour(juce::Colours::yellow);
            else
                g.setColour(juce::Colours::green);
            float steph = juce::jmap<double>(steps.entries[i], 0, steps.num_active_entries,
                                             getHeight() - 2.0, 0);
            g.fillRect((float)1.0 + i * 25, steph, 12.0, getHeight() - steph);
            if (i == playingstep)
                g.setColour(juce::Colours::cyan);
            else
                g.setColour(juce::Colours::cyan.darker());
            steph = juce::jmap<double>(steps.transformed_entries[i], 0, steps.num_active_entries,
                                       getHeight() - 2.0, 0);
            g.fillRect((float)1.0 + i * 25 + 13, steph, 12.0, getHeight() - steph);
            g.setColour(juce::Colours::white);

            g.drawText(juce::String(steps.entries[i]),
                       juce::Rectangle<int>(1.0 + i * 25, 0.0, 24, 20.0),
                       juce::Justification::centred);
        }
    }

    Row steps;

  private:
    int draggingIndex = -1;
    int dragystart = 0;
    int stepstart = 0;
};

//==============================================================================
class AudioPluginAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                              public juce::MidiKeyboardStateListener,
                                              public juce::Timer
{
  public:
    explicit AudioPluginAudioProcessorEditor(AudioPluginAudioProcessor &);
    ~AudioPluginAudioProcessorEditor() override;
    void timerCallback() override;
    void handleNoteOn(juce::MidiKeyboardState *source, int midiChannel, int midiNoteNumber,
                      float velocity) override;

    void handleNoteOff(juce::MidiKeyboardState *source, int midiChannel, int midiNoteNumber,
                       float velocity) override
    {
    }
    //==============================================================================
    void paint(juce::Graphics &) override;
    void resized() override;

  private:
    AudioPluginAudioProcessor &processorRef;
    size_t numrow_elements = 16;
    juce::Slider transposeSlider;
    MultiStepComponent rowEntryComponent;
    MultiStepComponent octaveRowComponent;
    juce::ToggleButton invertButton;
    juce::ToggleButton reverseButton;
    juce::TextButton velocityMenuButton;
    juce::TextButton octaveMenuButton;
    void doTransform();
    void showMenuForRow(int which);
    bool rowValid = false;
    juce::MidiKeyboardComponent keyboardComponent;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioPluginAudioProcessorEditor)
};
