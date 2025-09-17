/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
AmenBreakChopperAudioProcessorEditor::AmenBreakChopperAudioProcessorEditor (AmenBreakChopperAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // === Live Status ===
    mStatusLabel.setText("Live Status", juce::dontSendNotification);
    mStatusLabel.setFont(juce::Font(16.0f, juce::Font::bold));
    addAndMakeVisible(mStatusLabel);

    mDelayTimeLabel.setText("Delay Time", juce::dontSendNotification);
    addAndMakeVisible(mDelayTimeLabel);
    mSequencePositionLabel.setText("Sequence Position", juce::dontSendNotification);
    addAndMakeVisible(mSequencePositionLabel);
    mNoteSequencePositionLabel.setText("Note Sequence Position", juce::dontSendNotification);
    addAndMakeVisible(mNoteSequencePositionLabel);

    mDelayTimeSlider.setSliderStyle(juce::Slider::SliderStyle::LinearHorizontal);
    mDelayTimeSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, true, 100, 25);
    addAndMakeVisible(mDelayTimeSlider);

    mSequencePositionSlider.setSliderStyle(juce::Slider::SliderStyle::LinearHorizontal);
    mSequencePositionSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, true, 100, 25);
    mSequencePositionSlider.setInterceptsMouseClicks(false, false);
    addAndMakeVisible(mSequencePositionSlider);

    mNoteSequencePositionSlider.setSliderStyle(juce::Slider::SliderStyle::LinearHorizontal);
    mNoteSequencePositionSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, true, 100, 25);
    mNoteSequencePositionSlider.setInterceptsMouseClicks(false, false);
    addAndMakeVisible(mNoteSequencePositionSlider);

    // === Control Mode ===
    mControlModeLabel.setText("Control Mode", juce::dontSendNotification);
    addAndMakeVisible(mControlModeLabel);
    addAndMakeVisible(mControlModeComboBox);
    mControlModeComboBox.addItemList(audioProcessor.getValueTreeState().getParameter("controlMode")->getAllValueStrings(), 1);

    // === Attachments ===
    mDelayTimeAttachment.reset(new juce::AudioProcessorValueTreeState::SliderAttachment(audioProcessor.getValueTreeState(), "delayTime", mDelayTimeSlider));
    mSequencePositionAttachment.reset(new juce::AudioProcessorValueTreeState::SliderAttachment(audioProcessor.getValueTreeState(), "sequencePosition", mSequencePositionSlider));
    mNoteSequencePositionAttachment.reset(new juce::AudioProcessorValueTreeState::SliderAttachment(audioProcessor.getValueTreeState(), "noteSequencePosition", mNoteSequencePositionSlider));
    mControlModeAttachment.reset(new juce::AudioProcessorValueTreeState::ComboBoxAttachment(audioProcessor.getValueTreeState(), "controlMode", mControlModeComboBox));

    // === MIDI Configuration ===
    mMidiConfigLabel.setText("MIDI Configuration", juce::dontSendNotification);
    mMidiConfigLabel.setFont(juce::Font(16.0f, juce::Font::bold));
    addAndMakeVisible(mMidiConfigLabel);

    mMidiInputChannelLabel.setText("MIDI In Channel", juce::dontSendNotification);
    addAndMakeVisible(mMidiInputChannelLabel);
    mMidiOutputChannelLabel.setText("MIDI Out Channel", juce::dontSendNotification);
    addAndMakeVisible(mMidiOutputChannelLabel);

    mMidiInputChannelSlider.setSliderStyle(juce::Slider::SliderStyle::IncDecButtons);
    mMidiInputChannelSlider.setTextBoxStyle(juce::Slider::TextBoxLeft, true, 50, 25);
    addAndMakeVisible(mMidiInputChannelSlider);
    mMidiOutputChannelSlider.setSliderStyle(juce::Slider::SliderStyle::IncDecButtons);
    mMidiOutputChannelSlider.setTextBoxStyle(juce::Slider::TextBoxLeft, true, 50, 25);
    addAndMakeVisible(mMidiOutputChannelSlider);

    mMidiCcSeqResetLabel.setText("Sequence Reset CC", juce::dontSendNotification);
    addAndMakeVisible(mMidiCcSeqResetLabel);
    mMidiCcTimerResetLabel.setText("Timer Reset CC", juce::dontSendNotification);
    addAndMakeVisible(mMidiCcTimerResetLabel);
    mMidiCcSoftResetLabel.setText("Soft Reset CC", juce::dontSendNotification);
    addAndMakeVisible(mMidiCcSoftResetLabel);

    mMidiCcSeqResetSlider.setSliderStyle(juce::Slider::SliderStyle::IncDecButtons);
    mMidiCcSeqResetSlider.setTextBoxStyle(juce::Slider::TextBoxLeft, true, 50, 25);
    addAndMakeVisible(mMidiCcSeqResetSlider);
    mMidiCcTimerResetSlider.setSliderStyle(juce::Slider::SliderStyle::IncDecButtons);
    mMidiCcTimerResetSlider.setTextBoxStyle(juce::Slider::TextBoxLeft, true, 50, 25);
    addAndMakeVisible(mMidiCcTimerResetSlider);
    mMidiCcSoftResetSlider.setSliderStyle(juce::Slider::SliderStyle::IncDecButtons);
    mMidiCcSoftResetSlider.setTextBoxStyle(juce::Slider::TextBoxLeft, true, 50, 25);
    addAndMakeVisible(mMidiCcSoftResetSlider);

    mMidiCcSeqResetModeLabel.setText("Seq Reset Mode", juce::dontSendNotification);
    addAndMakeVisible(mMidiCcSeqResetModeLabel);
    addAndMakeVisible(mMidiCcSeqResetModeComboBox);
    mMidiCcSeqResetModeComboBox.addItemList(audioProcessor.getValueTreeState().getParameter("midiCcSeqResetMode")->getAllValueStrings(), 1);

    mMidiCcTimerResetModeLabel.setText("Timer Reset Mode", juce::dontSendNotification);
    addAndMakeVisible(mMidiCcTimerResetModeLabel);
    addAndMakeVisible(mMidiCcTimerResetModeComboBox);
    mMidiCcTimerResetModeComboBox.addItemList(audioProcessor.getValueTreeState().getParameter("midiCcTimerResetMode")->getAllValueStrings(), 1);

    mMidiCcSoftResetModeLabel.setText("Soft Reset Mode", juce::dontSendNotification);
    addAndMakeVisible(mMidiCcSoftResetModeLabel);
    addAndMakeVisible(mMidiCcSoftResetModeComboBox);
    mMidiCcSoftResetModeComboBox.addItemList(audioProcessor.getValueTreeState().getParameter("midiCcSoftResetMode")->getAllValueStrings(), 1);

    mMidiInputChannelAttachment.reset(new juce::AudioProcessorValueTreeState::SliderAttachment(audioProcessor.getValueTreeState(), "midiInputChannel", mMidiInputChannelSlider));
    mMidiOutputChannelAttachment.reset(new juce::AudioProcessorValueTreeState::SliderAttachment(audioProcessor.getValueTreeState(), "midiOutputChannel", mMidiOutputChannelSlider));
    mMidiCcSeqResetAttachment.reset(new juce::AudioProcessorValueTreeState::SliderAttachment(audioProcessor.getValueTreeState(), "midiCcSeqReset", mMidiCcSeqResetSlider));
    mMidiCcTimerResetAttachment.reset(new juce::AudioProcessorValueTreeState::SliderAttachment(audioProcessor.getValueTreeState(), "midiCcTimerReset", mMidiCcTimerResetSlider));
    mMidiCcSoftResetAttachment.reset(new juce::AudioProcessorValueTreeState::SliderAttachment(audioProcessor.getValueTreeState(), "midiCcSoftReset", mMidiCcSoftResetSlider));
    mMidiCcSeqResetModeAttachment.reset(new juce::AudioProcessorValueTreeState::ComboBoxAttachment(audioProcessor.getValueTreeState(), "midiCcSeqResetMode", mMidiCcSeqResetModeComboBox));
    mMidiCcTimerResetModeAttachment.reset(new juce::AudioProcessorValueTreeState::ComboBoxAttachment(audioProcessor.getValueTreeState(), "midiCcTimerResetMode", mMidiCcTimerResetModeComboBox));
    mMidiCcSoftResetModeAttachment.reset(new juce::AudioProcessorValueTreeState::ComboBoxAttachment(audioProcessor.getValueTreeState(), "midiCcSoftResetMode", mMidiCcSoftResetModeComboBox));


    // === OSC Configuration ===
    mOscConfigLabel.setText("OSC Configuration", juce::dontSendNotification);
    mOscConfigLabel.setFont(juce::Font(16.0f, juce::Font::bold));
    addAndMakeVisible(mOscConfigLabel);
    mOscHostAddressLabel.setText("Host IP Address", juce::dontSendNotification);
    addAndMakeVisible(mOscHostAddressLabel);
    mOscSendPortLabel.setText("Send Port", juce::dontSendNotification);
    addAndMakeVisible(mOscSendPortLabel);
    mOscReceivePortLabel.setText("Receive Port", juce::dontSendNotification);
    addAndMakeVisible(mOscReceivePortLabel);

    mOscHostAddressEditor.setText(audioProcessor.getValueTreeState().state.getProperty("oscHostAddress").toString());
    mOscHostAddressEditor.addListener(this);
    addAndMakeVisible(mOscHostAddressEditor);
    mOscSendPortSlider.setSliderStyle(juce::Slider::SliderStyle::IncDecButtons);
    mOscSendPortSlider.setTextBoxStyle(juce::Slider::TextBoxLeft, true, 70, 25);
    addAndMakeVisible(mOscSendPortSlider);
    mOscReceivePortSlider.setSliderStyle(juce::Slider::SliderStyle::IncDecButtons);
    mOscReceivePortSlider.setTextBoxStyle(juce::Slider::TextBoxLeft, true, 70, 25);
    addAndMakeVisible(mOscReceivePortSlider);

    mOscSendPortAttachment.reset(new juce::AudioProcessorValueTreeState::SliderAttachment(audioProcessor.getValueTreeState(), "oscSendPort", mOscSendPortSlider));
    mOscReceivePortAttachment.reset(new juce::AudioProcessorValueTreeState::SliderAttachment(audioProcessor.getValueTreeState(), "oscReceivePort", mOscReceivePortSlider));

    // === Delay Adjustment ===
    mDelayAdjustLabel.setText("Delay Adjust (samples)", juce::dontSendNotification);
    addAndMakeVisible(mDelayAdjustLabel);
    mDelayAdjustSlider.setSliderStyle(juce::Slider::SliderStyle::LinearHorizontal);
    mDelayAdjustSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 25);
    addAndMakeVisible(mDelayAdjustSlider);

    addAndMakeVisible(mDelayAdjustFwdButton);
    addAndMakeVisible(mDelayAdjustBwdButton);
    mDelayAdjustFwdButton.onClick = [this] {
        mDelayAdjustSlider.setValue(mDelayAdjustSlider.getValue() + 1);
    };
    mDelayAdjustBwdButton.onClick = [this] {
        mDelayAdjustSlider.setValue(mDelayAdjustSlider.getValue() - 1);
    };

    mDelayAdjustCcLabel.setText("Delay Adjust CC", juce::dontSendNotification);
    addAndMakeVisible(mDelayAdjustCcLabel);
    mDelayAdjustFwdCcLabel.setText("Forward", juce::dontSendNotification);
    addAndMakeVisible(mDelayAdjustFwdCcLabel);
    mDelayAdjustBwdCcLabel.setText("Backward", juce::dontSendNotification);
    addAndMakeVisible(mDelayAdjustBwdCcLabel);

    mDelayAdjustFwdCcSlider.setSliderStyle(juce::Slider::SliderStyle::IncDecButtons);
    mDelayAdjustFwdCcSlider.setTextBoxStyle(juce::Slider::TextBoxLeft, true, 50, 25);
    addAndMakeVisible(mDelayAdjustFwdCcSlider);
    mDelayAdjustBwdCcSlider.setSliderStyle(juce::Slider::SliderStyle::IncDecButtons);
    mDelayAdjustBwdCcSlider.setTextBoxStyle(juce::Slider::TextBoxLeft, true, 50, 25);
    addAndMakeVisible(mDelayAdjustBwdCcSlider);

    mDelayAdjustAttachment.reset(new juce::AudioProcessorValueTreeState::SliderAttachment(audioProcessor.getValueTreeState(), "delayAdjust", mDelayAdjustSlider));
    mDelayAdjustFwdCcAttachment.reset(new juce::AudioProcessorValueTreeState::SliderAttachment(audioProcessor.getValueTreeState(), "midiCcDelayAdjustFwd", mDelayAdjustFwdCcSlider));
    mDelayAdjustBwdCcAttachment.reset(new juce::AudioProcessorValueTreeState::SliderAttachment(audioProcessor.getValueTreeState(), "midiCcDelayAdjustBwd", mDelayAdjustBwdCcSlider));

    mDelayAdjustCcStepLabel.setText("CC Step", juce::dontSendNotification);
    addAndMakeVisible(mDelayAdjustCcStepLabel);
    mDelayAdjustCcStepSlider.setSliderStyle(juce::Slider::SliderStyle::IncDecButtons);
    mDelayAdjustCcStepSlider.setTextBoxStyle(juce::Slider::TextBoxLeft, true, 50, 25);
    addAndMakeVisible(mDelayAdjustCcStepSlider);
    mDelayAdjustCcStepAttachment.reset(new juce::AudioProcessorValueTreeState::SliderAttachment(audioProcessor.getValueTreeState(), "delayAdjustCcStep", mDelayAdjustCcStepSlider));

    // Add listener for controlMode
    audioProcessor.getValueTreeState().addParameterListener("controlMode", this);

    // Set initial state
    updateControlEnablement();

    setSize (400, 800);
}

