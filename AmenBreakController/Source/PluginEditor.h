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
class AmenBreakControllerAudioProcessorEditor
    : public juce::AudioProcessorEditor,
      public juce::TextEditor::Listener,
      public juce::Button::Listener {
public:
  AmenBreakControllerAudioProcessorEditor(AmenBreakControllerAudioProcessor &);
  ~AmenBreakControllerAudioProcessorEditor() override;

  //==============================================================================
  void paint(juce::Graphics &) override;
  void resized() override;
  void textEditorTextChanged(juce::TextEditor &editor) override;
  void buttonClicked(juce::Button *button) override;

private:
  AmenBreakControllerAudioProcessor &audioProcessor;

  // Live Status
  juce::Label mStatusLabel;
  juce::Slider mSequencePositionSlider;
  juce::Slider mNoteSequencePositionSlider;
  juce::Label mSequencePositionLabel;
  juce::Label mNoteSequencePositionLabel;

  // MIDI Config
  juce::Label mMidiConfigLabel;
  juce::Slider mMidiInputChannelSlider;
  juce::Slider mMidiOutputChannelSlider;
  juce::Label mMidiInputChannelLabel;
  juce::Label mMidiOutputChannelLabel;

  juce::Label mMidiCcSeqResetLabel;
  juce::Slider mMidiCcSeqResetSlider;
  juce::ComboBox mMidiCcSeqResetModeComboBox;
  juce::Label mMidiCcHardResetLabel;
  juce::Slider mMidiCcHardResetSlider;
  juce::ComboBox mMidiCcHardResetModeComboBox;
  juce::Label mMidiCcSoftResetLabel;
  juce::Slider mMidiCcSoftResetSlider;
  juce::ComboBox mMidiCcSoftResetModeComboBox;

  // OSC Config
  juce::Label mOscConfigLabel;
  juce::TextEditor mOscHostAddressEditor;
  juce::Slider mOscSendPortSlider;
  juce::Slider mOscReceivePortSlider;
  juce::Label mOscHostAddressLabel;
  juce::Label mOscSendPortLabel;
  juce::Label mOscReceivePortLabel;

  // OSC Trigger Buttons
  juce::Label mOscTriggerLabel;
  juce::TextButton mSequenceResetButton;
  juce::TextButton mHardResetButton;
  juce::TextButton mSoftResetButton;

  // Attachments
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      mSequencePositionAttachment;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      mNoteSequencePositionAttachment;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      mMidiInputChannelAttachment;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      mMidiOutputChannelAttachment;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      mOscSendPortAttachment;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      mOscReceivePortAttachment;

  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      mMidiCcSeqResetAttachment;
  std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>
      mMidiCcSeqResetModeAttachment;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      mMidiCcHardResetAttachment;
  std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>
      mMidiCcHardResetModeAttachment;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      mMidiCcSoftResetAttachment;
  std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>
      mMidiCcSoftResetModeAttachment;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(
      AmenBreakControllerAudioProcessorEditor)
};