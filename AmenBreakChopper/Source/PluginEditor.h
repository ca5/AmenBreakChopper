/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include "PluginProcessor.h"
#include <JuceHeader.h>
#include <map>

//==============================================================================
/**
 */
class AmenBreakChopperAudioProcessorEditor
    : public juce::AudioProcessorEditor,
      public juce::Timer {
public:
  AmenBreakChopperAudioProcessorEditor(AmenBreakChopperAudioProcessor &);
  ~AmenBreakChopperAudioProcessorEditor() override;

  //==============================================================================
  void paint(juce::Graphics &) override;
  void resized() override;
  void timerCallback() override;

private:
  AmenBreakChopperAudioProcessor &audioProcessor;

  juce::WebBrowserComponent webView;

  // Cache to track parameter changes
  std::map<juce::String, float> lastParameterValues;

  // Helper to send events to JS
  void sendParameterUpdate(const juce::String &paramId, float newValue);

  // Initial state setup
  void syncAllParametersToFrontend();
  void sendDeviceList();

  // Standalone Device Management
  juce::AudioDeviceManager* deviceManager = nullptr;

public:
  void setDeviceManager(juce::AudioDeviceManager* dm) { deviceManager = dm; }

  // Debug helper
  juce::String lastMidiDebugLog;

  bool isWebViewLoaded{false};
  int framesWaited{0};
  bool hasFrontendConnected{false};
  int retryCounter{0};

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(
      AmenBreakChopperAudioProcessorEditor)
};