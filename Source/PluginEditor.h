#pragma once

#include "PluginProcessor.h"
#include "juce_audio_basics/juce_audio_basics.h"
#include "juce_audio_utils/juce_audio_utils.h"
#include "juce_core/juce_core.h"
#include "juce_events/juce_events.h"
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

    void mouseDown(const juce::MouseEvent &ev) override
    {
        if (readonly)
            return;
        int index = (float)ev.x / ((float)getWidth() / steps.num_active_entries);
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
        float stepw = (float)getWidth() / steps.num_active_entries;
        for (size_t i = 0; i < steps.num_active_entries; ++i)
        {
            if (i == draggingIndex)
                g.setColour(juce::Colours::yellow);
            else
                g.setColour(juce::Colours::green);
            float steph = juce::jmap<double>(steps.entries[i], 0, steps.num_active_entries,
                                             getHeight() - 2.0, 0);
            g.fillRect((float)1.0 + i * stepw, steph, stepw / 2.0, getHeight() - steph);
            if (i == playingstep)
                g.setColour(juce::Colours::grey);
            else
                g.setColour(juce::Colours::darkgrey);
            steph = juce::jmap<double>(steps.transformed_entries[i], 0, steps.num_active_entries,
                                       getHeight() - 2.0, 0);
            g.fillRect((float)1.0 + i * stepw + stepw / 2, steph, stepw / 2, getHeight() - steph);
            g.setColour(juce::Colours::white);

            g.drawText(juce::String(steps.entries[i]),
                       juce::Rectangle<int>(1.0 + i * stepw, 0.0, stepw, 20.0),
                       juce::Justification::centred);
        }
    }

    Row steps;

  private:
    int draggingIndex = -1;
    int dragystart = 0;
    int stepstart = 0;
};

class RowComponent : public juce::Component
{
  public:
    RowComponent(juce::String name, size_t rowId, Row initialRow)
    {
        infoLabel.setText(name, juce::dontSendNotification);
        addAndMakeVisible(infoLabel);
        stepComponent.steps = initialRow;
        addAndMakeVisible(stepComponent);
    }
    void resized() override
    {
        infoLabel.setBounds(0, 0, getWidth(), 25);
        stepComponent.setBounds(0, 25, getWidth(), getHeight() - 25);
    }

    juce::Label infoLabel;
    MultiStepComponent stepComponent;
};

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
    std::vector<std::unique_ptr<RowComponent>> rowComponents;
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
