/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
AmenBreakControllerAudioProcessor::AmenBreakControllerAudioProcessor()
     : AudioProcessor (BusesProperties()
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
       mValueTreeState(*this, nullptr, "PARAMETERS", createParameterLayout())
{
    // OSC Sender
    mSender.connect("127.0.0.1", 9002);

    // OSC Receiver
    mReceiver.addListener(this);
    mReceiver.connect(9001);
}

AmenBreakControllerAudioProcessor::~AmenBreakControllerAudioProcessor()
{
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout AmenBreakControllerAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    juce::StringArray controlModes = { "Internal", "OSC" };
    layout.add(std::make_unique<juce::AudioParameterChoice>("controlMode", "Control Mode", controlModes, 0));
    layout.add(std::make_unique<juce::AudioParameterInt>("delayTime", "Delay Time", 0, 15, 0));
    layout.add(std::make_unique<juce::AudioParameterInt>("sequencePosition", "Sequence Position", 0, 15, 0));
    layout.add(std::make_unique<juce::AudioParameterInt>("noteSequencePosition", "Note Sequence Position", 0, 15, 0));
    layout.add(std::make_unique<juce::AudioParameterInt>("midiInputChannel", "MIDI In Channel", 0, 16, 0));
    layout.add(std::make_unique<juce::AudioParameterInt>("midiOutputChannel", "MIDI Out Channel", 1, 16, 1));
    return layout;
}

juce::AudioProcessorValueTreeState& AmenBreakControllerAudioProcessor::getValueTreeState()
{
    return mValueTreeState;
}

//==============================================================================
void AmenBreakControllerAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    mSampleRate = sampleRate;
    // Initialize sequencer state
    mSamplesUntilNextEighthNote = 0.0;
    mSequencePosition = 0;
    mNoteSequencePosition = 0;
    mLastReceivedNoteValue = 0;
    mSequenceResetQueued = false;
    mTimerResetQueued = false;
    mNewNoteReceived = false;
    mSoftResetQueued = false;
}

void AmenBreakControllerAudioProcessor::releaseResources()
{
}

bool AmenBreakControllerAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    if (layouts.getMainOutputChannelSet().size() != 0)
        return false;
   #if ! JucePlugin_IsSynth
    if (layouts.getMainInputChannelSet().size() != 0)
        return false;
   #endif
    return true;
  #endif
}

void AmenBreakControllerAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    buffer.clear();

    juce::ScopedNoDenormals noDenormals;

    juce::AudioPlayHead* playHead = getPlayHead();
    bool isPlaying = false;
    if (playHead != nullptr)
    {
        if (auto positionInfo = playHead->getPosition())
            isPlaying = positionInfo->getIsPlaying();
    }

    if (!isPlaying)
        return;

    double bpm = 120.0;
    if (playHead != nullptr)
    {
        if (auto positionInfo = playHead->getPosition())
        {
            if (positionInfo->getBpm().hasValue())
                bpm = *positionInfo->getBpm();
        }
    }
    const double samplesPerEighthNote = (60.0 / bpm) * 0.5 * mSampleRate;

    auto* modeParam = mValueTreeState.getRawParameterValue("controlMode");
    const bool isInternalMode = modeParam->load() < 0.5f;
    const int midiInChannel = (int)mValueTreeState.getRawParameterValue("midiInputChannel")->load();
    const int midiOutChannel = (int)mValueTreeState.getRawParameterValue("midiOutputChannel")->load();

    juce::MidiBuffer processedMidi;
    for (const auto metadata : midiMessages)
    {
        auto message = metadata.getMessage();
        if (midiInChannel == 0 || message.getChannel() == midiInChannel)
        {
            if (message.isNoteOn())
            {
                int noteNumber = message.getNoteNumber();
                if (noteNumber >= 0 && noteNumber <= 15)
                {
                    mLastReceivedNoteValue = noteNumber;
                    mNoteSequencePosition = noteNumber;
                    mNewNoteReceived = true;
                }
            }
            else if (message.isController())
            {
                if (message.getControllerNumber() == 93) mSequenceResetQueued = true;
                if (message.getControllerNumber() == 106) mTimerResetQueued = true;
                if (message.getControllerNumber() == 97)  mSoftResetQueued = true;
            }
        }
    }
    midiMessages.clear();

    const int bufferLength = buffer.getNumSamples();
    for (int sample = 0; sample < bufferLength; ++sample)
    {
        if (mTimerResetQueued)
        {
            mSamplesUntilNextEighthNote = 0.0;
            mSequencePosition = 0;
            mNoteSequencePosition = 0;
            mTimerResetQueued = false;
        }

        if (mSamplesUntilNextEighthNote <= 0)
        {
            mSamplesUntilNextEighthNote += samplesPerEighthNote;

            if (mSequenceResetQueued)
            {
                mNoteSequencePosition = mSequencePosition;
                auto* delayTimeParam = mValueTreeState.getParameter("delayTime");
                if (delayTimeParam != nullptr) delayTimeParam->setValueNotifyingHost(0.0f);
                mSender.send(juce::OSCMessage("/delayTime", 0));
                mSequenceResetQueued = false;
            }

            if (mSoftResetQueued)
            {
                mSequencePosition = 0;
                mNoteSequencePosition = 0;
                mSoftResetQueued = false;
            }

            if (mNewNoteReceived && isInternalMode)
            {
                const int diff = mSequencePosition - mLastReceivedNoteValue;
                const int newDelayTime = (diff % 16 + 16) % 16;
                auto* delayTimeParam = mValueTreeState.getParameter("delayTime");
                if (delayTimeParam != nullptr) delayTimeParam->setValueNotifyingHost(static_cast<float>(newDelayTime) / 15.0f);
                mSender.send(juce::OSCMessage("/delayTime", newDelayTime));
            }

            auto* seqPosParam = mValueTreeState.getParameter("sequencePosition");
            if (seqPosParam != nullptr) seqPosParam->setValueNotifyingHost(static_cast<float>(mSequencePosition) / 15.0f);

            auto* noteSeqPosParam = mValueTreeState.getParameter("noteSequencePosition");
            if (noteSeqPosParam != nullptr) noteSeqPosParam->setValueNotifyingHost(static_cast<float>(mNoteSequencePosition) / 15.0f);
            
            mSender.send(juce::OSCMessage("/sequencePosition", mSequencePosition));
            mSender.send(juce::OSCMessage("/noteSequencePosition", mNoteSequencePosition));

            const int note1 = mNoteSequencePosition;
            const int note2 = 32 + mSequencePosition;
            const juce::uint8 velocity = 100;
            const int noteDurationInSamples = 50;

            processedMidi.addEvent(juce::MidiMessage::noteOn(midiOutChannel, note1, velocity), sample);
            processedMidi.addEvent(juce::MidiMessage::noteOff(midiOutChannel, note1), sample + noteDurationInSamples);
            processedMidi.addEvent(juce::MidiMessage::noteOn(midiOutChannel, note2, velocity), sample);
            processedMidi.addEvent(juce::MidiMessage::noteOff(midiOutChannel, note2), sample + noteDurationInSamples);

            mNewNoteReceived = false;

            mSequencePosition++;
            if (mSequencePosition > 15) mSequencePosition = 0;

            mNoteSequencePosition++;
            if (mNoteSequencePosition > 15) mNoteSequencePosition = 0;
        }

        mSamplesUntilNextEighthNote--;
    }
    midiMessages.swapWith(processedMidi);
}

