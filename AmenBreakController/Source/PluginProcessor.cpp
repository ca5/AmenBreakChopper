/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
AmenBreakControllerAudioProcessor::AmenBreakControllerAudioProcessor()
     : AudioProcessor (BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true).withOutput("Output", juce::AudioChannelSet::stereo(), true)),
       mValueTreeState(*this, nullptr, "PARAMETERS", createParameterLayout())
{
    mValueTreeState.state.setProperty("oscHostAddress", "127.0.0.1", nullptr);
    mReceiver.addListener(this);
    mValueTreeState.addParameterListener("oscSendPort", this);
    mValueTreeState.addParameterListener("oscReceivePort", this);
}

AmenBreakControllerAudioProcessor::~AmenBreakControllerAudioProcessor()
{
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout AmenBreakControllerAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add(std::make_unique<juce::AudioParameterInt>("sequencePosition", "Sequence Position", 0, 15, 0));
    layout.add(std::make_unique<juce::AudioParameterInt>("noteSequencePosition", "Note Sequence Position", 0, 15, 0));
    layout.add(std::make_unique<juce::AudioParameterInt>("midiInputChannel", "MIDI In Channel", 0, 16, 0));
    layout.add(std::make_unique<juce::AudioParameterInt>("midiOutputChannel", "MIDI Out Channel", 1, 16, 1));
    layout.add(std::make_unique<juce::AudioParameterInt>("oscSendPort", "OSC Send Port", 1, 65535, 9002));
    layout.add(std::make_unique<juce::AudioParameterInt>("oscReceivePort", "OSC Receive Port", 1, 65535, 9001));

    juce::StringArray resetModes = { "Any", "Gate-On", "Gate-Off" };
    layout.add(std::make_unique<juce::AudioParameterChoice>("oscSeqResetMode", "OSC Seq Reset Mode", resetModes, 0));
    layout.add(std::make_unique<juce::AudioParameterChoice>("oscTimerResetMode", "OSC Timer Reset Mode", resetModes, 0));
    layout.add(std::make_unique<juce::AudioParameterChoice>("oscSoftResetMode", "OSC Soft Reset Mode", resetModes, 0));

    return layout;
}

juce::AudioProcessorValueTreeState& AmenBreakControllerAudioProcessor::getValueTreeState()
{
    return mValueTreeState;
}

void AmenBreakControllerAudioProcessor::parameterChanged(const juce::String& parameterID, float newValue)
{
    if (parameterID == "oscSendPort")
    {
        auto hostAddress = mValueTreeState.state.getProperty("oscHostAddress").toString();
        if (!mSender.connect(hostAddress, (int)newValue))
            juce::Logger::writeToLog("AmenBreakController: Failed to connect OSC sender on port change.");
    }
    else if (parameterID == "oscReceivePort")
    {
        if (!mReceiver.connect((int)newValue))
            juce::Logger::writeToLog("AmenBreakController: Failed to connect OSC receiver on port change.");
    }
}

void AmenBreakControllerAudioProcessor::setOscHostAddress(const juce::String& hostAddress)
{
    mValueTreeState.state.setProperty("oscHostAddress", hostAddress, nullptr);
    auto sendPort = (int)mValueTreeState.getRawParameterValue("oscSendPort")->load();
    if (!mSender.connect(hostAddress, sendPort))
        juce::Logger::writeToLog("AmenBreakController: Failed to connect OSC sender on host change.");
}

void AmenBreakControllerAudioProcessor::sendOscMessage(const juce::OSCMessage& message)
{
    mSender.send(message);
}

//==============================================================================
void AmenBreakControllerAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    auto hostAddress = mValueTreeState.state.getProperty("oscHostAddress").toString();
    auto sendPort = (int)mValueTreeState.getRawParameterValue("oscSendPort")->load();
    if (!mSender.connect(hostAddress, sendPort))
        juce::Logger::writeToLog("AmenBreakController: Failed to connect OSC sender.");

    auto receivePort = (int)mValueTreeState.getRawParameterValue("oscReceivePort")->load();
    if (!mReceiver.connect(receivePort))
        juce::Logger::writeToLog("AmenBreakController: Failed to connect OSC receiver.");
}

