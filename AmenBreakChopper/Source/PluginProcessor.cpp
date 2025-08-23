/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
AmenBreakChopperAudioProcessor::AmenBreakChopperAudioProcessor()
     : AudioProcessor (BusesProperties()
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
       mValueTreeState(*this, nullptr, "PARAMETERS", createParameterLayout())
{
    mValueTreeState.state.setProperty("oscHostAddress", "127.0.0.1", nullptr);
    mReceiver.addListener(this);
    mValueTreeState.addParameterListener("oscSendPort", this);
    mValueTreeState.addParameterListener("oscReceivePort", this);
}

AmenBreakChopperAudioProcessor::~AmenBreakChopperAudioProcessor()
{
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout AmenBreakChopperAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    juce::StringArray controlModes = { "Internal", "OSC" };

    layout.add(std::make_unique<juce::AudioParameterChoice>("controlMode", "Control Mode", controlModes, 0));
    layout.add(std::make_unique<juce::AudioParameterInt>("delayTime", "Delay Time", 0, 15, 0));
    layout.add(std::make_unique<juce::AudioParameterInt>("sequencePosition", "Sequence Position", 0, 15, 0));
    layout.add(std::make_unique<juce::AudioParameterInt>("noteSequencePosition", "Note Sequence Position", 0, 15, 0));
    layout.add(std::make_unique<juce::AudioParameterInt>("midiInputChannel", "MIDI In Channel", 0, 16, 0));
    layout.add(std::make_unique<juce::AudioParameterInt>("midiOutputChannel", "MIDI Out Channel", 1, 16, 1));

    // OSC Settings
    layout.add(std::make_unique<juce::AudioParameterInt>("oscSendPort", "OSC Send Port", 1, 65535, 9001));
    layout.add(std::make_unique<juce::AudioParameterInt>("oscReceivePort", "OSC Receive Port", 1, 65535, 9002));

    // MIDI CC Settings
    layout.add(std::make_unique<juce::AudioParameterInt>("midiCcSeqReset", "MIDI CC Seq Reset", 0, 127, 93));
    layout.add(std::make_unique<juce::AudioParameterInt>("midiCcTimerReset", "MIDI CC Timer Reset", 0, 127, 106));
    layout.add(std::make_unique<juce::AudioParameterInt>("midiCcSoftReset", "MIDI CC Soft Reset", 0, 127, 97));

    return layout;
}

void AmenBreakChopperAudioProcessor::parameterChanged(const juce::String& parameterID, float newValue)
{
    if (parameterID == "oscSendPort")
    {
        auto hostAddress = mValueTreeState.state.getProperty("oscHostAddress").toString();
        if (!mSender.connect(hostAddress, (int)newValue))
            juce::Logger::writeToLog("AmenBreakChopper: Failed to connect OSC sender on port change.");
    }
    else if (parameterID == "oscReceivePort")
    {
        if (!mReceiver.connect((int)newValue))
            juce::Logger::writeToLog("AmenBreakChopper: Failed to connect OSC receiver on port change.");
    }
}

void AmenBreakChopperAudioProcessor::setOscHostAddress(const juce::String& hostAddress)
{
    mValueTreeState.state.setProperty("oscHostAddress", hostAddress, nullptr);
    auto sendPort = (int)mValueTreeState.getRawParameterValue("oscSendPort")->load();
    if (!mSender.connect(hostAddress, sendPort))
        juce::Logger::writeToLog("AmenBreakChopper: Failed to connect OSC sender on host change.");
}


juce::AudioProcessorValueTreeState& AmenBreakChopperAudioProcessor::getValueTreeState()
{
    return mValueTreeState;
}

const juce::String AmenBreakChopperAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool AmenBreakChopperAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool AmenBreakChopperAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool AmenBreakChopperAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double AmenBreakChopperAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int AmenBreakChopperAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int AmenBreakChopperAudioProcessor::getCurrentProgram()
{
    return 0;
}

void AmenBreakChopperAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String AmenBreakChopperAudioProcessor::getProgramName (int index)
{
    return {};
}

void AmenBreakChopperAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void AmenBreakChopperAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // OSC Sender
    auto hostAddress = mValueTreeState.state.getProperty("oscHostAddress").toString();
    auto sendPort = (int)mValueTreeState.getRawParameterValue("oscSendPort")->load();
    if (!mSender.connect(hostAddress, sendPort))
        juce::Logger::writeToLog("AmenBreakChopper: Failed to connect OSC sender.");

    // OSC Receiver
    auto receivePort = (int)mValueTreeState.getRawParameterValue("oscReceivePort")->load();
    if (!mReceiver.connect(receivePort))
        juce::Logger::writeToLog("AmenBreakChopper: Failed to connect OSC receiver.");

    mSampleRate = sampleRate;

    const int numChannels = getTotalNumInputChannels();
    // Calculate buffer size for 16 eighth notes at a very slow BPM (e.g., 30 BPM)
    // to ensure the buffer is always large enough.
    // Max delay = 16 * (1/8 note) = 2 whole notes = 8 beats.
    // Duration of 8 beats at 30 BPM = 8 * (60 / 30) = 8 * 2 = 16 seconds.
    const double maxDelayTimeInSeconds = 16.0;
    const int delayBufferSize = static_cast<int>(maxDelayTimeInSeconds * sampleRate);

    mDelayBuffer.setSize(numChannels, delayBufferSize);
    mDelayBuffer.clear();

    // Initialize sequencer state
    mNextEighthNotePpq = 0.0;
    mSequencePosition = 0;
    mNoteSequencePosition = 0;
    mLastReceivedNoteValue = 0;
    mSequenceResetQueued = false;
    mTimerResetQueued = false;
    mNewNoteReceived = false;
    mSoftResetQueued = false;
}