//==============================================================================
void AmenBreakControllerAudioProcessor::oscMessageReceived(const juce::OSCMessage& message)
{
    auto* modeParam = mValueTreeState.getRawParameterValue("controlMode");
    const bool isInternalMode = modeParam->load() < 0.5f;
    if (isInternalMode) return;

    if (message.getAddressPattern() == "/delayTime")
    {
        if (message.size() > 0 && message[0].isInt32())
        {
            int newDelayTime = message[0].getInt32();
            if (newDelayTime >= 0 && newDelayTime <= 15)
            {
                auto* delayTimeParam = mValueTreeState.getParameter("delayTime");
                if (delayTimeParam != nullptr)
                    delayTimeParam->setValueNotifyingHost(static_cast<float>(newDelayTime) / 15.0f);
            }
        }
    }
}

// Other functions...
const juce::String AmenBreakControllerAudioProcessor::getName() const { return JucePlugin_Name; }
 bool AmenBreakControllerAudioProcessor::acceptsMidi() const { return true; }
 bool AmenBreakControllerAudioProcessor::producesMidi() const { return true; }
 bool AmenBreakControllerAudioProcessor::isMidiEffect() const { return true; }
 double AmenBreakControllerAudioProcessor::getTailLengthSeconds() const { return 0.0; }
 int AmenBreakControllerAudioProcessor::getNumPrograms() { return 1; }
 int AmenBreakControllerAudioProcessor::getCurrentProgram() { return 0; }
 void AmenBreakControllerAudioProcessor::setCurrentProgram (int index) { juce::ignoreUnused(index); }
 const juce::String AmenBreakControllerAudioProcessor::getProgramName (int index) { juce::ignoreUnused(index); return {}; }
 void AmenBreakControllerAudioProcessor::changeProgramName (int index, const juce::String& newName) { juce::ignoreUnused(index, newName); }
 bool AmenBreakControllerAudioProcessor::hasEditor() const { return true; }
 juce::AudioProcessorEditor* AmenBreakControllerAudioProcessor::createEditor() { return new AmenBreakControllerAudioProcessorEditor (*this); }
 void AmenBreakControllerAudioProcessor::getStateInformation (juce::MemoryBlock& destData) { auto state = mValueTreeState.copyState(); std::unique_ptr<juce::XmlElement> xml (state.createXml()); copyXmlToBinary (*xml, destData); }
 void AmenBreakControllerAudioProcessor::setStateInformation (const void* data, int sizeInBytes) { std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes)); if (xmlState.get() != nullptr) if (xmlState->hasTagName (mValueTreeState.state.getType())) mValueTreeState.replaceState (juce::ValueTree::fromXml (*xmlState)); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new AmenBreakControllerAudioProcessor(); }