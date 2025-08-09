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
class AmenBreakChopperAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    AmenBreakChopperAudioProcessorEditor (AmenBreakChopperAudioProcessor&);
    ~AmenBreakChopperAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    AmenBreakChopperAudioProcessor& audioProcessor;

    juce::Slider mDelayTimeSlider;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mDelayTimeAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AmenBreakChopperAudioProcessorEditor)
};
