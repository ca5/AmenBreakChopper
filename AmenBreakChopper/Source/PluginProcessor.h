/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_osc/juce_osc.h>

//==============================================================================
/**
*/
class AmenBreakChopperAudioProcessor  : public juce::AudioProcessor,
                                      private juce::OSCReceiver::Listener<juce::OSCReceiver::MessageLoopCallback>
{
public:
    //==============================================================================
    AmenBreakChopperAudioProcessor();
    ~AmenBreakChopperAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
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
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getValueTreeState();

private:
    //==============================================================================
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    juce::AudioProcessorValueTreeState mValueTreeState;

    juce::AudioBuffer<float> mDelayBuffer;
    int mWritePosition { 0 };
    double mSampleRate { 0.0 };

    // --- Sequencer State ---
    double mSamplesUntilNextEighthNote { 0.0 };
    int mSequencePosition { 0 };
    int mNoteSequencePosition { 0 };
    int mLastReceivedNoteValue { 0 };
    bool mSequenceResetQueued { false };
    bool mTimerResetQueued { false };
    bool mNewNoteReceived { false };
    bool mSoftResetQueued { false };

    // --- OSC State ---
    juce::OSCSender mSender;
    juce::OSCReceiver mReceiver;

    void oscMessageReceived (const juce::OSCMessage& message) override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AmenBreakChopperAudioProcessor)
};