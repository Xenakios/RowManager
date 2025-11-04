#pragma once

#include "PluginProcessor.h"
#include "juce_audio_basics/juce_audio_basics.h"
#include "juce_audio_utils/juce_audio_utils.h"
#include "juce_core/juce_core.h"
#include "juce_events/juce_events.h"
#include "juce_graphics/juce_graphics.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include "row_engine.h"
#include <cstdint>

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
            row_iterator.set_position(i);
            steph = juce::jmap<double>(row_iterator.next(), 0, steps.num_active_entries,
                                       getHeight() - 2.0, 0);
            g.fillRect((float)1.0 + i * stepw + stepw / 2, steph, stepw / 2, getHeight() - steph);
            g.setColour(juce::Colours::white);

            g.drawText(juce::String(steps.entries[i]),
                       juce::Rectangle<int>(1.0 + i * stepw, 0.0, stepw, 20.0),
                       juce::Justification::centred);
        }
    }

    Row steps;
    Row::Iterator row_iterator;

  private:
    int draggingIndex = -1;
    int dragystart = 0;
    int stepstart = 0;
};

class RowComponent : public juce::Component
{
  public:
    RowComponent(juce::String name, size_t rowId, Row initialRow, toproc_fifo_t &fifo)
        : toproc_fifo(fifo)
    {
        rowid = rowId;
        infoLabel.setText(name, juce::dontSendNotification);
        infoLabel.setColour(juce::Label::textColourId, juce::Colours::black);
        addAndMakeVisible(infoLabel);
        stepComponent.readonly = false;
        stepComponent.OnEdited = [this]() {
            if (OnEdited)
                OnEdited(rowid);
        };
        stepComponent.steps = initialRow;
        stepComponent.row_iterator = Row::Iterator(stepComponent.steps, RowTransform{});
        addAndMakeVisible(stepComponent);
        addAndMakeVisible(menuButton);
        menuButton.setButtonText("Transform...");
        menuButton.onClick = [this]() {
            juce::PopupMenu menu;
            struct Info
            {
                juce::String text;
                bool inv = false;
                bool rev = false;
            };
            std::vector<Info> infos;
            infos.emplace_back("Prime", false, false);
            infos.emplace_back("Retrograde", false, true);
            infos.emplace_back("Inverse", true, false);
            infos.emplace_back("Retrograde Inverse", true, true);
            for (auto &transform : infos)
            {
                const auto &curtransform = stepComponent.row_iterator.transform;
                for (uint16_t i = 0; i < stepComponent.steps.num_active_entries; ++i)
                {

                    bool ticked = i == curtransform.transpose &&
                                  transform.inv == curtransform.inverted &&
                                  curtransform.reversed == transform.rev;
                    menu.addItem(transform.text + " " + juce::String(i), true, ticked,
                                 [this, transform, i]() {
                                     stepComponent.row_iterator = Row::Iterator(
                                         stepComponent.steps, {i, transform.inv, transform.rev});
                                     if (OnEdited)
                                         OnEdited(rowid);
                                 });
                }
            }

            menu.showMenuAsync(juce::PopupMenu::Options{});
        };
    }
    void resized() override
    {
        infoLabel.setBounds(0, 0, getWidth(), 25);
        stepComponent.setBounds(0, 25, getWidth(), getHeight() - 50);
        menuButton.setBounds(1, stepComponent.getBottom() + 1, 100, 24);
    }
    void paint(juce::Graphics &g) override { g.fillAll(juce::Colours::orange); }
    size_t rowid = 0;
    std::function<void(size_t)> OnEdited;
    toproc_fifo_t &toproc_fifo;
    juce::Label infoLabel;
    juce::TextButton menuButton;
    MultiStepComponent stepComponent;
};

class VelocityRowComponent : public RowComponent
{
  public:
    VelocityRowComponent(juce::String name, size_t rowId, Row initialRow, toproc_fifo_t &fifo)
        : RowComponent(name, rowId, initialRow, fifo)
    {
        addAndMakeVisible(velLowSlider);
        velLowSlider.setSliderStyle(juce::Slider::SliderStyle::IncDecButtons);
        velLowSlider.setNumDecimalPlacesToDisplay(0);
        velLowSlider.setTextBoxStyle(juce::Slider::TextEntryBoxPosition::TextBoxLeft, false, 30,
                                     24);
        velLowSlider.setRange(0, 127, 1);
        // velLowSlider.setIncDecButtonsMode
        velLowSlider.setValue(64, juce::dontSendNotification);
        velLowSlider.onValueChange = [this]() {
            MessageToProcessor msg;
            msg.opcode = MessageToProcessor::OP_ChangeIntParameter;
            msg.par_index = 1;
            msg.par_ivalue = velLowSlider.getValue();
            toproc_fifo.push(msg);
        };
    }
    juce::Slider velLowSlider;
    juce::Slider velCurveSlider;
    void resized() override
    {
        RowComponent::resized();
        velLowSlider.setBounds(menuButton.getRight() + 2, menuButton.getY(), 120, 24);
    }
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
    std::vector<std::unique_ptr<RowComponent>> rowComponents;
    void doTransform();
    juce::ToggleButton selfSequenceToggle;
    juce::Label debugLabel;
    bool rowValid = false;
    juce::MidiKeyboardComponent keyboardComponent;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioPluginAudioProcessorEditor)
};
