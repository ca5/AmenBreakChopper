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

  // --- Defaults for Standalone ---
  if (juce::JUCEApplicationBase::isStandaloneApp()) {
      // Default Input to OFF for Standalone to accept "silence" policy
      if (auto* p = mValueTreeState.getParameter("inputEnabled"))
          p->setValueNotifyingHost(0.0f);
  }
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

  if (bufferSize == 0) return std::vector<float>(16 * 32, 0.0f);

  int currentWritePos = mUiWritePosition.load(); // Use synced UI write position
  int currentSeqPos = mUiSequencePosition.load();
  int waveformOffset = mWaveformOffset.load(); // Fixed offset set at reset
  double samplesToNextBeat = mSamplesToNextBeat.load();
  
  const auto* channelData = mDelayBuffer.getReadPointer(0); // Use Left channel for visualization

      // Check if we should visualize the Static Sample or the Live Delay Buffer
      bool isInputEnabled = *mValueTreeState.getRawParameterValue("inputEnabled") > 0.5f;
      
      if (!isInputEnabled && mIsSampleLoaded) {
          // --- Static Sample Visualization ---
          int idx = mActiveBufferIndex.load();
          if (mSampleBuffers[idx].getNumSamples() > 0) {
              const float* sampleData = mSampleBuffers[idx].getReadPointer(0);
              int sampleLength = mSampleBuffers[idx].getNumSamples();
              
              // Divide entire sample into 16 slices
              double samplesPerSlice = (double)sampleLength / 16.0;
              int increment = static_cast<int>(samplesPerSlice / 32.0);
              if (increment < 1) increment = 1;
              
              // Use fixed waveform offset (set at reset, doesn't change with sequencer)
              for (int visualPos = 0; visualPos < 16; ++visualPos) {
                  // Calculate which sample slice to show at this visual position
                  int sampleSlice = (visualPos + waveformOffset) % 16;
                  int startSample = static_cast<int>(sampleSlice * samplesPerSlice);
                  
                  for (int k = 0; k < 32; ++k) {
                       int pos = startSample + k * increment;
                       if (pos < sampleLength) {
                           waveformData.push_back(sampleData[pos]);
                       } else {
                           waveformData.push_back(0.0f);
                       }
                  }
              }
              return waveformData;
          }
      }

      // --- Live Delay Buffer Visualization (Fallback) ---
      for (int stepIndex = 0; stepIndex < 16; ++stepIndex) {
          // currentSeqPos points to the NEXT step to be played
          // The currently playing (recording) step is (currentSeqPos - 1)
          // Calculate 'i': How far is stepIndex from the currently playing step?
          
          int currentlyPlayingStep = (currentSeqPos - 1 + 16) % 16;
          int i = (stepIndex - currentlyPlayingStep + 16) % 16;
          
          if (i == 0) {
              // Current Step (Active/Recording) - no waveform yet (still recording)
              for (int k = 0; k < 32; ++k) {
                  waveformData.push_back(0.0f);
              }
          } else {
              // Past Step - show completed waveform
              double timeElapsedInCurrent = eighthNoteSamples - samplesToNextBeat;
              double startSampleRelToNow = -(timeElapsedInCurrent + (16.0 - i) * eighthNoteSamples);
              
              int readPos = currentWritePos + static_cast<int>(startSampleRelToNow);
              
              int increment = static_cast<int>(eighthNoteSamples / 32.0);
              if (increment == 0) increment = 1;
              int delayBufferLength = mDelayBuffer.getNumSamples();
              
              for (int k = 0; k < 32; ++k) {
                  int pos = readPos + k * increment;
                  while (pos < 0) pos += delayBufferLength;
                  while (pos >= delayBufferLength) pos -= delayBufferLength;
                  
                  if (pos < delayBufferLength) {
                     waveformData.push_back(channelData[pos]);
                  } else {
                     waveformData.push_back(0.0f);
                  }
              }
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

  // Standalone / Sync Settings
  juce::StringArray bpmModes = {"Host", "MIDI Clock", "Manual"};
  layout.add(std::make_unique<juce::AudioParameterChoice>(
      "bpmSyncMode", "BPM Sync Mode", bpmModes, 0));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      "internalBpm", "Internal BPM", 40.0f, 300.0f, 120.0f));
  layout.add(std::make_unique<juce::AudioParameterBool>(
      "inputEnabled", "Input Enabled", true));
  layout.add(std::make_unique<juce::AudioParameterInt>(
      "inputChanL", "Input Channel L", 1, 8, 1));
  layout.add(std::make_unique<juce::AudioParameterInt>(
      "inputChanR", "Input Channel R", 1, 8, 2));

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
      "delayAdjust", "Delay Adjust", -1000, 1000, 0));
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
  mReceiver.connect(receivePort);
    juce::Logger::writeToLog(
        "AmenBreakChopper: Failed to connect OSC receiver.");

  mMidiClockTracker.reset();
  mSampleRate = sampleRate;

  // We enforce a Stereo internal buffer for the delay/looping logic.
  // Input routing will map selected inputs to this stereo pair.
  const int delayBufferSize =
      static_cast<int>(16.0 * sampleRate); // 16 seconds max delay

  mDelayBuffer.setSize(2, delayBufferSize); // Fixed 2 channels (Stereo)
  mDelayBuffer.clear();

  // Initialize checks
  if (!mIsInitialized) {
      // Initialize sequencer state
      mNextEighthNotePpq = 0.0;
      mSequencePosition = 0;
      mUiSequencePosition.store(0);
      mUiWritePosition.store(0);
      mNoteSequencePosition = 0;
      mLastReceivedNoteValue = 0;
      mSequenceResetQueued = false;
      mHardResetQueued = false;
      mNewNoteReceived = false;
      mLastDelayAdjustFwdCcValue = 0;
      mLastDelayAdjustBwdCcValue = 0;
      mLastDelayAdjust = 0;

      mIsSampleLoaded = false;
      mSampleReadPos = 0.0;
      mSampleBufferRates[0] = 44100.0;
      mSampleBufferRates[1] = 44100.0;
      mSampleBuffers[0].setSize(0, 0); // Clear logic
      mSampleBuffers[1].setSize(0, 0); 
      
      // Default State based on Wrapper Type
      juce::PluginHostType hostType;
      if (hostType.getPluginLoadedAs() == juce::AudioProcessor::wrapperType_Standalone) {
          loadBuiltInSample("amen140.wav");
          // Force immediate switch for startup
          if (mPendingSampleSwitch.load()) {
             mActiveBufferIndex.store(1 - mActiveBufferIndex.load());
             mIsSampleLoaded = true;
             mPendingSampleSwitch = false; 
             
             // Apply Pending Params immediately
             float pendingBpm = mPendingBpm.load();
             mCurrentBpm.store(pendingBpm);
             
             if (auto* p = mValueTreeState.getParameter("internalBpm")) {
                 if (auto* fp = dynamic_cast<juce::AudioParameterFloat*>(p)) {
                     fp->setValueNotifyingHost(fp->convertTo0to1(pendingBpm));
                 }
             }
             if (auto* p = mValueTreeState.getParameter("bpmSyncMode")) {
                 p->setValueNotifyingHost(1.0f); // Manual
             }
             if (auto* p = mValueTreeState.getParameter("inputEnabled"))
                 p->setValueNotifyingHost(0.0f); // Disable Input
                 
             mWaveformDirty = true;
          }
      }
      
      mIsInitialized = true;
  }
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
  // Support Mono/Stereo Output
  if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono() &&
      layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
    return false;

  // Allow any Input configuration as long as it has at least as many channels as output (or more).
  // Actually, we want to support e.g. 8 Inputs -> 2 Outputs.
  if (layouts.getMainInputChannelSet().size() < layouts.getMainOutputChannelSet().size())
      return false;

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

  // --- Clear unused output channels ---
  for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
    buffer.clear(i, 0, buffer.getNumSamples());

  // --- Parameters ---
  auto *bpmModeParam = mValueTreeState.getRawParameterValue("bpmSyncMode");
  bool useMidiClock = (bpmModeParam->load() >= 0.5f);

  auto *inputEnabledParam = mValueTreeState.getRawParameterValue("inputEnabled");
  bool inputEnabled = (inputEnabledParam->load() > 0.5f);

  // FIX: If input is disabled, strictly clear the main buffer.
  // This prevents the "dry" signal from passing through when delayTime=0 or during processing gaps.
  // We do this BEFORE any processing so the delay buffer also records silence (effectively).
  if (!inputEnabled && !mIsSampleLoaded) {
      buffer.clear();
  }

  int inputChanL = (int)mValueTreeState.getRawParameterValue("inputChanL")->load() - 1;
  int inputChanR = (int)mValueTreeState.getRawParameterValue("inputChanR")->load() - 1;

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
    
    // --- MIDI Clock Handling ---
    if (message.isMidiClock()) {
         mMidiClockTracker.processClockMessage(juce::Time::getMillisecondCounterHiRes() * 0.001);
    } else if (message.isMidiStart()) {
         mSequencePosition = 0;
         mNoteSequencePosition = 0;
         mMidiClockPpq = 0.0;
         mNextEighthNotePpq = 0.0;
    } else if (message.isMidiStop()) {
         // Optionally handle stop
    }

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

  // --- Get transport state from host OR MIDI Clock ---
  juce::AudioPlayHead *playHead = getPlayHead();
  juce::AudioPlayHead::PositionInfo positionInfo;
  if (playHead != nullptr)
    positionInfo = playHead->getPosition().orFallback(positionInfo);

  double bpm = 120.0;
  double ppqAtStartOfBlock = 0.0;
  bool isPlaying = true; // Default to running for internal/standalone

  // AudioParameterChoice with 3 options: 0.0, 0.5, 1.0
  float bpmModeVal = bpmModeParam->load();
  int bpmMode = 0;
  if (bpmModeVal > 0.75f) bpmMode = 2; // Manual
  else if (bpmModeVal > 0.25f) bpmMode = 1; // MIDI Clock
  else bpmMode = 0; // Host

  if (bpmMode == 1) { // MIDI Clock
      bpm = mMidiClockTracker.detectedBpm;
      ppqAtStartOfBlock = mMidiClockPpq;
      // We assume playing if using MIDI clock logic (or check clock active?)
      isPlaying = true; 
  } else if (bpmMode == 2) { // Manual
      if (auto* p = mValueTreeState.getRawParameterValue("internalBpm")) {
          bpm = (double)p->load();
      }
      // Emulate PPQ for Manual Mode
      ppqAtStartOfBlock = mMidiClockPpq;
      isPlaying = true;
  } else { // Host (0)
      bpm = positionInfo.getBpm().orFallback(120.0);
      ppqAtStartOfBlock = positionInfo.getPpqPosition().orFallback(0.0);
      isPlaying = positionInfo.getIsPlaying();
  }
  
  // Reset logic updates
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

    if (isPlaying) {
      // Also reset PPQ tracking to the current tick
      mNextEighthNotePpq = std::ceil(ppqAtStartOfBlock * 2.0) / 2.0;

      // Apply the current delayAdjust as a phase offset on reset
      const int currentDelayAdjust =
          static_cast<juce::AudioParameterInt *>(
              mValueTreeState.getParameter("delayAdjust"))
              ->get();

      
      // Conversion from MS to PPQ
      // 1 Beat = 60000 / BPM ms
      // 1 PPQ = 1 Beat
      // ms to PPQ = ms / (60000 / BPM)
      
      const double msPerBeat = 60000.0 / bpm;
      const double adjustInPpq = (double)currentDelayAdjust / msPerBeat;

      mNextEighthNotePpq += adjustInPpq;
      mLastDelayAdjust = currentDelayAdjust;
    }
    mHardResetQueued = false;
  }

  // --- Get musical time information (Effective) ---
  const double sampleRate = getSampleRate();
  const double ppqPerSample = bpm / (60.0 * sampleRate);

  // --- Handle transport jumps or looping ---
  if (isPlaying && !useMidiClock) {
      if (positionInfo.getIsLooping() ||
          (ppqAtStartOfBlock < mNextEighthNotePpq - 0.5)) {
        mNextEighthNotePpq = std::ceil(ppqAtStartOfBlock * 2.0) / 2.0;
      }
  }

  // Update BPM for UI
  mCurrentBpm.store(bpm);
  mUsingMidiClock.store(useMidiClock); // For UI

  // --- Sequencer Tick Logic (Block-based) ---
  const int bufferLength = buffer.getNumSamples();
  double ppqAtEndOfBlock = ppqAtStartOfBlock + (bufferLength * ppqPerSample);
  
  // Advance MIDI Clock PPQ for next block
  // Advance MIDI Clock PPQ for next block
  if (bpmMode >= 1) { // MIDI Clock or Manual
      mMidiClockPpq = ppqAtEndOfBlock;
  }

  if (isPlaying) {
    // ... Logic continues below (reused) ...
    // Note: We need to ensure the closing braces match the original structure.


  // --- Apply delayAdjust to sequencer phase ---
  auto *delayAdjustParam = static_cast<juce::AudioParameterInt *>(
      mValueTreeState.getParameter("delayAdjust"));
  const int currentDelayAdjust = delayAdjustParam->get();
  const int deltaDelayAdjust = currentDelayAdjust - mLastDelayAdjust;

  if (deltaDelayAdjust != 0) {
    // Convert ms delta to PPQ delta
    // deltaMs / (60000 / BPM) = deltaPpq
    const double msPerBeat = 60000.0 / bpm;
    // Avoid division by zero
    double safeMsPerBeat = (msPerBeat < 1.0) ? 1.0 : msPerBeat; 

    const double deltaPpq = (double)deltaDelayAdjust / safeMsPerBeat;
    
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
      // Capture current position before reset for waveform rotation
      mWaveformOffset.store(mSequencePosition.load());
      
      mSequencePosition = 0;
      mUiSequencePosition.store(0); // Immediately update UI position
      mNoteSequencePosition = 0;
      mSoftResetQueued = false;
    }

    // --- Quantized Sample Switch Logic ---
    // If a sample load is pending, switch on the beat (every 2 steps / quarter note)
    if (mPendingSampleSwitch.load()) {
        // Switch on beat (even steps: 0, 2, 4...)
        if (mSequencePosition % 2 == 0) {
            // 1. Swap Buffer
            int pendingIndex = 1 - mActiveBufferIndex.load();
            mActiveBufferIndex.store(pendingIndex);
            
            // 2. Update BPM
            float newBpm = mPendingBpm.load();
            mCurrentBpm.store(newBpm); 
            
            // Update Parameters (careful in audio thread, but needed for sync)
            if (auto* p = mValueTreeState.getParameter("internalBpm")) {
               if (auto* fp = dynamic_cast<juce::AudioParameterFloat*>(p)) {
                   fp->setValueNotifyingHost(fp->convertTo0to1(newBpm));
               }
            }
             // Set Manual Mode
             if (auto* p = mValueTreeState.getParameter("bpmSyncMode")) {
                 p->setValueNotifyingHost(1.0f); // Manual
             }
             // Disable Input
            if (auto* p = mValueTreeState.getParameter("inputEnabled"))
                 p->setValueNotifyingHost(0.0f);

            // 3. Reset State
            mIsSampleLoaded = true;
            mSampleReadPos = 0.0;
            
            // Keep existing waveform offset (don't reset to 0)
            // This preserves any rotation from previous reset
            // mWaveformOffset.store(0);  // REMOVED - keep current offset
            
            // Don't reset mSequencePosition - let it continue
            // mSequencePosition = 0;  // REMOVED
            // mUiSequencePosition.store(0);  // REMOVED
            // mNoteSequencePosition = 0;  // REMOVED
            mPendingSampleSwitch = false; 
            
            // 4. Force Redraw
            mWaveformDirty = true; 
        }
    }

    if (mNewNoteReceived) {
      const int diff = mSequencePosition - mLastReceivedNoteValue;
      const int newDelayTime = (diff % 16 + 16) % 16;
      mValueTreeState.getParameter("delayTime")
          ->setValueNotifyingHost(static_cast<float>(newDelayTime) / 15.0f);
    }

    mValueTreeState.getParameter("sequencePosition")
        ->setValueNotifyingHost(static_cast<float>(mSequencePosition.load()) / 15.0f);
    mValueTreeState.getParameter("noteSequencePosition")
        ->setValueNotifyingHost(static_cast<float>(mNoteSequencePosition) /
                                15.0f);

    mSender.send(juce::OSCMessage("/sequencePosition", mSequencePosition.load()));
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

    // Advance sequence
    mSequencePosition = (mSequencePosition + 1) % 16;
    mNoteSequencePosition = (mNoteSequencePosition + 1) % 16;

    // Sequence advanced, trigger visual update
    mWaveformDirty = true; 

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
    if (inputEnabled) {
      // Input L
      if (inputChanL < totalNumInputChannels) {
         float inputVal = buffer.getReadPointer(inputChanL)[sample];
         mDelayBuffer.getWritePointer(0)[(mWritePosition + sample) % delayBufferLength] = inputVal;
      }
      // Input R
      if (inputChanR < totalNumInputChannels) {
         float inputVal = buffer.getReadPointer(inputChanR)[sample];
         mDelayBuffer.getWritePointer(1)[(mWritePosition + sample) % delayBufferLength] = inputVal;
      }
    } else if (mIsSampleLoaded) {
        // Playback Built-in Sample
        // Use Active Buffer
        int idx = mActiveBufferIndex.load();
        if (mSampleBuffers[idx].getNumSamples() > 0) {
            // Phase-Locked Playback with Offset (Chopping)
            // Calculate read position based on current PPQ to ensure perfect sync with sequencer
            // preventing drift between audio loop and sequencer grid.
            
            double curPpq = ppqAtStartOfBlock + sample * ppqPerSample;
            // Force loop to match sequencer grid (16 steps = 8.0 PPQ)
            // This time-stretches the sample to fit the grid, ensuring sync
            double loopLengthPpq = 16.0 * 0.5; // 16 steps * 0.5 PPQ/step = 8.0 PPQ
            
            // Apply Delay Offset directly to Phase (Chopping)
            // currentDelayTime is in Steps (Eighth Notes). 1 Step = 0.5 PPQ.
            // We want to read from "Past", so Subtract offset.
            double offsetPpq = currentDelayTime * 0.5;
            double targetPpq = curPpq - offsetPpq;
            
            // Wrap PPQ within loop
            double wrappedPpq = fmod(targetPpq, loopLengthPpq);
            // fmod returns negative if input is negative. Handle wrapping correctly.
            if (wrappedPpq < 0) wrappedPpq += loopLengthPpq;
            
            double progress = wrappedPpq / loopLengthPpq;
            int sampleLen = mSampleBuffers[idx].getNumSamples();
            
            // Map progress 0..1 to sample indices 0..Length
            // Use simple Linear Interpolation or just integer index
            double idealPos = progress * sampleLen;
            mSampleReadPos = idealPos; // Update member for reference (though value is derived per sample)

            // Interpolated read
            int posInt = (int)idealPos;
            int posNext = (posInt + 1) % sampleLen;
            float frac = (float)(idealPos - posInt);
            
            float l = mSampleBuffers[idx].getSample(0, posInt) * (1.0f - frac) + 
                      mSampleBuffers[idx].getSample(0, posNext) * frac;
                      
            float r = (mSampleBuffers[idx].getNumChannels() > 1) ? 
                      (mSampleBuffers[idx].getSample(1, posInt) * (1.0f - frac) + 
                       mSampleBuffers[idx].getSample(1, posNext) * frac) : l;
            
            mDelayBuffer.getWritePointer(0)[(mWritePosition + sample) % delayBufferLength] = l;
            mDelayBuffer.getWritePointer(1)[(mWritePosition + sample) % delayBufferLength] = r;

            // No need to manually increment mSampleReadPos or check bounds as it's derived from PPQ
        } else {
             mDelayBuffer.getWritePointer(0)[(mWritePosition + sample) % delayBufferLength] = 0.0f;
             mDelayBuffer.getWritePointer(1)[(mWritePosition + sample) % delayBufferLength] = 0.0f;
        }

    } else {
        // Silence input to delay buffer if disabled
        mDelayBuffer.getWritePointer(0)[(mWritePosition + sample) % delayBufferLength] = 0.0f;
        mDelayBuffer.getWritePointer(1)[(mWritePosition + sample) % delayBufferLength] = 0.0f;
    }

    // If DelayTime is 0, bypass the effect (output is same as input)
    // FIX: If sample is loaded, we ALWAYS want to write to output (to overwrite input buffer), even if delay time is 0.
    // MODIFIED: If sample is loaded, we apply the "Delay" as an offset to the Sample Read Phase,
    // so we act as if the Delay Effect is 0 (Direct Output of chopped sample).
    int effectiveDelayTime = currentDelayTime;
    if (mIsSampleLoaded && !inputEnabled) effectiveDelayTime = 0;
    
    if ((effectiveDelayTime != 0 || mIsSampleLoaded || inputEnabled) && isPlaying) {
      double eighthNoteTime = (60.0 / bpm) / 2.0;

      int delayTimeInSamples =
          static_cast<int>(eighthNoteTime * effectiveDelayTime * sampleRate);

      for (int channel = 0; channel < totalNumOutputChannels; ++channel) {
           if (channel >= 2) break; // Only mapped to first 2 outputs

           const float* delayBufferData = mDelayBuffer.getReadPointer(channel);
           auto* channelData = buffer.getWritePointer(channel);
          
           const int readPosition =
            (mWritePosition - delayTimeInSamples + sample + delayBufferLength) %
            delayBufferLength;
            
           const float delayedSample = delayBufferData[readPosition];
           channelData[sample] = delayedSample;
      }
    }
  }


  mWritePosition = (mWritePosition + bufferLength) % delayBufferLength;
  
  // Use local 'isPlaying' which accounts for BPM Sync Mode (Manual/MIDI/Host)
  if (isPlaying) {
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
      
      // Update UI Sequence Position to match the audio state at end of block
      mUiSequencePosition.store(mSequencePosition.load());
      
  } else {
      mSamplesToNextBeat.store(0.0);
  }
  
  // Always update UI write position as buffer is circular and write head moves
  mUiWritePosition.store(mWritePosition);
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