void AmenBreakControllerAudioProcessor::releaseResources()
{
}

bool AmenBreakControllerAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return true;
}

void AmenBreakControllerAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    buffer.clear();
    const int midiInChannel = (int)mValueTreeState.getRawParameterValue("midiInputChannel")->load();

    // --- Handle MIDI In -> OSC Out ---
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
                    mSender.send(juce::OSCMessage("/setNoteSequencePosition", noteNumber));
                }
            }
            else if (message.isController())
            {
                if (message.getControllerNumber() == 93) mSender.send(juce::OSCMessage("/sequenceReset"));
                if (message.getControllerNumber() == 106) mSender.send(juce::OSCMessage("/timerReset"));
                if (message.getControllerNumber() == 97)  mSender.send(juce::OSCMessage("/softReset"));
            }
        }
    }
    midiMessages.clear(); // We've processed all incoming MIDI.

    // --- Handle OSC In -> MIDI Out ---
    const juce::ScopedLock sl (mQueueLock);
    if (!mMidiOutputQueue.isEmpty())
    {
        const int noteDurationInSamples = 50;
        for (const auto& metadata : mMidiOutputQueue)
        {
            auto msg = metadata.getMessage();
            // Note-ons at the start of the buffer, note-offs after a short duration
            int samplePos = msg.isNoteOff() ? noteDurationInSamples : 0;
            midiMessages.addEvent(msg, samplePos);
        }
        mMidiOutputQueue.clear();
    }
}

//==============================================================================
void AmenBreakControllerAudioProcessor::oscMessageReceived(const juce::OSCMessage& message)
{
    const juce::ScopedLock sl (mQueueLock);
    const int midiOutChannel = (int)mValueTreeState.getRawParameterValue("midiOutputChannel")->load();
    const juce::uint8 velocity = 100;

    if (message.getAddressPattern() == "/sequencePosition")
    {
        if (message.size() > 0 && message[0].isInt32())
        {
            int newPosition = message[0].getInt32();
            mValueTreeState.getParameter("sequencePosition")->setValueNotifyingHost(static_cast<float>(newPosition) / 15.0f);

            const int note = 32 + newPosition;
            mMidiOutputQueue.addEvent(juce::MidiMessage::noteOn(midiOutChannel, note, velocity), 0);
            mMidiOutputQueue.addEvent(juce::MidiMessage::noteOff(midiOutChannel, note), 1);
        }
    }
    else if (message.getAddressPattern() == "/noteSequencePosition")
    {
        if (message.size() > 0 && message[0].isInt32())
        {
            int newPosition = message[0].getInt32();
            mValueTreeState.getParameter("noteSequencePosition")->setValueNotifyingHost(static_cast<float>(newPosition) / 15.0f);

            const int note = newPosition;
            mMidiOutputQueue.addEvent(juce::MidiMessage::noteOn(midiOutChannel, note, velocity), 0);
            mMidiOutputQueue.addEvent(juce::MidiMessage::noteOff(midiOutChannel, note), 1);
        }
    }
}

//==============================================================================
void AmenBreakControllerAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = mValueTreeState.copyState();
    state.removeProperty("sequencePosition", nullptr);
    state.removeProperty("noteSequencePosition", nullptr);
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void AmenBreakControllerAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName(mValueTreeState.state.getType()))
            mValueTreeState.replaceState(juce::ValueTree::fromXml(*xmlState));

    if (auto* p = mValueTreeState.getParameter("sequencePosition"))
        p->setValueNotifyingHost(p->getDefaultValue());
    if (auto* p = mValueTreeState.getParameter("noteSequencePosition"))
        p->setValueNotifyingHost(p->getDefaultValue());
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

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new AmenBreakControllerAudioProcessor(); }