void AmenBreakChopperAudioProcessor::releaseResources()
{
    mDelayBuffer.setSize(0, 0);
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool AmenBreakChopperAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void AmenBreakChopperAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // --- Clear any unused output channels ---
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // --- Get transport state from host ---
    juce::AudioPlayHead* playHead = getPlayHead();
    juce::AudioPlayHead::PositionInfo positionInfo;
    if (playHead != nullptr)
        positionInfo = playHead->getPosition().orFallback(positionInfo);

    if (!positionInfo.getIsPlaying())
    {
        // If transport is not playing, do nothing (pass audio through).
        return;
    }

    // --- Get musical time information ---
    const double sampleRate = getSampleRate();
    const double bpm = positionInfo.getBpm().orFallback(120.0);
    const double ppqAtStartOfBlock = positionInfo.getPpqPosition().orFallback(0.0);
    const double ppqPerSample = bpm / (60.0 * sampleRate);

    // --- Handle transport jumps or looping ---
    // If transport loops or jumps backwards, reset the next note position to align with the grid.
    if (positionInfo.getIsLooping() || (ppqAtStartOfBlock < mNextEighthNotePpq - 0.5))
    {
        // Find the first 8th note position at or after the start of this block.
        mNextEighthNotePpq = std::ceil(ppqAtStartOfBlock * 2.0) / 2.0;
    }

    // --- Get current parameter values ---
    auto* modeParam = mValueTreeState.getRawParameterValue("controlMode");
    const bool isInternalMode = modeParam->load() < 0.5f;
    const int midiInChannel = (int)mValueTreeState.getRawParameterValue("midiInputChannel")->load();
    const int midiOutChannel = (int)mValueTreeState.getRawParameterValue("midiOutputChannel")->load();

    // --- Process incoming MIDI messages ---
    juce::MidiBuffer processedMidi; // Create a new buffer for our generated notes
    for (const auto metadata : midiMessages)
    {
        auto message = metadata.getMessage();
        // Omni mode: if midiInChannel is 0, accept all channels.
        if (midiInChannel == 0 || message.getChannel() == midiInChannel)
        {
            if (message.isNoteOn())
            {
                int noteNumber = message.getNoteNumber();
                if (noteNumber >= 0 && noteNumber <= 15)
                {
                    mLastReceivedNoteValue = noteNumber;
                    mNoteSequencePosition = noteNumber; // MIDI note overrides the note sequence
                    mNewNoteReceived = true;
                }
            }
            else if (message.isController())
            {
                const int ccSeqReset = (int)mValueTreeState.getRawParameterValue("midiCcSeqReset")->load();
                const int ccTimerReset = (int)mValueTreeState.getRawParameterValue("midiCcTimerReset")->load();
                const int ccSoftReset = (int)mValueTreeState.getRawParameterValue("midiCcSoftReset")->load();

                if (message.getControllerNumber() == ccSeqReset) mSequenceResetQueued = true;
                if (message.getControllerNumber() == ccTimerReset) mTimerResetQueued = true;
                if (message.getControllerNumber() == ccSoftReset) mSoftResetQueued = true;
            }
        }
    }
    midiMessages.clear(); // Clear the incoming buffer

    // --- Sequencer Tick Logic (Block-based) ---
    const int bufferLength = buffer.getNumSamples();
    double ppqAtEndOfBlock = ppqAtStartOfBlock + (bufferLength * ppqPerSample);

    while (mNextEighthNotePpq < ppqAtEndOfBlock)
    {
        const int tickSample = static_cast<int>((mNextEighthNotePpq - ppqAtStartOfBlock) / ppqPerSample);

        if (mTimerResetQueued)
        {
            mSequencePosition = 0;
            mNoteSequencePosition = 0;
            mTimerResetQueued = false;
            // Also reset PPQ tracking to the current tick
            mNextEighthNotePpq = std::ceil(ppqAtStartOfBlock * 2.0) / 2.0;
        }

        if (mSequenceResetQueued)
        {
            mNoteSequencePosition = mSequencePosition; // Sync Note-Seq to Main-Seq
            mValueTreeState.getParameter("delayTime")->setValueNotifyingHost(0.0f); // Reset DelayTime
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
            mValueTreeState.getParameter("delayTime")->setValueNotifyingHost(static_cast<float>(newDelayTime) / 15.0f);
        }

        mValueTreeState.getParameter("sequencePosition")->setValueNotifyingHost(static_cast<float>(mSequencePosition) / 15.0f);
        mValueTreeState.getParameter("noteSequencePosition")->setValueNotifyingHost(static_cast<float>(mNoteSequencePosition) / 15.0f);

        mSender.send(juce::OSCMessage("/sequencePosition", mSequencePosition));
        mSender.send(juce::OSCMessage("/noteSequencePosition", mNoteSequencePosition));

        const int note1 = mNoteSequencePosition;
        const int note2 = 32 + mSequencePosition;
        const juce::uint8 velocity = 100;
        const int noteDurationInSamples = 50;

        processedMidi.addEvent(juce::MidiMessage::noteOn(midiOutChannel, note1, velocity), tickSample);
        processedMidi.addEvent(juce::MidiMessage::noteOff(midiOutChannel, note1), tickSample + noteDurationInSamples);
        processedMidi.addEvent(juce::MidiMessage::noteOn(midiOutChannel, note2, velocity), tickSample);
        processedMidi.addEvent(juce::MidiMessage::noteOff(midiOutChannel, note2), tickSample + noteDurationInSamples);

        mNewNoteReceived = false;

        mSequencePosition = (mSequencePosition + 1) % 16;
        mNoteSequencePosition = (mNoteSequencePosition + 1) % 16;

        mNextEighthNotePpq += 0.5; // Advance to the next 8th note position
    }

    midiMessages.swapWith(processedMidi); // Place our generated notes into the main buffer

    // --- Audio Processing Logic (Sample-by-sample) ---
    const int delayBufferLength = mDelayBuffer.getNumSamples();
    auto* delayTimeParam = mValueTreeState.getRawParameterValue("delayTime");
    const int currentDelayTime = static_cast<int>(delayTimeParam->load());

    for (int sample = 0; sample < bufferLength; ++sample)
    {
        // Write input to the delay buffer for this sample
        for (int channel = 0; channel < totalNumInputChannels; ++channel)
        {
            const float* bufferData = buffer.getReadPointer(channel);
            float* delayBufferData = mDelayBuffer.getWritePointer(channel);
            delayBufferData[(mWritePosition + sample) % delayBufferLength] = bufferData[sample];
        }

        // If DelayTime is 0, bypass the effect (output is same as input)
        if (currentDelayTime != 0)
        {
            double eighthNoteTime = (60.0 / bpm) / 2.0;
            int delayTimeInSamples = static_cast<int>(eighthNoteTime * currentDelayTime * sampleRate);

            for (int channel = 0; channel < totalNumInputChannels; ++channel)
            {
                float* delayBufferData = mDelayBuffer.getWritePointer(channel);
                auto* channelData = buffer.getWritePointer(channel);
                const int readPosition = (mWritePosition - delayTimeInSamples + sample + delayBufferLength) % delayBufferLength;
                const float delayedSample = delayBufferData[readPosition];
                channelData[sample] = delayedSample; // 100% Wet
            }
        }
    }

    mWritePosition = (mWritePosition + bufferLength) % delayBufferLength;
}