//==============================================================================
//==============================================================================
void AmenBreakChopperAudioProcessor::loadBuiltInSample(const juce::String& resourceName) {
    int size = 0;
    const char* data = BinaryData::getNamedResource(resourceName.toRawUTF8(), size);
    
    // Fallback
    if (data == nullptr) {
        juce::String mangled = resourceName.replaceCharacter('.', '_');
        data = BinaryData::getNamedResource(mangled.toRawUTF8(), size);
    }
    
    if (data != nullptr && size > 0) {
        // Determine Target Buffer (Inactive one)
        int currentIndex = mActiveBufferIndex.load();
        int targetIndex = 1 - currentIndex;
        
        auto inputStream = std::make_unique<juce::MemoryInputStream>(data, size, false);
        juce::AudioFormatManager formatManager;
        formatManager.registerBasicFormats();
        
        std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(std::move(inputStream)));
        
        if (reader != nullptr) {
            // Load into Target Buffer
            mSampleBuffers[targetIndex].setSize(reader->numChannels, (int)reader->lengthInSamples);
            reader->read(&mSampleBuffers[targetIndex], 0, (int)reader->lengthInSamples, 0, true, true);
            
            mSampleBufferRates[targetIndex] = reader->sampleRate; 
            
            // Parse BPM
            float newBpm = 120.0f; // default
            juce::String cleanName = resourceName;
            juce::String digits = cleanName.retainCharacters("0123456789.");
            if (digits.isNotEmpty()) {
                float parsed = digits.getFloatValue();
                if (parsed > 30.0f && parsed < 300.0f) {
                    newBpm = parsed;
                }
            }
            
            // Queue the Switch
            mPendingBpm.store(newBpm);
            mPendingSampleSwitch.store(true);
            
        }
    } else {
        juce::Logger::writeToLog("AmenBreakChopper: Failed to load built-in sample " + resourceName);
    }
}

void AmenBreakChopperAudioProcessor::triggerNoteFromUi(int noteNumber) {
  mUiTriggeredNote = noteNumber;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter() {
  return new AmenBreakChopperAudioProcessor();
}
