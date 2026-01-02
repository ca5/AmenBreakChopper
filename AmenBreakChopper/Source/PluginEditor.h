/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include "PluginProcessor.h"
#include <JuceHeader.h>

//==============================================================================
/**
 */
class AmenBreakChopperAudioProcessorEditor
    : public juce::AudioProcessorEditor,
      public juce::AudioProcessorValueTreeState::Listener,
      public juce::Timer {
public:
  AmenBreakChopperAudioProcessorEditor(AmenBreakChopperAudioProcessor &);
  ~AmenBreakChopperAudioProcessorEditor() override;

  //==============================================================================
  void paint(juce::Graphics &) override;
  void resized() override;
  void timerCallback() override;
  void parameterChanged(const juce::String &parameterID,
                        float newValue) override;

private:
  AmenBreakChopperAudioProcessor &audioProcessor;

  juce::WebBrowserComponent webView;

  // Helper to send events to JS
  void sendParameterUpdate(const juce::String &paramId, float newValue);

  // Initial state setup
  void syncAllParametersToFrontend();

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(
      AmenBreakChopperAudioProcessorEditor)
};