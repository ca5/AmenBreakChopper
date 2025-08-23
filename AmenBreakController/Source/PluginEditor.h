/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include "PluginProcessor.h"

//==============================================================================
/**
*/
class AmenBreakControllerAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    AmenBreakControllerAudioProcessorEditor (AmenBreakControllerAudioProcessor&);
    ~AmenBreakControllerAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    AmenBreakControllerAudioProcessor& audioProcessor;

    juce::Slider mDelayTimeSlider;
    juce::Slider mSequencePositionSlider;
    juce::Slider mNoteSequencePositionSlider;

    juce::Slider mMidiInputChannelSlider;
    juce::Slider mMidiOutputChannelSlider;

    juce::ComboBox mControlModeComboBox;

    juce::Label mControlModeLabel, mDelayTimeLabel, mSequencePositionLabel, mNoteSequencePositionLabel, mMidiInputChannelLabel, mMidiOutputChannelLabel;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mDelayTimeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mSequencePositionAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mNoteSequencePositionAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mMidiInputChannelAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mMidiOutputChannelAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> mControlModeAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AmenBreakControllerAudioProcessorEditor)
};