AmenBreakChopperAudioProcessorEditor::~AmenBreakChopperAudioProcessorEditor()
{
    audioProcessor.getValueTreeState().removeParameterListener("controlMode", this);
}

//==============================================================================
void AmenBreakChopperAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}

void AmenBreakChopperAudioProcessorEditor::resized()
{
    const int labelWidth = 150;
    const int controlWidth = getWidth() - labelWidth - 20;
    const int rowHeight = 25;
    const int sliderHeight = 50;
    int y = 10;

    // === Live Status ===
    mStatusLabel.setBounds(10, y, getWidth() - 20, rowHeight);
    y += rowHeight + 5;
    mDelayTimeLabel.setBounds(10, y, labelWidth, rowHeight);
    mDelayTimeSlider.setBounds(10, y + rowHeight - 15, getWidth() - 20, sliderHeight);
    y += sliderHeight + 10;
    mSequencePositionLabel.setBounds(10, y, labelWidth, rowHeight);
    mSequencePositionSlider.setBounds(10, y + rowHeight - 15, getWidth() - 20, sliderHeight);
    y += sliderHeight + 10;
    mNoteSequencePositionLabel.setBounds(10, y, labelWidth, rowHeight);
    mNoteSequencePositionSlider.setBounds(10, y + rowHeight - 15, getWidth() - 20, sliderHeight);
    y += sliderHeight + 15;

    // === Control Mode ===
    mControlModeLabel.setBounds(10, y, labelWidth, rowHeight);
    mControlModeComboBox.setBounds(10 + labelWidth, y, controlWidth, rowHeight);
    y += rowHeight + 15;

    // === MIDI Configuration ===
    mMidiConfigLabel.setBounds(10, y, getWidth() - 20, rowHeight);
    y += rowHeight + 5;
    mMidiInputChannelLabel.setBounds(10, y, labelWidth, rowHeight);
    mMidiInputChannelSlider.setBounds(10 + labelWidth, y, 100, rowHeight);
    y += rowHeight + 5;
    mMidiOutputChannelLabel.setBounds(10, y, labelWidth, rowHeight);
    mMidiOutputChannelSlider.setBounds(10 + labelWidth, y, 100, rowHeight);
    y += rowHeight + 5;
    mMidiCcSeqResetLabel.setBounds(10, y, labelWidth, rowHeight);
    mMidiCcSeqResetSlider.setBounds(10 + labelWidth, y, 100, rowHeight);
    mMidiCcSeqResetModeComboBox.setBounds(10 + labelWidth + 105, y, controlWidth - 105, rowHeight);
    y += rowHeight + 5;
    mMidiCcTimerResetLabel.setBounds(10, y, labelWidth, rowHeight);
    mMidiCcTimerResetSlider.setBounds(10 + labelWidth, y, 100, rowHeight);
    mMidiCcTimerResetModeComboBox.setBounds(10 + labelWidth + 105, y, controlWidth - 105, rowHeight);
    y += rowHeight + 5;
    mMidiCcSoftResetLabel.setBounds(10, y, labelWidth, rowHeight);
    mMidiCcSoftResetSlider.setBounds(10 + labelWidth, y, 100, rowHeight);
    mMidiCcSoftResetModeComboBox.setBounds(10 + labelWidth + 105, y, controlWidth - 105, rowHeight);
    y += rowHeight + 15;

    // === OSC Configuration ===
    mOscConfigLabel.setBounds(10, y, getWidth() - 20, rowHeight);
    y += rowHeight + 5;
    mOscHostAddressLabel.setBounds(10, y, labelWidth, rowHeight);
    mOscHostAddressEditor.setBounds(10 + labelWidth, y, controlWidth, rowHeight);
    y += rowHeight + 5;
    mOscSendPortLabel.setBounds(10, y, labelWidth, rowHeight);
    mOscSendPortSlider.setBounds(10 + labelWidth, y, 120, rowHeight);
    y += rowHeight + 5;
    mOscReceivePortLabel.setBounds(10, y, labelWidth, rowHeight);
    mOscReceivePortSlider.setBounds(10 + labelWidth, y, 120, rowHeight);
    y += rowHeight + 15;

    // === Delay Adjustment ===
    mDelayAdjustLabel.setBounds(10, y, labelWidth, rowHeight);
    mDelayAdjustSlider.setBounds(10 + labelWidth, y, controlWidth - 80, rowHeight);
    mDelayAdjustBwdButton.setBounds(10 + labelWidth + controlWidth - 75, y, 35, rowHeight);
    mDelayAdjustFwdButton.setBounds(10 + labelWidth + controlWidth - 35, y, 35, rowHeight);
    y += rowHeight + 5;
    mDelayAdjustCcLabel.setBounds(10, y, getWidth() - 20, rowHeight);
    y += rowHeight + 5;
    mDelayAdjustBwdCcLabel.setBounds(10, y, labelWidth, rowHeight);
    mDelayAdjustBwdCcSlider.setBounds(10 + labelWidth, y, 100, rowHeight);
    y += rowHeight + 5;
    mDelayAdjustFwdCcLabel.setBounds(10, y, labelWidth, rowHeight);
    mDelayAdjustFwdCcSlider.setBounds(10 + labelWidth, y, 100, rowHeight);
    y += rowHeight + 5;
    mDelayAdjustCcStepLabel.setBounds(10, y, labelWidth, rowHeight);
    mDelayAdjustCcStepSlider.setBounds(10 + labelWidth, y, 100, rowHeight);
}

