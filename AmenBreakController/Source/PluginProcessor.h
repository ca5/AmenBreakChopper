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
class AmenBreakControllerAudioProcessor  : public juce::AudioProcessor,
                                           private juce::OSCReceiver::Listener<juce::OSCReceiver::MessageLoopCallback>,
                                           public juce::AudioProcessorValueTreeState::Listener
{
public:
    //==============================================================================
    AmenBreakControllerAudioProcessor();
    ~AmenBreakControllerAudioProcessor() override;

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
    void setOscHostAddress(const juce::String& hostAddress);
    void sendOscMessage(const juce::OSCMessage& message);

private:
    //==============================================================================
    void parameterChanged(const juce::String& parameterID, float newValue) override;
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    juce::AudioProcessorValueTreeState mValueTreeState;

    // --- Thread-safe MIDI queue for OSC->MIDI feedback ---
    juce::CriticalSection mQueueLock;
    juce::MidiBuffer mMidiOutputQueue;

    // --- CC Value State ---
    int mLastSeqResetCcValue { 0 };
    int mLastTimerResetCcValue { 0 };
    int mLastSoftResetCcValue { 0 };

    // --- OSC State ---
    juce::OSCSender mSender;
    juce::OSCReceiver mReceiver;
    int mLastOscNoteSeq { -1 };
    int mLastOscNoteNoteSeq { -1 };

    void oscMessageReceived (const juce::OSCMessage& message) override;
    bool shouldTriggerReset(int mode, int previousValue, int currentValue);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AmenBreakControllerAudioProcessor)
};