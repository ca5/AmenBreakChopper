/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
AmenBreakControllerAudioProcessorEditor::AmenBreakControllerAudioProcessorEditor (AmenBreakControllerAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // === Live Status ===
    mStatusLabel.setText("Live Status", juce::dontSendNotification);
    mStatusLabel.setFont(juce::Font(16.0f, juce::Font::bold));
    addAndMakeVisible(mStatusLabel);

    mSequencePositionLabel.setText("Sequence Position", juce::dontSendNotification);
    addAndMakeVisible(mSequencePositionLabel);
    mNoteSequencePositionLabel.setText("Note Sequence Position", juce::dontSendNotification);
    addAndMakeVisible(mNoteSequencePositionLabel);

    mSequencePositionSlider.setSliderStyle(juce::Slider::SliderStyle::LinearHorizontal);
    mSequencePositionSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, true, 100, 25);
    mSequencePositionSlider.setInterceptsMouseClicks(false, false);
    addAndMakeVisible(mSequencePositionSlider);

    mNoteSequencePositionSlider.setSliderStyle(juce::Slider::SliderStyle::LinearHorizontal);
    mNoteSequencePositionSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, true, 100, 25);
    mNoteSequencePositionSlider.setInterceptsMouseClicks(false, false);
    addAndMakeVisible(mNoteSequencePositionSlider);

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

    // === OSC Triggers ===
    mOscTriggerLabel.setText("OSC Triggers", juce::dontSendNotification);
    mOscTriggerLabel.setFont(juce::Font(16.0f, juce::Font::bold));
    addAndMakeVisible(mOscTriggerLabel);

    mSequenceResetButton.setButtonText("Sequence Reset");
    mSequenceResetButton.addListener(this);
    addAndMakeVisible(mSequenceResetButton);

    mTimerResetButton.setButtonText("Timer Reset");
    mTimerResetButton.addListener(this);
    addAndMakeVisible(mTimerResetButton);

    mSoftResetButton.setButtonText("Soft Reset");
    mSoftResetButton.addListener(this);
    addAndMakeVisible(mSoftResetButton);

    // === Attachments ===
    mSequencePositionAttachment.reset(new juce::AudioProcessorValueTreeState::SliderAttachment(audioProcessor.getValueTreeState(), "sequencePosition", mSequencePositionSlider));
    mNoteSequencePositionAttachment.reset(new juce::AudioProcessorValueTreeState::SliderAttachment(audioProcessor.getValueTreeState(), "noteSequencePosition", mNoteSequencePositionSlider));
    mMidiInputChannelAttachment.reset(new juce::AudioProcessorValueTreeState::SliderAttachment(audioProcessor.getValueTreeState(), "midiInputChannel", mMidiInputChannelSlider));
    mMidiOutputChannelAttachment.reset(new juce::AudioProcessorValueTreeState::SliderAttachment(audioProcessor.getValueTreeState(), "midiOutputChannel", mMidiOutputChannelSlider));
    mOscSendPortAttachment.reset(new juce::AudioProcessorValueTreeState::SliderAttachment(audioProcessor.getValueTreeState(), "oscSendPort", mOscSendPortSlider));
    mOscReceivePortAttachment.reset(new juce::AudioProcessorValueTreeState::SliderAttachment(audioProcessor.getValueTreeState(), "oscReceivePort", mOscReceivePortSlider));

    setSize (400, 550);
}

AmenBreakControllerAudioProcessorEditor::~AmenBreakControllerAudioProcessorEditor()
{
}

//==============================================================================
void AmenBreakControllerAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}

void AmenBreakControllerAudioProcessorEditor::resized()
{
    const int labelWidth = 150;
    const int controlWidth = getWidth() - labelWidth - 20;
    const int rowHeight = 25;
    const int sliderHeight = 50;
    int y = 10;

    // === Live Status ===
    mStatusLabel.setBounds(10, y, getWidth() - 20, rowHeight);
    y += rowHeight + 5;
    mSequencePositionLabel.setBounds(10, y, labelWidth, rowHeight);
    mSequencePositionSlider.setBounds(10, y + rowHeight - 15, getWidth() - 20, sliderHeight);
    y += sliderHeight + 10;
    mNoteSequencePositionLabel.setBounds(10, y, labelWidth, rowHeight);
    mNoteSequencePositionSlider.setBounds(10, y + rowHeight - 15, getWidth() - 20, sliderHeight);
    y += sliderHeight + 15;

    // === MIDI Configuration ===
    mMidiConfigLabel.setBounds(10, y, getWidth() - 20, rowHeight);
    y += rowHeight + 5;
    mMidiInputChannelLabel.setBounds(10, y, labelWidth, rowHeight);
    mMidiInputChannelSlider.setBounds(10 + labelWidth, y, 100, rowHeight);
    y += rowHeight + 5;
    mMidiOutputChannelLabel.setBounds(10, y, labelWidth, rowHeight);
    mMidiOutputChannelSlider.setBounds(10 + labelWidth, y, 100, rowHeight);
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

    // === OSC Triggers ===
    mOscTriggerLabel.setBounds(10, y, getWidth() - 20, rowHeight);
    y += rowHeight + 5;
    mSequenceResetButton.setBounds(10, y, 120, rowHeight);
    mTimerResetButton.setBounds(140, y, 120, rowHeight);
    mSoftResetButton.setBounds(270, y, 120, rowHeight);
}

void AmenBreakControllerAudioProcessorEditor::buttonClicked(juce::Button* button)
{
    if (button == &mSequenceResetButton)
    {
        audioProcessor.sendOscMessage(juce::OSCMessage("/sequenceReset"));
    }
    else if (button == &mTimerResetButton)
    {
        audioProcessor.sendOscMessage(juce::OSCMessage("/timerReset"));
    }
    else if (button == &mSoftResetButton)
    {
        audioProcessor.sendOscMessage(juce::OSCMessage("/softReset"));
    }
}

void AmenBreakControllerAudioProcessorEditor::textEditorTextChanged(juce::TextEditor& editor)
{
    if (&editor == &mOscHostAddressEditor)
    {
        audioProcessor.setOscHostAddress(mOscHostAddressEditor.getText());
    }
}
