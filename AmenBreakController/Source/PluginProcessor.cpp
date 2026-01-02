/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
AmenBreakControllerAudioProcessor::AmenBreakControllerAudioProcessor()
    : AudioProcessor(
          BusesProperties()
              .withInput("Input", juce::AudioChannelSet::stereo(), true)
              .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      mValueTreeState(*this, nullptr, "PARAMETERS", createParameterLayout()) {
  mValueTreeState.state.setProperty("oscHostAddress", "127.0.0.1", nullptr);
  mReceiver.addListener(this);
  mValueTreeState.addParameterListener("oscSendPort", this);
  mValueTreeState.addParameterListener("oscReceivePort", this);
}

AmenBreakControllerAudioProcessor::~AmenBreakControllerAudioProcessor() {}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout
AmenBreakControllerAudioProcessor::createParameterLayout() {
  juce::AudioProcessorValueTreeState::ParameterLayout layout;

  layout.add(std::make_unique<juce::AudioParameterInt>(
      "sequencePosition", "Sequence Position", 0, 15, 0));
  layout.add(std::make_unique<juce::AudioParameterInt>(
      "noteSequencePosition", "Note Sequence Position", 0, 15, 0));

  // MIDI Settings
  layout.add(std::make_unique<juce::AudioParameterInt>(
      "midiInputChannel", "MIDI In Channel", 0, 16, 0));
  layout.add(std::make_unique<juce::AudioParameterInt>(
      "midiOutputChannel", "MIDI Out Channel", 1, 16, 1));

  // MIDI CC Settings
  juce::StringArray ccModes = {"Any", "Gate-On", "Gate-Off"};
  layout.add(std::make_unique<juce::AudioParameterInt>(
      "midiCcSeqReset", "MIDI CC Seq Reset", 0, 127, 93));
  layout.add(std::make_unique<juce::AudioParameterChoice>(
      "midiCcSeqResetMode", "Seq Reset Mode", ccModes, 1));
  layout.add(std::make_unique<juce::AudioParameterInt>(
      "midiCcHardReset", "MIDI CC Hard Reset", 0, 127, 106));
  layout.add(std::make_unique<juce::AudioParameterChoice>(
      "midiCcHardResetMode", "Hard Reset Mode", ccModes, 1));
  layout.add(std::make_unique<juce::AudioParameterInt>(
      "midiCcSoftReset", "MIDI CC Soft Reset", 0, 127, 97));
  layout.add(std::make_unique<juce::AudioParameterChoice>(
      "midiCcSoftResetMode", "Soft Reset Mode", ccModes, 1));

  // OSC Settings
  layout.add(std::make_unique<juce::AudioParameterInt>(
      "oscSendPort", "OSC Send Port", 1, 65535, 9002));
  layout.add(std::make_unique<juce::AudioParameterInt>(
      "oscReceivePort", "OSC Receive Port", 1, 65535, 9001));

  return layout;
}

juce::AudioProcessorValueTreeState &
AmenBreakControllerAudioProcessor::getValueTreeState() {
  return mValueTreeState;
}

void AmenBreakControllerAudioProcessor::parameterChanged(
    const juce::String &parameterID, float newValue) {
  if (parameterID == "oscSendPort") {
    auto hostAddress =
        mValueTreeState.state.getProperty("oscHostAddress").toString();
    if (!mSender.connect(hostAddress, (int)newValue))
      juce::Logger::writeToLog(
          "AmenBreakController: Failed to connect OSC sender on port change.");
  } else if (parameterID == "oscReceivePort") {
    if (!mReceiver.connect((int)newValue))
      juce::Logger::writeToLog("AmenBreakController: Failed to connect OSC "
                               "receiver on port change.");
  }
}

void AmenBreakControllerAudioProcessor::setOscHostAddress(
    const juce::String &hostAddress) {
  mValueTreeState.state.setProperty("oscHostAddress", hostAddress, nullptr);
  auto sendPort =
      (int)mValueTreeState.getRawParameterValue("oscSendPort")->load();
  if (!mSender.connect(hostAddress, sendPort))
    juce::Logger::writeToLog(
        "AmenBreakController: Failed to connect OSC sender on host change.");
}

void AmenBreakControllerAudioProcessor::sendOscMessage(
    const juce::OSCMessage &message) {
  mSender.send(message);
}

//==============================================================================
bool AmenBreakControllerAudioProcessor::shouldTriggerReset(int mode,
                                                           int previousValue,
                                                           int currentValue) {
  switch (mode) {
  case 0: // Any
    return true;
  case 1: // Gate-On
    return currentValue >= 65 && previousValue < 65;
  case 2: // Gate-Off
    return currentValue <= 63 && previousValue > 63;
  default:
    return false;
  }
}

void AmenBreakControllerAudioProcessor::prepareToPlay(double sampleRate,
                                                      int samplesPerBlock) {
  mLastSeqResetCcValue = 0;
  mLastHardResetCcValue = 0;
  mLastSoftResetCcValue = 0;

  auto hostAddress =
      mValueTreeState.state.getProperty("oscHostAddress").toString();
  auto sendPort =
      (int)mValueTreeState.getRawParameterValue("oscSendPort")->load();
  if (!mSender.connect(hostAddress, sendPort))
    juce::Logger::writeToLog(
        "AmenBreakController: Failed to connect OSC sender.");

  auto receivePort =
      (int)mValueTreeState.getRawParameterValue("oscReceivePort")->load();
  if (!mReceiver.connect(receivePort))
    juce::Logger::writeToLog(
        "AmenBreakController: Failed to connect OSC receiver.");
}

void AmenBreakControllerAudioProcessor::releaseResources() {
  // When playback stops, turn off any hanging notes.
  const juce::ScopedLock sl(mQueueLock);
  const int midiOutChannel =
      (int)mValueTreeState.getRawParameterValue("midiOutputChannel")->load();
  if (mLastOscNoteSeq >= 0)
    mMidiOutputQueue.addEvent(
        juce::MidiMessage::noteOff(midiOutChannel, 32 + mLastOscNoteSeq), 0);
  if (mLastOscNoteNoteSeq >= 0)
    mMidiOutputQueue.addEvent(
        juce::MidiMessage::noteOff(midiOutChannel, mLastOscNoteNoteSeq), 0);
}

bool AmenBreakControllerAudioProcessor::isBusesLayoutSupported(
    const BusesLayout &layouts) const {
  return true;
}

void AmenBreakControllerAudioProcessor::processBlock(
    juce::AudioBuffer<float> &buffer, juce::MidiBuffer &midiMessages) {
  buffer.clear();
  const int midiInChannel =
      (int)mValueTreeState.getRawParameterValue("midiInputChannel")->load();

  // --- Handle MIDI In -> OSC Out ---
  for (const auto metadata : midiMessages) {
    auto message = metadata.getMessage();
    if (midiInChannel == 0 || message.getChannel() == midiInChannel) {
      if (message.isNoteOn()) {
        int noteNumber = message.getNoteNumber();
        if (noteNumber >= 0 && noteNumber <= 15) {
          mSender.send(
              juce::OSCMessage("/setNoteSequencePosition", noteNumber));
        }
      } else if (message.isController()) {
        const int controllerNumber = message.getControllerNumber();
        const int controllerValue = message.getControllerValue();

        const int ccSeqReset =
            (int)mValueTreeState.getRawParameterValue("midiCcSeqReset")->load();
        const int ccHardReset =
            (int)mValueTreeState.getRawParameterValue("midiCcHardReset")
                ->load();
        const int ccSoftReset =
            (int)mValueTreeState.getRawParameterValue("midiCcSoftReset")
                ->load();

        if (controllerNumber == ccSeqReset) {
          const int mode =
              (int)mValueTreeState.getRawParameterValue("midiCcSeqResetMode")
                  ->load();
          if (shouldTriggerReset(mode, mLastSeqResetCcValue, controllerValue))
            mSender.send(juce::OSCMessage("/sequenceReset"));
          mLastSeqResetCcValue = controllerValue;
        }

        if (controllerNumber == ccHardReset) {
          const int mode =
              (int)mValueTreeState.getRawParameterValue("midiCcHardResetMode")
                  ->load();
          if (shouldTriggerReset(mode, mLastHardResetCcValue, controllerValue))
            mSender.send(juce::OSCMessage("/hardReset"));
          mLastHardResetCcValue = controllerValue;
        }

        if (controllerNumber == ccSoftReset) {
          const int mode =
              (int)mValueTreeState.getRawParameterValue("midiCcSoftResetMode")
                  ->load();
          if (shouldTriggerReset(mode, mLastSoftResetCcValue, controllerValue))
            mSender.send(juce::OSCMessage("/softReset"));
          mLastSoftResetCcValue = controllerValue;
        }
      }
    }
  }
  midiMessages.clear(); // We've processed all incoming MIDI.

  // --- Handle OSC In -> MIDI Out ---
  const juce::ScopedLock sl(mQueueLock);
  if (!mMidiOutputQueue.isEmpty()) {
    for (const auto &metadata : mMidiOutputQueue) {
      // Add all queued messages at the start of the buffer.
      midiMessages.addEvent(metadata.getMessage(), 0);
    }
    mMidiOutputQueue.clear();
  }
}

//==============================================================================
void AmenBreakControllerAudioProcessor::oscMessageReceived(
    const juce::OSCMessage &message) {
  const juce::ScopedLock sl(mQueueLock);
  const int midiOutChannel =
      (int)mValueTreeState.getRawParameterValue("midiOutputChannel")->load();
  const juce::uint8 velocity = 100;

  if (message.getAddressPattern() == "/sequencePosition") {
    if (message.size() > 0 && message[0].isInt32()) {
      int newPosition = message[0].getInt32();
      mValueTreeState.getParameter("sequencePosition")
          ->setValueNotifyingHost(static_cast<float>(newPosition) / 15.0f);

      // Turn off the last note from this sequence
      if (mLastOscNoteSeq >= 0)
        mMidiOutputQueue.addEvent(
            juce::MidiMessage::noteOff(midiOutChannel, 32 + mLastOscNoteSeq),
            0);

      // Turn on the new note
      const int note = 32 + newPosition;
      mMidiOutputQueue.addEvent(
          juce::MidiMessage::noteOn(midiOutChannel, note, velocity), 0);

      // Store the new note number
      mLastOscNoteSeq = newPosition;
    }
  } else if (message.getAddressPattern() == "/noteSequencePosition") {
    if (message.size() > 0 && message[0].isInt32()) {
      int newPosition = message[0].getInt32();
      mValueTreeState.getParameter("noteSequencePosition")
          ->setValueNotifyingHost(static_cast<float>(newPosition) / 15.0f);

      // Turn off the last note from this sequence
      if (mLastOscNoteNoteSeq >= 0)
        mMidiOutputQueue.addEvent(
            juce::MidiMessage::noteOff(midiOutChannel, mLastOscNoteNoteSeq), 0);

      // Turn on the new note
      const int note = newPosition;
      mMidiOutputQueue.addEvent(
          juce::MidiMessage::noteOn(midiOutChannel, note, velocity), 0);

      // Store the new note number
      mLastOscNoteNoteSeq = newPosition;
    }
  }
}

//==============================================================================
void AmenBreakControllerAudioProcessor::getStateInformation(
    juce::MemoryBlock &destData) {
  auto state = mValueTreeState.copyState();
  state.removeProperty("sequencePosition", nullptr);
  state.removeProperty("noteSequencePosition", nullptr);
  std::unique_ptr<juce::XmlElement> xml(state.createXml());
  copyXmlToBinary(*xml, destData);
}

void AmenBreakControllerAudioProcessor::setStateInformation(const void *data,
                                                            int sizeInBytes) {
  std::unique_ptr<juce::XmlElement> xmlState(
      getXmlFromBinary(data, sizeInBytes));

  if (xmlState.get() != nullptr)
    if (xmlState->hasTagName(mValueTreeState.state.getType()))
      mValueTreeState.replaceState(juce::ValueTree::fromXml(*xmlState));

  if (auto *p = mValueTreeState.getParameter("sequencePosition"))
    p->setValueNotifyingHost(p->getDefaultValue());
  if (auto *p = mValueTreeState.getParameter("noteSequencePosition"))
    p->setValueNotifyingHost(p->getDefaultValue());
}

// Other functions...
const juce::String AmenBreakControllerAudioProcessor::getName() const {
  return JucePlugin_Name;
}
bool AmenBreakControllerAudioProcessor::acceptsMidi() const { return true; }
bool AmenBreakControllerAudioProcessor::producesMidi() const { return true; }
bool AmenBreakControllerAudioProcessor::isMidiEffect() const { return true; }
double AmenBreakControllerAudioProcessor::getTailLengthSeconds() const {
  return 0.0;
}
int AmenBreakControllerAudioProcessor::getNumPrograms() { return 1; }
int AmenBreakControllerAudioProcessor::getCurrentProgram() { return 0; }
void AmenBreakControllerAudioProcessor::setCurrentProgram(int index) {
  juce::ignoreUnused(index);
}
const juce::String
AmenBreakControllerAudioProcessor::getProgramName(int index) {
  juce::ignoreUnused(index);
  return {};
}
void AmenBreakControllerAudioProcessor::changeProgramName(
    int index, const juce::String &newName) {
  juce::ignoreUnused(index, newName);
}
bool AmenBreakControllerAudioProcessor::hasEditor() const { return true; }
juce::AudioProcessorEditor *AmenBreakControllerAudioProcessor::createEditor() {
  return new AmenBreakControllerAudioProcessorEditor(*this);
}

juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter() {
  return new AmenBreakControllerAudioProcessor();
}