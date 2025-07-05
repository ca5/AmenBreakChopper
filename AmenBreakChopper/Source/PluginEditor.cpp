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

    mFeedbackSlider.setSliderStyle(juce::Slider::SliderStyle::LinearHorizontal);
    mFeedbackSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, true, 100, 25);
    addAndMakeVisible(mFeedbackSlider);

    mMixSlider.setSliderStyle(juce::Slider::SliderStyle::LinearHorizontal);
    mMixSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, true, 100, 25);
    addAndMakeVisible(mMixSlider);

    mDelayTimeAttachment.reset(new juce::AudioProcessorValueTreeState::SliderAttachment(audioProcessor.getValueTreeState(), "delayTime", mDelayTimeSlider));
    mFeedbackAttachment.reset(new juce::AudioProcessorValueTreeState::SliderAttachment(audioProcessor.getValueTreeState(), "feedback", mFeedbackSlider));
    mMixAttachment.reset(new juce::AudioProcessorValueTreeState::SliderAttachment(audioProcessor.getValueTreeState(), "mix", mMixSlider));

    setSize (400, 300);
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
    mFeedbackSlider.setBounds(10, 70, getWidth() - 20, 50);
    mMixSlider.setBounds(10, 130, getWidth() - 20, 50);
}