//==============================================================================
bool AmenBreakChopperAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* AmenBreakChopperAudioProcessor::createEditor()
{
    return new AmenBreakChopperAudioProcessorEditor (*this);
}

//==============================================================================
void AmenBreakChopperAudioProcessor::oscMessageReceived(const juce::OSCMessage& message)
{
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
    else if (message.getAddressPattern() == "/sequenceReset")
    {
        mSequenceResetQueued = true;
    }
    else if (message.getAddressPattern() == "/timerReset")
    {
        mTimerResetQueued = true;
    }
    else if (message.getAddressPattern() == "/softReset")
    {
        mSoftResetQueued = true;
    }
    else if (message.getAddressPattern() == "/setNoteSequencePosition")
    {
        if (message.size() > 0 && message[0].isInt32())
        {
            int noteNumber = message[0].getInt32();
            if (noteNumber >= 0 && noteNumber <= 15)
            {
                mLastReceivedNoteValue = noteNumber;
                mNoteSequencePosition = noteNumber; // OSC note overrides the note sequence
                mNewNoteReceived = true;
            }
        }
    }
}

//==============================================================================
void AmenBreakChopperAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = mValueTreeState.copyState();
    // These parameters should not be saved with the project.
    state.removeProperty("delayTime", nullptr);
    state.removeProperty("sequencePosition", nullptr);
    state.removeProperty("noteSequencePosition", nullptr);
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void AmenBreakChopperAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName(mValueTreeState.state.getType()))
            mValueTreeState.replaceState(juce::ValueTree::fromXml(*xmlState));

    // Always reset these parameters to 0 on load.
    if (auto* p = mValueTreeState.getParameter("delayTime"))
        p->setValueNotifyingHost(p->getDefaultValue());
    if (auto* p = mValueTreeState.getParameter("sequencePosition"))
        p->setValueNotifyingHost(p->getDefaultValue());
    if (auto* p = mValueTreeState.getParameter("noteSequencePosition"))
        p->setValueNotifyingHost(p->getDefaultValue());
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AmenBreakChopperAudioProcessor();
}
