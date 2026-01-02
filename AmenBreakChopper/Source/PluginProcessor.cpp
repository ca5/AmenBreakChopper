/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
AmenBreakChopperAudioProcessor::AmenBreakChopperAudioProcessor()
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

AmenBreakChopperAudioProcessor::~AmenBreakChopperAudioProcessor() {}

std::vector<float> AmenBreakChopperAudioProcessor::getWaveformData() {
  std::vector<float> waveformData;
  waveformData.reserve(16 * 32);

  // 1. Calculate timing
  // Use stored bpm from processBlock
  double bpm = mCurrentBpm.load();
  if (bpm <= 0.1) bpm = 120.0;
  
  double sampleRate = mSampleRate;
  if (sampleRate <= 0.0) sampleRate = 44100.0;

  double eighthNoteSamples = (60.0 / bpm) / 2.0 * sampleRate;
  
  // 2. Determine "End" of the loop (latest recorded data)
  // The buffer records continuously. 
  // We want to visualize the *last 16 steps* (2 bars of 4/4 usually, or 8 beats).
  // Step 15 ends at mWritePosition.
  // Step 0 starts at mWritePosition - 16 * eighthNoteSamples.
  
  int bufferSize = mDelayBuffer.getNumSamples();
  if (bufferSize == 0) return std::vector<float>(16 * 32, 0.0f);

  if (bufferSize == 0) return std::vector<float>(16 * 32, 0.0f);

  int currentWritePos = mWritePosition; 
  int currentSeqPos = mSequencePosition;
  double samplesToNextBeat = mSamplesToNextBeat.load();
  
  const auto* channelData = mDelayBuffer.getReadPointer(0); // Use Left channel for visualization

  // Loop through 0..15 corresponding to the 16 steps of the sequence.
  for (int stepIndex = 0; stepIndex < 16; ++stepIndex) {
      // Logic:
      // "Current Step" (the one defined by currentSeqPos - 1) ends at "currentWritePos + samplesToNextBeat".
      // Current Step is (currentSeqPos - 1 + 16) % 16.
      
      int currentStepIdx = (currentSeqPos - 1 + 16) % 16;
      
      // We want to find the start/end of 'stepIndex' relative to 'currentStepIdx'.
      // offsetSteps = stepIndex - currentStepIdx;
      // If offsetSteps > 0, it's in the future (relative to the currently playing step start).
      // We want the most recent *completed* or *active* recording of this step.
      
      int diff = stepIndex - currentStepIdx;
      // Wrap diff to be within [-15, 0] roughly? 
      // Actually, define distance in steps BACKWARDS from current step end.
      
      // Time of StepIndex End = Time of CurrentStep End + (diff * duration).
      // Time of CurrentStep End = currentWritePos + samplesToNextBeat.
      
      double endSamplePosFromNow = samplesToNextBeat + (diff * eighthNoteSamples);
      
      // If endSamplePosFromNow > 0, it means it ends in the future.
      // We want the version that is fully recorded or currently recording?
      // For the current step (diff=0), end is in future, start is in past. We show it.
      // For next step (diff=1), end is far in future. We want the previous loop's version.
      // So subtract Loop Duration (16 * duration) until endSamplePosFromNow <= samplesToNextBeat ? 
      // Actually, strictly speaking, we want the most recent data.
      // If diff=1 (Next Step), it hasn't happened yet. So we show (Next Step - 16).
      // If diff=0 (Current Step), it is happening now. We show it.
      
      while (endSamplePosFromNow > samplesToNextBeat) {
          endSamplePosFromNow -= (16.0 * eighthNoteSamples);
      }
      
      double startSamplePosFromNow = endSamplePosFromNow - eighthNoteSamples;
      
      // Convert to buffer indices relative to currentWritePos
      // Index = currentWritePos + offset
      int startIdx = currentWritePos + static_cast<int>(startSamplePosFromNow);
      int endIdx = currentWritePos + static_cast<int>(endSamplePosFromNow);
      
      int len = endIdx - startIdx;
      if (len <= 0) len = 1; 
      
      for (int i = 0; i < 32; ++i) {
          int samplePos = startIdx + (i * len) / 32;
          
          while (samplePos < 0) samplePos += bufferSize;
          while (samplePos >= bufferSize) samplePos -= bufferSize;
          
          waveformData.push_back(channelData[samplePos]);
      }
  }

  return waveformData;
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout
AmenBreakChopperAudioProcessor::createParameterLayout() {
  juce::AudioProcessorValueTreeState::ParameterLayout layout;

  juce::StringArray controlModes = {"Internal", "OSC"};

  layout.add(std::make_unique<juce::AudioParameterChoice>(
      "controlMode", "Control Mode", controlModes, 0));
  layout.add(std::make_unique<juce::AudioParameterInt>("delayTime",
                                                       "Delay Time", 0, 15, 0));
  layout.add(std::make_unique<juce::AudioParameterInt>(
      "sequencePosition", "Sequence Position", 0, 15, 0));
  layout.add(std::make_unique<juce::AudioParameterInt>(
      "noteSequencePosition", "Note Sequence Position", 0, 15, 0));
  layout.add(std::make_unique<juce::AudioParameterInt>(
      "midiInputChannel", "MIDI In Channel", 0, 16, 0));
  layout.add(std::make_unique<juce::AudioParameterInt>(
      "midiOutputChannel", "MIDI Out Channel", 1, 16, 1));

  // OSC Settings
  layout.add(std::make_unique<juce::AudioParameterInt>(
      "oscSendPort", "OSC Send Port", 1, 65535, 9001));
  layout.add(std::make_unique<juce::AudioParameterInt>(
      "oscReceivePort", "OSC Receive Port", 1, 65535, 9002));

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

  // Delay adjustment
  layout.add(std::make_unique<juce::AudioParameterInt>(
      "delayAdjust", "Delay Adjust", -4096, 4096, 0));
  layout.add(std::make_unique<juce::AudioParameterInt>(
      "midiCcDelayAdjustFwd", "MIDI CC Delay Adjust Fwd", 0, 127, 21));
  layout.add(std::make_unique<juce::AudioParameterInt>(
      "midiCcDelayAdjustBwd", "MIDI CC Delay Adjust Bwd", 0, 127, 19));
  layout.add(std::make_unique<juce::AudioParameterInt>(
      "delayAdjustCcStep", "Delay Adjust CC Step", 1, 128, 64));

  // Visual Settings
  juce::StringArray themeNames = {"Green",  "Blue", "Purple", "Red",
                                  "Orange", "Cyan", "Pink"};
  layout.add(std::make_unique<juce::AudioParameterChoice>(
      "colorTheme", "Color Theme", themeNames, 0));

  return layout;
}

void AmenBreakChopperAudioProcessor::parameterChanged(
    const juce::String &parameterID, float newValue) {
  if (parameterID == "oscSendPort") {
    auto hostAddress =
        mValueTreeState.state.getProperty("oscHostAddress").toString();
    if (!mSender.connect(hostAddress, (int)newValue))
      juce::Logger::writeToLog(
          "AmenBreakChopper: Failed to connect OSC sender on port change.");
  } else if (parameterID == "oscReceivePort") {
    if (!mReceiver.connect((int)newValue))
      juce::Logger::writeToLog(
          "AmenBreakChopper: Failed to connect OSC receiver on port change.");
  }
}

void AmenBreakChopperAudioProcessor::setOscHostAddress(
    const juce::String &hostAddress) {
  mValueTreeState.state.setProperty("oscHostAddress", hostAddress, nullptr);
  auto sendPort =
      (int)mValueTreeState.getRawParameterValue("oscSendPort")->load();
  if (!mSender.connect(hostAddress, sendPort))
    juce::Logger::writeToLog(
        "AmenBreakChopper: Failed to connect OSC sender on host change.");
}

void AmenBreakChopperAudioProcessor::performSequenceReset() {
  mSequenceResetQueued = true;
}

void AmenBreakChopperAudioProcessor::performSoftReset() {
  mSoftResetQueued = true;
}

void AmenBreakChopperAudioProcessor::performHardReset() {
  mHardResetQueued = true;
}

juce::AudioProcessorValueTreeState &
AmenBreakChopperAudioProcessor::getValueTreeState() {
  return mValueTreeState;
}

const juce::String AmenBreakChopperAudioProcessor::getName() const {
  return JucePlugin_Name;
}

bool AmenBreakChopperAudioProcessor::acceptsMidi() const {
#if JucePlugin_WantsMidiInput
  return true;
#else
  return false;
#endif
}

bool AmenBreakChopperAudioProcessor::producesMidi() const {
#if JucePlugin_ProducesMidiOutput
  return true;
#else
  return false;
#endif
}

bool AmenBreakChopperAudioProcessor::isMidiEffect() const {
#if JucePlugin_IsMidiEffect
  return true;
#else
  return false;
#endif
}

double AmenBreakChopperAudioProcessor::getTailLengthSeconds() const {
  return 0.0;
}

int AmenBreakChopperAudioProcessor::getNumPrograms() {
  return 1; // NB: some hosts don't cope very well if you tell them there are 0
            // programs, so this should be at least 1, even if you're not really
            // implementing programs.
}

int AmenBreakChopperAudioProcessor::getCurrentProgram() { return 0; }

void AmenBreakChopperAudioProcessor::setCurrentProgram(int index) {}

const juce::String AmenBreakChopperAudioProcessor::getProgramName(int index) {
  return {};
}

void AmenBreakChopperAudioProcessor::changeProgramName(
    int index, const juce::String &newName) {}

//==============================================================================
void AmenBreakChopperAudioProcessor::prepareToPlay(double sampleRate,
                                                   int samplesPerBlock) {
  // OSC Sender
  auto hostAddress =
      mValueTreeState.state.getProperty("oscHostAddress").toString();
  auto sendPort =
      (int)mValueTreeState.getRawParameterValue("oscSendPort")->load();
  if (!mSender.connect(hostAddress, sendPort))
    juce::Logger::writeToLog("AmenBreakChopper: Failed to connect OSC sender.");

  // OSC Receiver
  auto receivePort =
      (int)mValueTreeState.getRawParameterValue("oscReceivePort")->load();
  if (!mReceiver.connect(receivePort))
    juce::Logger::writeToLog(
        "AmenBreakChopper: Failed to connect OSC receiver.");

  mSampleRate = sampleRate;

  const int numChannels = getTotalNumInputChannels();
  // Calculate buffer size for 16 eighth notes at a very slow BPM (e.g., 30 BPM)
  // to ensure the buffer is always large enough.
  // Max delay = 16 * (1/8 note) = 2 whole notes = 8 beats.
  // Duration of 8 beats at 30 BPM = 8 * (60 / 30) = 8 * 2 = 16 seconds.
  const double maxDelayTimeInSeconds = 16.0;
  const int delayBufferSize =
      static_cast<int>(maxDelayTimeInSeconds * sampleRate);

  mDelayBuffer.setSize(numChannels, delayBufferSize);
  mDelayBuffer.clear();

  // Initialize sequencer state
  mNextEighthNotePpq = 0.0;
  mSequencePosition = 0;
  mNoteSequencePosition = 0;
  mLastReceivedNoteValue = 0;
  mSequenceResetQueued = false;
  mHardResetQueued = false;
  mNewNoteReceived = false;
  mSoftResetQueued = false;

  mLastSeqResetCcValue = 0;
  mLastHardResetCcValue = 0;
  mLastSoftResetCcValue = 0;
  mLastDelayAdjustFwdCcValue = 0;
  mLastDelayAdjustBwdCcValue = 0;
  mLastDelayAdjust = 0;
}

void AmenBreakChopperAudioProcessor::releaseResources() {
  mDelayBuffer.setSize(0, 0);
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool AmenBreakChopperAudioProcessor::isBusesLayoutSupported(
    const BusesLayout &layouts) const {
#if JucePlugin_IsMidiEffect
  juce::ignoreUnused(layouts);
  return true;
#else
  // This is the place where you check if the layout is supported.
  // In this template code we only support mono or stereo.
  // Some plugin hosts, such as certain GarageBand versions, will only
  // load plugins that support stereo bus layouts.
  if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono() &&
      layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
    return false;

  // This checks if the input layout matches the output layout
#if !JucePlugin_IsSynth
  if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
    return false;
#endif

  return true;
#endif
}
#endif

bool AmenBreakChopperAudioProcessor::shouldTriggerReset(int mode,
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

void AmenBreakChopperAudioProcessor::processBlock(
    juce::AudioBuffer<float> &buffer, juce::MidiBuffer &midiMessages) {
  juce::ScopedNoDenormals noDenormals;
  auto totalNumInputChannels = getTotalNumInputChannels();
  auto totalNumOutputChannels = getTotalNumOutputChannels();

  // --- Clear any unused output channels ---
  for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
    buffer.clear(i, 0, buffer.getNumSamples());

  // --- Process incoming MIDI messages ---
  const int midiInChannel =
      (int)mValueTreeState.getRawParameterValue("midiInputChannel")->load();
  const int midiOutChannel =
      (int)mValueTreeState.getRawParameterValue("midiOutputChannel")->load();

  // Check for UI-triggered note invocation
  int uiNote = mUiTriggeredNote.exchange(-1);
  if (uiNote >= 0 && uiNote <= 15) {
    mLastReceivedNoteValue = uiNote;
    mNoteSequencePosition = uiNote;
    mNewNoteReceived = true;
    if (onNoteEvent)
      onNoteEvent(uiNote, -1);
  }

  juce::MidiBuffer processedMidi; // Create a new buffer for our generated notes
  for (const auto metadata : midiMessages) {
    auto message = metadata.getMessage();
    // Omni mode: if midiInChannel is 0, accept all channels.
    if (midiInChannel == 0 || message.getChannel() == midiInChannel) {
      if (message.isNoteOn()) {
        int noteNumber = message.getNoteNumber();
        if (noteNumber >= 0 && noteNumber <= 15) {
          mLastReceivedNoteValue = noteNumber;
          mNoteSequencePosition =
              noteNumber; // MIDI note overrides the note sequence
          mNewNoteReceived = true;

          if (onNoteEvent)
            onNoteEvent(noteNumber, -1); // -1 indicates Input/Trigger
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
            mSequenceResetQueued = true;
          mLastSeqResetCcValue = controllerValue;
        }

        if (controllerNumber == ccHardReset) {
          const int mode =
              (int)mValueTreeState.getRawParameterValue("midiCcHardResetMode")
                  ->load();
          if (shouldTriggerReset(mode, mLastHardResetCcValue, controllerValue))
            mHardResetQueued = true;
          mLastHardResetCcValue = controllerValue;
        }

        if (controllerNumber == ccSoftReset) {
          const int mode =
              (int)mValueTreeState.getRawParameterValue("midiCcSoftResetMode")
                  ->load();
          if (shouldTriggerReset(mode, mLastSoftResetCcValue, controllerValue))
            mSoftResetQueued = true;
          mLastSoftResetCcValue = controllerValue;
        }

        const int ccFwd =
            (int)mValueTreeState.getRawParameterValue("midiCcDelayAdjustFwd")
                ->load();
        const int ccBwd =
            (int)mValueTreeState.getRawParameterValue("midiCcDelayAdjustBwd")
                ->load();

        // Detect press events (rising edge) for the current message
        bool fwdJustPressed =
            (controllerNumber == ccFwd && controllerValue >= 65 &&
             mLastDelayAdjustFwdCcValue < 65);
        bool bwdJustPressed =
            (controllerNumber == ccBwd && controllerValue >= 65 &&
             mLastDelayAdjustBwdCcValue < 65);

        // Determine the "held" state of the *other* button (from before this
        // message)
        bool bwdWasHeld = (mLastDelayAdjustBwdCcValue >= 65);
        bool fwdWasHeld = (mLastDelayAdjustFwdCcValue >= 65);

        // Check for reset condition: one button was just pressed while the
        // other was already held.
        if ((fwdJustPressed && bwdWasHeld) || (bwdJustPressed && fwdWasHeld)) {
          auto *param = static_cast<juce::AudioParameterInt *>(
              mValueTreeState.getParameter("delayAdjust"));
          param->operator=(0);
        }
        // If no reset, handle single press actions.
        else if (fwdJustPressed) {
          auto *stepParam = static_cast<juce::AudioParameterInt *>(
              mValueTreeState.getParameter("delayAdjustCcStep"));
          auto *param = static_cast<juce::AudioParameterInt *>(
              mValueTreeState.getParameter("delayAdjust"));
          param->operator=(param->get() + stepParam->get());
        } else if (bwdJustPressed) {
          auto *stepParam = static_cast<juce::AudioParameterInt *>(
              mValueTreeState.getParameter("delayAdjustCcStep"));
          auto *param = static_cast<juce::AudioParameterInt *>(
              mValueTreeState.getParameter("delayAdjust"));
          param->operator=(param->get() - stepParam->get());
        }

        // Finally, update the 'last value' state keepers for the next
        // message/block.
        if (controllerNumber == ccFwd) {
          mLastDelayAdjustFwdCcValue = controllerValue;
        }
        if (controllerNumber == ccBwd) {
          mLastDelayAdjustBwdCcValue = controllerValue;
        }
      }
    }
  }
  midiMessages.clear(); // Clear the incoming buffer

  // --- Get transport state from host ---
  juce::AudioPlayHead *playHead = getPlayHead();
  juce::AudioPlayHead::PositionInfo positionInfo;
  if (playHead != nullptr)
    positionInfo = playHead->getPosition().orFallback(positionInfo);

  // On reset, turn off any hanging notes first
  if (mHardResetQueued || mSoftResetQueued) {
    if (mLastNote1 >= 0)
      processedMidi.addEvent(
          juce::MidiMessage::noteOff(midiOutChannel, mLastNote1), 0);
    if (mLastNote2 >= 0)
      processedMidi.addEvent(
          juce::MidiMessage::noteOff(midiOutChannel, mLastNote2), 0);
    mLastNote1 = -1;
    mLastNote2 = -1;
  }

  if (mHardResetQueued) {
    mSequencePosition = 0;
    mNoteSequencePosition = 0;
    mValueTreeState.getParameter("sequencePosition")
        ->setValueNotifyingHost(0.0f);
    mValueTreeState.getParameter("noteSequencePosition")
        ->setValueNotifyingHost(0.0f);

    if (positionInfo.getIsPlaying()) {
      const double ppqAtStartOfBlock =
          positionInfo.getPpqPosition().orFallback(0.0);
      const double bpm = positionInfo.getBpm().orFallback(120.0);
      const double ppqPerSample = bpm / (60.0 * getSampleRate());

      // Also reset PPQ tracking to the current tick
      mNextEighthNotePpq = std::ceil(ppqAtStartOfBlock * 2.0) / 2.0;

      // Apply the current delayAdjust as a phase offset on reset
      const int currentDelayAdjust =
          static_cast<juce::AudioParameterInt *>(
              mValueTreeState.getParameter("delayAdjust"))
              ->get();
      mNextEighthNotePpq += currentDelayAdjust * ppqPerSample;
      mLastDelayAdjust = currentDelayAdjust;
    }
    mHardResetQueued = false;
  }



  // --- Get musical time information ---
  const double sampleRate = getSampleRate();
  const double bpm = positionInfo.getBpm().orFallback(120.0);
  const double ppqAtStartOfBlock =
      positionInfo.getPpqPosition().orFallback(0.0);
  const double ppqPerSample = bpm / (60.0 * sampleRate);

  // --- Handle transport jumps or looping ---
  // If transport loops or jumps backwards, reset the next note position to
  // align with the grid.
  if (positionInfo.getIsLooping() ||
      (ppqAtStartOfBlock < mNextEighthNotePpq - 0.5)) {
    // Find the first 8th note position at or after the start of this block.
    mNextEighthNotePpq = std::ceil(ppqAtStartOfBlock * 2.0) / 2.0;
  }

  // Update BPM for UI
  mCurrentBpm.store(bpm);

  // Check if we crossed a beat boundary (integer part of mNextEighthNotePpq changed?)
  // Actually, we can just check if mSequencePosition changes in the loop below.

  // --- Sequencer Tick Logic (Block-based) ---
  const int bufferLength = buffer.getNumSamples();
  double ppqAtEndOfBlock = ppqAtStartOfBlock + (bufferLength * ppqPerSample);

  if (positionInfo.getIsPlaying()) {

  // --- Apply delayAdjust to sequencer phase ---
  auto *delayAdjustParam = static_cast<juce::AudioParameterInt *>(
      mValueTreeState.getParameter("delayAdjust"));
  const int currentDelayAdjust = delayAdjustParam->get();
  const int deltaDelayAdjust = currentDelayAdjust - mLastDelayAdjust;

  if (deltaDelayAdjust != 0) {
    const double deltaPpq = deltaDelayAdjust * ppqPerSample;
    mNextEighthNotePpq += deltaPpq;
    mWaveformDirty = true; // Delay adjust changed, waveforms shifted
  }
  mLastDelayAdjust = currentDelayAdjust;

  while (mNextEighthNotePpq < ppqAtEndOfBlock) {
    const int tickSample = static_cast<int>(
        (mNextEighthNotePpq - ppqAtStartOfBlock) / ppqPerSample);

    if (mSequenceResetQueued) {
      mNoteSequencePosition = mSequencePosition; // Sync Note-Seq to Main-Seq
      mValueTreeState.getParameter("delayTime")
          ->setValueNotifyingHost(0.0f); // Reset DelayTime
      mNewNoteReceived = false;
      mSequenceResetQueued = false;
    }

    if (mSoftResetQueued) {
      mSequencePosition = 0;
      mNoteSequencePosition = 0;
      mSoftResetQueued = false;
    }

    if (mNewNoteReceived) {
      const int diff = mSequencePosition - mLastReceivedNoteValue;
      const int newDelayTime = (diff % 16 + 16) % 16;
      mValueTreeState.getParameter("delayTime")
          ->setValueNotifyingHost(static_cast<float>(newDelayTime) / 15.0f);
    }

    mValueTreeState.getParameter("sequencePosition")
        ->setValueNotifyingHost(static_cast<float>(mSequencePosition) / 15.0f);
    mValueTreeState.getParameter("noteSequencePosition")
        ->setValueNotifyingHost(static_cast<float>(mNoteSequencePosition) /
                                15.0f);

    mSender.send(juce::OSCMessage("/sequencePosition", mSequencePosition));
    mSender.send(
        juce::OSCMessage("/noteSequencePosition", mNoteSequencePosition));

    const int note1 = mNoteSequencePosition;
    const int note2 = 32 + mSequencePosition;
    const juce::uint8 velocity = 100;

    // Send Note Off for the previous note if it's valid
    if (mLastNote1 >= 0)
      processedMidi.addEvent(
          juce::MidiMessage::noteOff(midiOutChannel, mLastNote1), tickSample);
    if (mLastNote2 >= 0)
      processedMidi.addEvent(
          juce::MidiMessage::noteOff(midiOutChannel, mLastNote2), tickSample);

    // Send Note On for the current note
    processedMidi.addEvent(
        juce::MidiMessage::noteOn(midiOutChannel, note1, velocity), tickSample);
    processedMidi.addEvent(
        juce::MidiMessage::noteOn(midiOutChannel, note2, velocity), tickSample);

    // Store the current note as the last one for the next tick
    mLastNote1 = note1;
    mLastNote2 = note2;

    if (onNoteEvent)
      onNoteEvent(note1, note2); // note1 = 0-15 (Seq), note2 = 32-47 (Original)

    mNewNoteReceived = false;

    // Sequence advanced, trigger visual update
    if (mSequencePosition != (mSequencePosition + 1) % 16) {
        mWaveformDirty = true; 
    }

    mSequencePosition = (mSequencePosition + 1) % 16;
    mNoteSequencePosition = (mNoteSequencePosition + 1) % 16;

    mNextEighthNotePpq += 0.5; // Advance to the next 8th note position
  }
} else {
    // If not playing, ensure we still flag dirty so visualization updates (scrolling input)
    mWaveformDirty = true;
}

  midiMessages.swapWith(
      processedMidi); // Place our generated notes into the main buffer

  // --- Audio Processing Logic (Sample-by-sample) ---
  const int delayBufferLength = mDelayBuffer.getNumSamples();
  auto *delayTimeParam = mValueTreeState.getRawParameterValue("delayTime");
  const int currentDelayTime = static_cast<int>(delayTimeParam->load());

  for (int sample = 0; sample < bufferLength; ++sample) {
    // Write input to the delay buffer for this sample
    for (int channel = 0; channel < totalNumInputChannels; ++channel) {
      const float *bufferData = buffer.getReadPointer(channel);
      float *delayBufferData = mDelayBuffer.getWritePointer(channel);
      delayBufferData[(mWritePosition + sample) % delayBufferLength] =
          bufferData[sample];
    }

    // If DelayTime is 0, bypass the effect (output is same as input)
    if (currentDelayTime != 0 && positionInfo.getIsPlaying()) {
      double eighthNoteTime = (60.0 / bpm) / 2.0;
      int delayTimeInSamples =
          static_cast<int>(eighthNoteTime * currentDelayTime * sampleRate);

      for (int channel = 0; channel < totalNumInputChannels; ++channel) {
        float *delayBufferData = mDelayBuffer.getWritePointer(channel);
        auto *channelData = buffer.getWritePointer(channel);
        const int readPosition =
            (mWritePosition - delayTimeInSamples + sample + delayBufferLength) %
            delayBufferLength;
        const float delayedSample = delayBufferData[readPosition];
        channelData[sample] = delayedSample; // 100% Wet
      }
    }
  }

  mWritePosition = (mWritePosition + bufferLength) % delayBufferLength;
  
  if (positionInfo.getIsPlaying()) {
      // Update samples to next beat for visualization AFTER sequencer update
      // We use the PPQ at the end of the block since mWritePosition is now there.
      double ppqDist = mNextEighthNotePpq - ppqAtEndOfBlock;
      if (ppqPerSample > 0.0) {
          double samples = ppqDist / ppqPerSample;
          if (samples < 0) samples = 0; // Safety
          mSamplesToNextBeat.store(samples);
      } else {
          mSamplesToNextBeat.store(0.0);
      }
  } else {
      mSamplesToNextBeat.store(0.0);
  }
}

//==============================================================================
bool AmenBreakChopperAudioProcessor::hasEditor() const {
  return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor *AmenBreakChopperAudioProcessor::createEditor() {
  return new AmenBreakChopperAudioProcessorEditor(*this);
}

//==============================================================================
void AmenBreakChopperAudioProcessor::oscMessageReceived(
    const juce::OSCMessage &message) {
  if (message.getAddressPattern() == "/delayTime") {
    if (message.size() > 0 && message[0].isInt32()) {
      int newDelayTime = message[0].getInt32();
      if (newDelayTime >= 0 && newDelayTime <= 15) {
        auto *delayTimeParam = mValueTreeState.getParameter("delayTime");
        if (delayTimeParam != nullptr)
          delayTimeParam->setValueNotifyingHost(
              static_cast<float>(newDelayTime) / 15.0f);
      }
    }
  } else if (message.getAddressPattern() == "/sequenceReset") {
    mSequenceResetQueued = true;
  } else if (message.getAddressPattern() == "/hardReset") {
    mHardResetQueued = true;
  } else if (message.getAddressPattern() == "/softReset") {
    mSoftResetQueued = true;

  } else if (message.getAddressPattern() == "/setNoteSequencePosition") {
    if (message.size() > 0 && message[0].isInt32()) {
      int noteNumber = message[0].getInt32();
      if (noteNumber >= 0 && noteNumber <= 15) {
        mLastReceivedNoteValue = noteNumber;
        mNoteSequencePosition =
            noteNumber; // OSC note overrides the note sequence
        mNewNoteReceived = true;
      }
    }
  }
}

//==============================================================================
void AmenBreakChopperAudioProcessor::getStateInformation(
    juce::MemoryBlock &destData) {
  auto state = mValueTreeState.copyState();
  // These parameters should not be saved with the project.
  state.removeProperty("delayTime", nullptr);
  state.removeProperty("sequencePosition", nullptr);
  state.removeProperty("noteSequencePosition", nullptr);
  std::unique_ptr<juce::XmlElement> xml(state.createXml());
  copyXmlToBinary(*xml, destData);
}

void AmenBreakChopperAudioProcessor::setStateInformation(const void *data,
                                                         int sizeInBytes) {
  std::unique_ptr<juce::XmlElement> xmlState(
      getXmlFromBinary(data, sizeInBytes));

  if (xmlState.get() != nullptr)
    if (xmlState->hasTagName(mValueTreeState.state.getType()))
      mValueTreeState.replaceState(juce::ValueTree::fromXml(*xmlState));

  // Always reset these parameters to 0 on load.
  if (auto *p = mValueTreeState.getParameter("delayTime"))
    p->setValueNotifyingHost(p->getDefaultValue());
  if (auto *p = mValueTreeState.getParameter("sequencePosition"))
    p->setValueNotifyingHost(p->getDefaultValue());
  if (auto *p = mValueTreeState.getParameter("noteSequencePosition"))
    p->setValueNotifyingHost(p->getDefaultValue());
}

//==============================================================================

void AmenBreakChopperAudioProcessor::triggerNoteFromUi(int noteNumber) {
  mUiTriggeredNote = noteNumber;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter() {
  return new AmenBreakChopperAudioProcessor();
}
