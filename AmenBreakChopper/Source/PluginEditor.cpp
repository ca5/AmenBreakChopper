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

    mDelayTimeAttachment.reset(new juce::AudioProcessorValueTreeState::SliderAttachment(audioProcessor.getValueTreeState(), "delayTime", mDelayTimeSlider));

    setSize (400, 100);
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
}