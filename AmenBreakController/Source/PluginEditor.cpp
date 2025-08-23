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
    mControlModeAttachment.reset(new juce::AudioProcessorValueTreeState::ComboBoxAttachment(audioProcessor.getValueTreeState(), "controlMode", mControlModeComboBox));

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
    mDelayTimeAttachment.reset(new juce::AudioProcessorValueTreeState::SliderAttachment(audioProcessor.getValueTreeState(), "delayTime", mDelayTimeSlider));
    mSequencePositionAttachment.reset(new juce::AudioProcessorValueTreeState::SliderAttachment(audioProcessor.getValueTreeState(), "sequencePosition", mSequencePositionSlider));
    mNoteSequencePositionAttachment.reset(new juce::AudioProcessorValueTreeState::SliderAttachment(audioProcessor.getValueTreeState(), "noteSequencePosition", mNoteSequencePositionSlider));
    mMidiInputChannelAttachment.reset(new juce::AudioProcessorValueTreeState::SliderAttachment(audioProcessor.getValueTreeState(), "midiInputChannel", mMidiInputChannelSlider));
    mMidiOutputChannelAttachment.reset(new juce::AudioProcessorValueTreeState::SliderAttachment(audioProcessor.getValueTreeState(), "midiOutputChannel", mMidiOutputChannelSlider));
    mControlModeAttachment.reset(new juce::AudioProcessorValueTreeState::ComboBoxAttachment(audioProcessor.getValueTreeState(), "controlMode", mControlModeComboBox));

    setSize (400, 310);
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
}