void AmenBreakChopperAudioProcessorEditor::textEditorTextChanged(juce::TextEditor& editor)
{
    if (&editor == &mOscHostAddressEditor)
    {
        audioProcessor.setOscHostAddress(mOscHostAddressEditor.getText());
    }
}

void AmenBreakChopperAudioProcessorEditor::parameterChanged(const juce::String& parameterID, float newValue)
{
    if (parameterID == "controlMode")
    {
        updateControlEnablement();
    }
}

void AmenBreakChopperAudioProcessorEditor::updateControlEnablement()
{
    auto* controlModeParam = audioProcessor.getValueTreeState().getRawParameterValue("controlMode");
    const bool isOscMode = controlModeParam->load() > 0.5f; // 0 is Internal, 1 is OSC
    const bool isInternalMode = !isOscMode;

    // MIDI controls are enabled in Internal mode
    mMidiConfigLabel.setEnabled(isInternalMode);
    mMidiInputChannelLabel.setEnabled(isInternalMode);
    mMidiInputChannelSlider.setEnabled(isInternalMode);
    mMidiOutputChannelLabel.setEnabled(isInternalMode);
    mMidiOutputChannelSlider.setEnabled(isInternalMode);
    mMidiCcSeqResetLabel.setEnabled(isInternalMode);
    mMidiCcSeqResetSlider.setEnabled(isInternalMode);
    mMidiCcTimerResetLabel.setEnabled(isInternalMode);
    mMidiCcTimerResetSlider.setEnabled(isInternalMode);
    mMidiCcSoftResetLabel.setEnabled(isInternalMode);
    mMidiCcSoftResetSlider.setEnabled(isInternalMode);

    mMidiCcSeqResetModeComboBox.setEnabled(isInternalMode);
    mMidiCcTimerResetModeComboBox.setEnabled(isInternalMode);
    mMidiCcSoftResetModeComboBox.setEnabled(isInternalMode);

    // OSC controls are enabled in OSC mode
    mOscConfigLabel.setEnabled(isOscMode);
    mOscHostAddressLabel.setEnabled(isOscMode);
    mOscHostAddressEditor.setEnabled(isOscMode);
    mOscSendPortLabel.setEnabled(isOscMode);
    mOscSendPortSlider.setEnabled(isOscMode);
    mOscReceivePortLabel.setEnabled(isOscMode);
    mOscReceivePortSlider.setEnabled(isOscMode);

    // Delay adjustment controls are always enabled
    mDelayAdjustLabel.setEnabled(true);
    mDelayAdjustSlider.setEnabled(true);
    mDelayAdjustFwdButton.setEnabled(true);
    mDelayAdjustBwdButton.setEnabled(true);
    mDelayAdjustCcLabel.setEnabled(isInternalMode);
    mDelayAdjustFwdCcLabel.setEnabled(isInternalMode);
    mDelayAdjustFwdCcSlider.setEnabled(isInternalMode);
    mDelayAdjustBwdCcLabel.setEnabled(isInternalMode);
    mDelayAdjustBwdCcSlider.setEnabled(isInternalMode);
    mDelayAdjustCcStepLabel.setEnabled(isInternalMode);
    mDelayAdjustCcStepSlider.setEnabled(isInternalMode);
}