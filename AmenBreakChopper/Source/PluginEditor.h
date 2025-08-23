/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
/**
*/
class AmenBreakChopperAudioProcessorEditor  : public juce::AudioProcessorEditor,
                                              public juce::TextEditor::Listener,
                                              public juce::AudioProcessorValueTreeState::Listener
{
public:
    AmenBreakChopperAudioProcessorEditor (AmenBreakChopperAudioProcessor&);
    ~AmenBreakChopperAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;
    void textEditorTextChanged(juce::TextEditor& editor) override;
    void parameterChanged(const juce::String& parameterID, float newValue) override;

private:
    void updateOscControlsEnablement();

    AmenBreakChopperAudioProcessor& audioProcessor;

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

    // New controls
    juce::Label mOscConfigLabel;
    juce::Label mOscHostAddressLabel;
    juce::TextEditor mOscHostAddressEditor;
    juce::Label mOscSendPortLabel;
    juce::Slider mOscSendPortSlider;
    juce::Label mOscReceivePortLabel;
    juce::Slider mOscReceivePortSlider;

    juce::Label mMidiCcConfigLabel;
    juce::Label mMidiCcSeqResetLabel;
    juce::Slider mMidiCcSeqResetSlider;
    juce::Label mMidiCcTimerResetLabel;
    juce::Slider mMidiCcTimerResetSlider;
    juce::Label mMidiCcSoftResetLabel;
    juce::Slider mMidiCcSoftResetSlider;

    // New attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mOscSendPortAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mOscReceivePortAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mMidiCcSeqResetAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mMidiCcTimerResetAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mMidiCcSoftResetAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AmenBreakChopperAudioProcessorEditor)
};