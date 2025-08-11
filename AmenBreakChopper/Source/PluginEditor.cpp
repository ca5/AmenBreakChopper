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

    mDelayTimeAttachment.reset(new juce::AudioProcessorValueTreeState::SliderAttachment(audioProcessor.getValueTreeState(), "delayTime", mDelayTimeSlider));
    mSequencePositionAttachment.reset(new juce::AudioProcessorValueTreeState::SliderAttachment(audioProcessor.getValueTreeState(), "sequencePosition", mSequencePositionSlider));
    mNoteSequencePositionAttachment.reset(new juce::AudioProcessorValueTreeState::SliderAttachment(audioProcessor.getValueTreeState(), "noteSequencePosition", mNoteSequencePositionSlider));

    setSize (400, 220);
}

AmenBreakChopperAudioProcessorEditor::~AmenBreakChopperAudioProcessorEditor()
{
}

//==============================================================================
void AmenBreakChopperAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}

void AmenBreakChopperAudioProcessorEditor::resized()
{
    mDelayTimeSlider.setBounds(10, 10, getWidth() - 20, 50);
    mSequencePositionSlider.setBounds(10, 70, getWidth() - 20, 50);
    mNoteSequencePositionSlider.setBounds(10, 130, getWidth() - 20, 50);
}