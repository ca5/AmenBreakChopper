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
    // Labels
    mControlModeLabel.setText("Control Mode", juce::dontSendNotification);
    mDelayTimeLabel.setText("Delay Time", juce::dontSendNotification);
    mSequencePositionLabel.setText("Sequence Position", juce::dontSendNotification);
    mNoteSequencePositionLabel.setText("Note Sequence Position", juce::dontSendNotification);
    mMidiInputChannelLabel.setText("MIDI In Channel", juce::dontSendNotification);
    mMidiOutputChannelLabel.setText("MIDI Out Channel", juce::dontSendNotification);

    addAndMakeVisible(mControlModeLabel);
    addAndMakeVisible(mDelayTimeLabel);
    addAndMakeVisible(mSequencePositionLabel);
    addAndMakeVisible(mNoteSequencePositionLabel);
    addAndMakeVisible(mMidiInputChannelLabel);
    addAndMakeVisible(mMidiOutputChannelLabel);

    // Controls
    addAndMakeVisible(mControlModeComboBox);
    mControlModeComboBox.addItemList(audioProcessor.getValueTreeState().getParameter("controlMode")->getAllValueStrings(), 1);

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

    mMidiInputChannelSlider.setSliderStyle(juce::Slider::SliderStyle::IncDecButtons);
    mMidiInputChannelSlider.setTextBoxStyle(juce::Slider::TextBoxLeft, true, 50, 25);
    addAndMakeVisible(mMidiInputChannelSlider);

    mMidiOutputChannelSlider.setSliderStyle(juce::Slider::SliderStyle::IncDecButtons);
    mMidiOutputChannelSlider.setTextBoxStyle(juce::Slider::TextBoxLeft, true, 50, 25);
    addAndMakeVisible(mMidiOutputChannelSlider);

    // Attachments
    mControlModeAttachment.reset(new juce::AudioProcessorValueTreeState::ComboBoxAttachment(audioProcessor.getValueTreeState(), "controlMode", mControlModeComboBox));
    mDelayTimeAttachment.reset(new juce::AudioProcessorValueTreeState::SliderAttachment(audioProcessor.getValueTreeState(), "delayTime", mDelayTimeSlider));
    mSequencePositionAttachment.reset(new juce::AudioProcessorValueTreeState::SliderAttachment(audioProcessor.getValueTreeState(), "sequencePosition", mSequencePositionSlider));
    mNoteSequencePositionAttachment.reset(new juce::AudioProcessorValueTreeState::SliderAttachment(audioProcessor.getValueTreeState(), "noteSequencePosition", mNoteSequencePositionSlider));
    mMidiInputChannelAttachment.reset(new juce::AudioProcessorValueTreeState::SliderAttachment(audioProcessor.getValueTreeState(), "midiInputChannel", mMidiInputChannelSlider));
    mMidiOutputChannelAttachment.reset(new juce::AudioProcessorValueTreeState::SliderAttachment(audioProcessor.getValueTreeState(), "midiOutputChannel", mMidiOutputChannelSlider));

    // --- New OSC and MIDI CC UI ---

    // OSC Labels
    mOscConfigLabel.setText("OSC Configuration", juce::dontSendNotification);
    mOscConfigLabel.setFont(juce::Font(16.0f, juce::Font::bold));
    addAndMakeVisible(mOscConfigLabel);
    mOscHostAddressLabel.setText("Host IP Address", juce::dontSendNotification);
    addAndMakeVisible(mOscHostAddressLabel);
    mOscSendPortLabel.setText("Send Port", juce::dontSendNotification);
    addAndMakeVisible(mOscSendPortLabel);
    mOscReceivePortLabel.setText("Receive Port", juce::dontSendNotification);
    addAndMakeVisible(mOscReceivePortLabel);

    // OSC Controls
    mOscHostAddressEditor.setText(audioProcessor.getValueTreeState().state.getProperty("oscHostAddress").toString());
    mOscHostAddressEditor.addListener(this);
    addAndMakeVisible(mOscHostAddressEditor);
    mOscSendPortSlider.setSliderStyle(juce::Slider::SliderStyle::IncDecButtons);
    mOscSendPortSlider.setTextBoxStyle(juce::Slider::TextBoxLeft, true, 70, 25);
    addAndMakeVisible(mOscSendPortSlider);
    mOscReceivePortSlider.setSliderStyle(juce::Slider::SliderStyle::IncDecButtons);
    mOscReceivePortSlider.setTextBoxStyle(juce::Slider::TextBoxLeft, true, 70, 25);
    addAndMakeVisible(mOscReceivePortSlider);

    // MIDI CC Labels
    mMidiCcConfigLabel.setText("MIDI CC Configuration", juce::dontSendNotification);
    mMidiCcConfigLabel.setFont(juce::Font(16.0f, juce::Font::bold));
    addAndMakeVisible(mMidiCcConfigLabel);
    mMidiCcSeqResetLabel.setText("Sequence Reset CC", juce::dontSendNotification);
    addAndMakeVisible(mMidiCcSeqResetLabel);
    mMidiCcTimerResetLabel.setText("Timer Reset CC", juce::dontSendNotification);
    addAndMakeVisible(mMidiCcTimerResetLabel);
    mMidiCcSoftResetLabel.setText("Soft Reset CC", juce::dontSendNotification);
    addAndMakeVisible(mMidiCcSoftResetLabel);

    // MIDI CC Controls
    mMidiCcSeqResetSlider.setSliderStyle(juce::Slider::SliderStyle::IncDecButtons);
    mMidiCcSeqResetSlider.setTextBoxStyle(juce::Slider::TextBoxLeft, true, 50, 25);
    addAndMakeVisible(mMidiCcSeqResetSlider);
    mMidiCcTimerResetSlider.setSliderStyle(juce::Slider::SliderStyle::IncDecButtons);
    mMidiCcTimerResetSlider.setTextBoxStyle(juce::Slider::TextBoxLeft, true, 50, 25);
    addAndMakeVisible(mMidiCcTimerResetSlider);
    mMidiCcSoftResetSlider.setSliderStyle(juce::Slider::SliderStyle::IncDecButtons);
    mMidiCcSoftResetSlider.setTextBoxStyle(juce::Slider::TextBoxLeft, true, 50, 25);
    addAndMakeVisible(mMidiCcSoftResetSlider);

    // New Attachments
    mOscSendPortAttachment.reset(new juce::AudioProcessorValueTreeState::SliderAttachment(audioProcessor.getValueTreeState(), "oscSendPort", mOscSendPortSlider));
    mOscReceivePortAttachment.reset(new juce::AudioProcessorValueTreeState::SliderAttachment(audioProcessor.getValueTreeState(), "oscReceivePort", mOscReceivePortSlider));
    mMidiCcSeqResetAttachment.reset(new juce::AudioProcessorValueTreeState::SliderAttachment(audioProcessor.getValueTreeState(), "midiCcSeqReset", mMidiCcSeqResetSlider));
    mMidiCcTimerResetAttachment.reset(new juce::AudioProcessorValueTreeState::SliderAttachment(audioProcessor.getValueTreeState(), "midiCcTimerReset", mMidiCcTimerResetSlider));
    mMidiCcSoftResetAttachment.reset(new juce::AudioProcessorValueTreeState::SliderAttachment(audioProcessor.getValueTreeState(), "midiCcSoftReset", mMidiCcSoftResetSlider));

    // Add listener for controlMode
    audioProcessor.getValueTreeState().addParameterListener("controlMode", this);

    // Set initial state
    updateControlEnablement();

    setSize (400, 570);
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

    mControlModeLabel.setBounds(10, y, labelWidth, rowHeight);
    mControlModeComboBox.setBounds(10 + labelWidth, y, controlWidth, rowHeight);
    y += 35;

    mDelayTimeLabel.setBounds(10, y, labelWidth, rowHeight);
    mDelayTimeSlider.setBounds(10, y + rowHeight - 15, getWidth() - 20, sliderHeight);
    y += sliderHeight + 10;

    mSequencePositionLabel.setBounds(10, y, labelWidth, rowHeight);
    mSequencePositionSlider.setBounds(10, y + rowHeight - 15, getWidth() - 20, sliderHeight);
    y += sliderHeight + 10;

    mNoteSequencePositionLabel.setBounds(10, y, labelWidth, rowHeight);
    mNoteSequencePositionSlider.setBounds(10, y + rowHeight - 15, getWidth() - 20, sliderHeight);
    y += sliderHeight + 10;

    mMidiInputChannelLabel.setBounds(10, y, labelWidth, rowHeight);
    mMidiInputChannelSlider.setBounds(10 + labelWidth, y, 100, rowHeight);
    y += rowHeight + 5;

    mMidiOutputChannelLabel.setBounds(10, y, labelWidth, rowHeight);
    mMidiOutputChannelSlider.setBounds(10 + labelWidth, y, 100, rowHeight);
    y += rowHeight + 15;

    // --- OSC Configuration ---
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

    // --- MIDI CC Configuration ---
    mMidiCcConfigLabel.setBounds(10, y, getWidth() - 20, rowHeight);
    y += rowHeight + 5;

    mMidiCcSeqResetLabel.setBounds(10, y, labelWidth, rowHeight);
    mMidiCcSeqResetSlider.setBounds(10 + labelWidth, y, 100, rowHeight);
    y += rowHeight + 5;

    mMidiCcTimerResetLabel.setBounds(10, y, labelWidth, rowHeight);
    mMidiCcTimerResetSlider.setBounds(10 + labelWidth, y, 100, rowHeight);
    y += rowHeight + 5;

    mMidiCcSoftResetLabel.setBounds(10, y, labelWidth, rowHeight);
    mMidiCcSoftResetSlider.setBounds(10 + labelWidth, y, 100, rowHeight);
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

    // OSC controls are enabled in OSC mode
    mOscConfigLabel.setEnabled(isOscMode);
    mOscHostAddressLabel.setEnabled(isOscMode);
    mOscHostAddressEditor.setEnabled(isOscMode);
    mOscSendPortLabel.setEnabled(isOscMode);
    mOscSendPortSlider.setEnabled(isOscMode);
    mOscReceivePortLabel.setEnabled(isOscMode);
    mOscReceivePortSlider.setEnabled(isOscMode);

    // MIDI controls are enabled in Internal mode (i.e., NOT OSC mode)
    const bool isInternalMode = !isOscMode;
    mMidiInputChannelLabel.setEnabled(isInternalMode);
    mMidiInputChannelSlider.setEnabled(isInternalMode);
    mMidiOutputChannelLabel.setEnabled(isInternalMode);
    mMidiOutputChannelSlider.setEnabled(isInternalMode);

    mMidiCcConfigLabel.setEnabled(isInternalMode);
    mMidiCcSeqResetLabel.setEnabled(isInternalMode);
    mMidiCcSeqResetSlider.setEnabled(isInternalMode);
    mMidiCcTimerResetLabel.setEnabled(isInternalMode);
    mMidiCcTimerResetSlider.setEnabled(isInternalMode);
    mMidiCcSoftResetLabel.setEnabled(isInternalMode);
    mMidiCcSoftResetSlider.setEnabled(isInternalMode);
}