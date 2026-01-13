/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <atomic>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_osc/juce_osc.h>

struct MidiClockTracker {
  double lastClockTime{0.0};
  double detectedBpm{120.0};
  int clockCount{0};
  std::vector<double> clockIntervals;

  void reset() {
    lastClockTime = 0.0;
    clockCount = 0;
    clockIntervals.clear();
  }

  void processClockMessage(double time) {
    if (lastClockTime > 0.0) {
      double interval = time - lastClockTime;
      if (interval > 0.0) {
        clockIntervals.push_back(interval);
        if (clockIntervals.size() > 24) { // Average over 24 clocks (1 beat)
          clockIntervals.erase(clockIntervals.begin());
        }

        // Issue #26: Update BPM only on 8th note ticks (every 12 clocks)
        // This reduces jitter in waveform/position display
        // Issue #26: Update BPM only on 8th note ticks (every 12 clocks)
        // This reduces jitter in waveform/position display
        if (clockIntervals.size() >= 4 && (clockCount % 12 == 0)) {
          double sum = 0.0;
          for (double i : clockIntervals)
            sum += i;
          double avgInterval = sum / clockIntervals.size();
          // MIDI Clock is 24 ppq. BPM = 60 / (24 * interval)
          if (avgInterval > 0.001) {
            detectedBpm = 60.0 / (24.0 * avgInterval);
          }
        }
      }
    }
    lastClockTime = time;
    clockCount++;
  }
};

//==============================================================================
/**
 */
class AmenBreakChopperAudioProcessor
    : public juce::AudioProcessor,
      private juce::OSCReceiver::Listener<
          juce::OSCReceiver::MessageLoopCallback>,
      public juce::AudioProcessorValueTreeState::Listener {
public:
  //==============================================================================
  AmenBreakChopperAudioProcessor();
  ~AmenBreakChopperAudioProcessor() override;

  //==============================================================================
  void prepareToPlay(double sampleRate, int samplesPerBlock) override;
  void releaseResources() override;

#ifndef JucePlugin_PreferredChannelConfigurations
  bool isBusesLayoutSupported(const BusesLayout &layouts) const override;
#endif

  void processBlock(juce::AudioBuffer<float> &, juce::MidiBuffer &) override;

  //==============================================================================
  juce::AudioProcessorEditor *createEditor() override;
  bool hasEditor() const override;

  //==============================================================================
  const juce::String getName() const override;

  bool acceptsMidi() const override;
  bool producesMidi() const override;
  bool isMidiEffect() const override;
  double getTailLengthSeconds() const override;

  //==============================================================================
  int getNumPrograms() override;
  int getCurrentProgram() override;
  void setCurrentProgram(int index) override;
  const juce::String getProgramName(int index) override;
  void changeProgramName(int index, const juce::String &newName) override;

  //==============================================================================
  void getStateInformation(juce::MemoryBlock &destData) override;
  void setStateInformation(const void *data, int sizeInBytes) override;

  juce::AudioProcessorValueTreeState &getValueTreeState();
  void setOscHostAddress(const juce::String &hostAddress);

  // Callbacks for UI
  std::function<void(int, int)> onNoteEvent;

  // Reset Commands
  void performSequenceReset();
  void performSoftReset();
  void performHardReset();
  void triggerNoteFromUi(int noteNumber);
  
  // Waveform Data
  std::vector<float> getWaveformData();
  int getSequencePosition() { return mSequencePosition.load(); }
  std::atomic<bool> mWaveformDirty{true};

private:
  //==============================================================================
  void parameterChanged(const juce::String &parameterID,
                        float newValue) override;
  static juce::AudioProcessorValueTreeState::ParameterLayout
  createParameterLayout();
  juce::AudioProcessorValueTreeState mValueTreeState;

  juce::AudioBuffer<float> mDelayBuffer;
  int mWritePosition{0};
  double mSampleRate{0.0};
  std::atomic<double> mCurrentBpm{120.0};
  std::atomic<double> mSamplesToNextBeat{0.0};

  // --- Sequencer State ---
  double mNextEighthNotePpq{0.0};
  std::atomic<int> mSequencePosition{0};
  int mNoteSequencePosition{0};
  int mLastReceivedNoteValue{0};
  std::atomic<int> mUiTriggeredNote{-1}; // Atomic for thread safety
  bool mSequenceResetQueued{false};
  bool mHardResetQueued{false};
  bool mNewNoteReceived{false};
  bool mSoftResetQueued{false};
  int mLastNote1{-1};
  int mLastNote2{-1};

  // --- CC Value State ---
  int mLastSeqResetCcValue{0};
  int mLastHardResetCcValue{0};
  int mLastSoftResetCcValue{0};
  int mLastDelayAdjustFwdCcValue{0};
  int mLastDelayAdjustBwdCcValue{0};
  int mLastDelayAdjust{0};

  // --- External Input & Clock State ---
  MidiClockTracker mMidiClockTracker;
  std::atomic<bool> mUsingMidiClock{false};
  double mMidiClockPpq{0.0}; // Synthesized phase from MIDI clock ticks

  // --- OSC State ---
  juce::OSCSender mSender;
  juce::OSCReceiver mReceiver;

  void oscMessageReceived(const juce::OSCMessage &message) override;

  bool shouldTriggerReset(int mode, int previousValue, int currentValue);

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AmenBreakChopperAudioProcessor)
};
