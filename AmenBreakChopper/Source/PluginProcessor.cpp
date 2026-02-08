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
  DBG("[ABC] *** CONSTRUCTOR CALLED ***");
  mValueTreeState.state.setProperty("oscHostAddress", "127.0.0.1", nullptr);
  mReceiver.addListener(this);
  mValueTreeState.addParameterListener("oscSendPort", this);
  mValueTreeState.addParameterListener("oscReceivePort", this);
  mValueTreeState.addParameterListener("audioSource", this); // Listen for audio source changes
  
#if JUCE_IOS || JUCE_MAC
     mIosLogger = std::make_unique<IOSLogger>();
     juce::Logger::setCurrentLogger(mIosLogger.get());
     juce::Logger::writeToLog("Logger initialized: redirected to os_log");
#endif

  // Safely initialize buffer to prevent division-by-zero if processBlock is called before prepareToPlay
  mDelayBuffer.setSize(2, 2048);
  mDelayBuffer.clear();

  // --- Defaults for Standalone vs Plugin ---
  if (juce::JUCEApplicationBase::isStandaloneApp()) {
      // Standalone: Default to Amen140
      if (auto* p = mValueTreeState.getParameter("audioSource")) {
          if (auto* choice = dynamic_cast<juce::AudioParameterChoice*>(p)) {
              p->setValueNotifyingHost(choice->convertTo0to1(1)); // Index 1 = Amen140
          }
      }
      // inputEnabled: false (0.0f) = internal samples, true (1.0f) = EXT INPUT
      if (auto* p = mValueTreeState.getParameter("inputEnabled"))
          p->setValueNotifyingHost(0.0f); // Disable external input, use internal samples
      DBG("[ABC] Constructor: Set audioSource=Amen140, inputEnabled=0.0 for Standalone");
  } else {
      // Plugin: Default to EXT INPUT
      if (auto* p = mValueTreeState.getParameter("audioSource")) {
          if (auto* choice = dynamic_cast<juce::AudioParameterChoice*>(p)) {
              p->setValueNotifyingHost(choice->convertTo0to1(0)); // Index 0 = EXT INPUT
          }
      }
      if (auto* p = mValueTreeState.getParameter("inputEnabled"))
          p->setValueNotifyingHost(1.0f); // Enable external input
      DBG("[ABC] Constructor: Set audioSource=EXT INPUT, inputEnabled=1.0 for Plugin");
  }
}

AmenBreakChopperAudioProcessor::~AmenBreakChopperAudioProcessor() {
  DBG("[ABC] *** DESTRUCTOR CALLED ***");
}

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
      // If this is the currently recording step, it contains mostly old data (from 16 beats ago).
      // The user requested to hide this.
      // mSequencePosition is the NEXT step index. So (current - 1) is Actively Playing.
      int activeStep = (currentSeqPos - 1 + 16) % 16;
      if (stepIndex == activeStep) {
          for (int i = 0; i < 32; ++i) waveformData.push_back(0.0f);
          continue;
      }
      
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

  // Standalone / Sync Settings
  juce::StringArray bpmModes = {"Host", "MIDI Clock", "Manual"};
  layout.add(std::make_unique<juce::AudioParameterChoice>(
      "bpmSyncMode", "BPM Sync Mode", bpmModes, 0));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      "internalBpm", "Internal BPM", 40.0f, 300.0f, 120.0f));
  
  // Audio Source Selection: EXT INPUT, Amen140, Amen160, Amen180, Amen200
  juce::StringArray audioSources = {"EXT INPUT", "Amen140", "Amen160", "Amen180", "Amen200"};
  layout.add(std::make_unique<juce::AudioParameterChoice>(
      "audioSource", "Audio Source", audioSources, 0)); // Default to EXT INPUT
  
  layout.add(std::make_unique<juce::AudioParameterBool>(
      "inputEnabled", "Input Enabled", false)); // Deprecated, kept for compatibility
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

  // MIDI Controller Settings
  layout.add(std::make_unique<juce::AudioParameterBool>(
      "midiControllerEnabled", "MIDI Controller Enabled", false));
  
  // Radio Button Group
  layout.add(std::make_unique<juce::AudioParameterInt>(
      "radioGroupChannel", "Radio Group MIDI Channel", 0, 15, 0));
  layout.add(std::make_unique<juce::AudioParameterInt>(
      "radioGroupCC", "Radio Group CC Number", 0, 127, 50));
  layout.add(std::make_unique<juce::AudioParameterInt>(
      "radioGroupSelection", "Radio Group Selection", -1, 7, -1));
  
  // Push Button
  layout.add(std::make_unique<juce::AudioParameterInt>(
      "pushButtonChannel", "Push Button MIDI Channel", 0, 15, 0));
  layout.add(std::make_unique<juce::AudioParameterInt>(
      "pushButtonCC", "Push Button CC Number", 0, 127, 5));
  layout.add(std::make_unique<juce::AudioParameterBool>(
      "pushButtonState", "Push Button State", false));
  
  // Slider
  layout.add(std::make_unique<juce::AudioParameterInt>(
      "sliderChannel", "Slider MIDI Channel", 0, 15, 0));
  layout.add(std::make_unique<juce::AudioParameterInt>(
      "sliderCC", "Slider CC Number", 0, 127, 6));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      "sliderValue", "Slider Value", 0.0f, 1.0f, 0.0f));

  // Toggle Buttons (4 square buttons)
  layout.add(std::make_unique<juce::AudioParameterInt>(
      "toggleButtonChannel", "Toggle Button MIDI Channel", 0, 15, 0));
  layout.add(std::make_unique<juce::AudioParameterBool>(
      "toggleButton1", "Toggle Button 1", false));
  layout.add(std::make_unique<juce::AudioParameterBool>(
      "toggleButton2", "Toggle Button 2", false));
  layout.add(std::make_unique<juce::AudioParameterBool>(
      "toggleButton3", "Toggle Button 3", false));
  layout.add(std::make_unique<juce::AudioParameterBool>(
      "toggleButton4", "Toggle Button 4", false));

  // MIDI Controller Advanced - Fader Values (0.0-1.0)
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      "faderAdvanced1", "Fader Advanced 1", 0.0f, 1.0f, 0.0f));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      "faderAdvanced2", "Fader Advanced 2", 0.0f, 1.0f, 0.0f));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      "faderAdvanced3", "Fader Advanced 3", 0.0f, 1.0f, 0.0f));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      "faderAdvanced4", "Fader Advanced 4", 0.0f, 1.0f, 0.0f));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      "faderAdvanced5", "Fader Advanced 5", 0.0f, 1.0f, 0.0f));

  // MIDI Controller Advanced - Toggle States (0 or 1)
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      "toggleAdvanced1", "Toggle Advanced 1", 0.0f, 1.0f, 0.0f));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      "toggleAdvanced2", "Toggle Advanced 2", 0.0f, 1.0f, 0.0f));

  // MIDI Controller Advanced - CC Number Settings
  layout.add(std::make_unique<juce::AudioParameterInt>(
      "ccFaderAdvanced1", "CC Fader Advanced 1", 0, 127, 1));
  layout.add(std::make_unique<juce::AudioParameterInt>(
      "ccFaderAdvanced2", "CC Fader Advanced 2", 0, 127, 2));
  layout.add(std::make_unique<juce::AudioParameterInt>(
      "ccFaderAdvanced3", "CC Fader Advanced 3", 0, 127, 3));
  layout.add(std::make_unique<juce::AudioParameterInt>(
      "ccFaderAdvanced4", "CC Fader Advanced 4", 0, 127, 13));
  layout.add(std::make_unique<juce::AudioParameterInt>(
      "ccFaderAdvanced5", "CC Fader Advanced 5", 0, 127, 14));
  layout.add(std::make_unique<juce::AudioParameterInt>(
      "ccToggleAdvanced1", "CC Toggle Advanced 1", 0, 127, 0));
  layout.add(std::make_unique<juce::AudioParameterInt>(
      "ccToggleAdvanced2", "CC Toggle Advanced 2", 0, 127, 12));

  // MIDI Controller Advanced - MIDI Channel (1-16)
  layout.add(std::make_unique<juce::AudioParameterInt>(
      "midiChannelAdvanced", "MIDI Channel Advanced", 1, 16, 1));

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
  } else if (parameterID == "audioSource") {
    // Handle audio source parameter changes (for state restoration)
    int audioSourceIndex = 0;
    if (auto* p = mValueTreeState.getParameter("audioSource")) {
        if (auto* choice = dynamic_cast<juce::AudioParameterChoice*>(p)) {
            audioSourceIndex = choice->getIndex();
        }
    }
    
    DBG("[ABC] parameterChanged: audioSource = " << audioSourceIndex);
    
    // audioSource: 0=EXT INPUT, 1=Amen140, 2=Amen160, 3=Amen180, 4=Amen200
    if (audioSourceIndex > 0) {
        // Load internal sample
        juce::String sampleToLoad = "";
        if (audioSourceIndex == 1) sampleToLoad = "amen140.wav";
        else if (audioSourceIndex == 2) sampleToLoad = "amen160.wav";
        else if (audioSourceIndex == 3) sampleToLoad = "amen180.wav";
        else if (audioSourceIndex == 4) sampleToLoad = "amen200.wav";
        
        if (sampleToLoad.isNotEmpty()) {
            DBG("[ABC] parameterChanged: Loading sample: " << sampleToLoad);
            loadBuiltInSample(sampleToLoad);
            
            // Force immediate switch
            if (mPendingSampleSwitch.load()) {
                mActiveBufferIndex.store(1 - mActiveBufferIndex.load());
                mIsSampleLoaded = true;
                mPendingSampleSwitch = false;
                mWaveformDirty = true;
                DBG("[ABC] parameterChanged: Sample switched successfully");
            }
        }
    } else {
        // EXT INPUT mode - no sample to load
        DBG("[ABC] parameterChanged: EXT INPUT mode selected");
    }
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
  DBG("[ABC] *** prepareToPlay CALLED *** sampleRate=" << sampleRate << " samplesPerBlock=" << samplesPerBlock);

  // OSC Sender
  auto hostAddress =
      mValueTreeState.state.getProperty("oscHostAddress").toString();
  auto sendPort =
      (int)mValueTreeState.getRawParameterValue("oscSendPort")->load();
  if (!mSender.connect(hostAddress, sendPort))
    DBG("AmenBreakChopper: Failed to connect OSC sender.");

  // OSC Receiver
  auto receivePort =
      (int)mValueTreeState.getRawParameterValue("oscReceivePort")->load();
  mReceiver.connect(receivePort);
    DBG("AmenBreakChopper: Failed to connect OSC receiver.");

  mMidiClockTracker.reset();
  mSampleRate = sampleRate;
  if (mSampleRate <= 0.0) mSampleRate = 44100.0; // Fallback safety

  // We enforce a Stereo internal buffer for the delay/looping logic.
  // Input routing will map selected inputs to this stereo pair.
  int delayBufferSize =
      static_cast<int>(16.0 * mSampleRate); // 16 seconds max delay

  if (delayBufferSize <= 0) delayBufferSize = 2048; // Safety minimum

  mDelayBuffer.setSize(2, delayBufferSize); // Fixed 2 channels (Stereo)
  mDelayBuffer.clear();

  // Initialize checks
  if (!mIsInitialized) {
      // Initialize sequencer state
      mNextEighthNotePpq = 0.0;
      mSequencePosition = 0;
      mNoteSequencePosition = 0;
      mLastReceivedNoteValue = 0;
      mSequenceResetQueued = false;
      mHardResetQueued = false;
      mNewNoteReceived = false;
      mLastDelayAdjustFwdCcValue = 0;
      mLastDelayAdjustBwdCcValue = 0;
      mLastDelayAdjustFwdCcValue = 0;
      mLastDelayAdjustBwdCcValue = 0;
      mLastDelayAdjust = 0;
      mWarmUpCounter = 20; // Initialize warm-up counter

      mIsSampleLoaded = false;
      mSampleReadPos = 0.0;
      mSampleBufferRates[0] = 44100.0;
      mSampleBufferRates[1] = 44100.0;
      mSampleBuffers[0].setSize(0, 0); // Clear logic
      mSampleBuffers[1].setSize(0, 0); 
      
      // Detect if running as Standalone (not as a plugin in a DAW)
      juce::PluginHostType hostType;
      bool isStandalone = false;
      
      // Check wrapper type first (most reliable)
      if (hostType.getPluginLoadedAs() == juce::AudioProcessor::wrapperType_Standalone) {
          isStandalone = true;
      }
      // Fallback to JUCEApplicationBase check (for non-plugin builds)
      else if (juce::JUCEApplicationBase::isStandaloneApp()) {
          isStandalone = true;
      }

      // Load sample based on audioSource parameter
      // This allows correct restoration of saved state
      int audioSourceIndex = 0;
      if (auto* p = mValueTreeState.getParameter("audioSource")) {
          if (auto* choice = dynamic_cast<juce::AudioParameterChoice*>(p)) {
              audioSourceIndex = choice->getIndex();
          }
      }
      
      DBG("[ABC] prepareToPlay: audioSourceIndex = " << audioSourceIndex);
      
      // audioSource: 0=EXT INPUT, 1=Amen140, 2=Amen160, 3=Amen180, 4=Amen200
      bool shouldLoadSample = (audioSourceIndex > 0); // Load sample if not EXT INPUT
      juce::String sampleToLoad = "";
      
      if (audioSourceIndex == 1) sampleToLoad = "amen140.wav";
      else if (audioSourceIndex == 2) sampleToLoad = "amen160.wav";
      else if (audioSourceIndex == 3) sampleToLoad = "amen180.wav";
      else if (audioSourceIndex == 4) sampleToLoad = "amen200.wav";
      
      if (!mIsSampleLoaded && mSampleBuffers[0].getNumSamples() == 0 && shouldLoadSample) {
          // Load the selected sample
          loadBuiltInSample(sampleToLoad);
          DBG("[ABC] prepareToPlay: loadBuiltInSample called (" << (isStandalone ? "Standalone" : "Plugin") << " mode): " << sampleToLoad);
          
          // Force immediate switch for startup
          if (mPendingSampleSwitch.load()) {
             DBG("[ABC] prepareToPlay: Pending sample switch detected, applying now");
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
             
             // Set bpmSyncMode to MANUAL when loading internal samples
             if (auto* p = mValueTreeState.getParameter("bpmSyncMode")) {
                 // Use raw set for safety during init
                 if (auto* raw = mValueTreeState.getRawParameterValue("bpmSyncMode"))
                     raw->store(1.0f); // Manual mode for internal samples
             }
                 
             mWaveformDirty = true;
          }
      } else {
          if (!shouldLoadSample) {
              DBG("[ABC] prepareToPlay: EXT INPUT mode, skipping sample load");
          } else {
              DBG("[ABC] prepareToPlay: Sample already loaded, skipping initialization");
          }
      }

      
      mIsInitialized = true;
      
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

  // Guard against uninitialized state which can cause infinite loops or crashes
  if (mSampleRate <= 0.0) {
      midiMessages.clear();
      return;
  }

  // Audio Warm-up: Silence initial blocks to prevent garbage/feedback during init
  if (mWarmUpCounter.load() > 0) {
      mWarmUpCounter--;
      buffer.clear();
      midiMessages.clear();
      return;
  }

  auto totalNumInputChannels = getTotalNumInputChannels();
  auto totalNumOutputChannels = getTotalNumOutputChannels();

  // --- Clear unused output channels ---
  for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
    buffer.clear(i, 0, buffer.getNumSamples());

  // --- Parameters ---
  auto *bpmModeParam = mValueTreeState.getRawParameterValue("bpmSyncMode");
  bool useMidiClock = (bpmModeParam->load() >= 0.5f);

  // Determine input mode from audioSource parameter
  // audioSource: 0=EXT INPUT, 1=Amen140, 2=Amen160, 3=Amen180, 4=Amen200
  int audioSourceIndex = 0;
  if (auto* p = mValueTreeState.getParameter("audioSource")) {
      if (auto* choice = dynamic_cast<juce::AudioParameterChoice*>(p)) {
          audioSourceIndex = choice->getIndex();
      }
  }
  
  bool inputEnabled = (audioSourceIndex == 0); // EXT INPUT when audioSource is 0
  
  // CRITICAL FIX: Ensure mDelayBuffer is initialized even if prepareToPlay wasn't called
  // This happens when DAW loads a saved project without calling prepareToPlay
  if (mDelayBuffer.getNumSamples() == 0 || mDelayBuffer.getNumChannels() == 0) {
      juce::Logger::writeToLog("[ABC] processBlock: mDelayBuffer not initialized! Initializing now...");
      const int delayBufferSize = static_cast<int>(getSampleRate() * 10.0); // 10 seconds
      mDelayBuffer.setSize(2, delayBufferSize);
      mDelayBuffer.clear();
      juce::Logger::writeToLog("[ABC] processBlock: mDelayBuffer initialized with " + juce::String(delayBufferSize) + " samples");
  }
  
  // DEBUG: Log audioSource value (temporarily logging every call for debugging)
  static int debugCounter = 0;
  if (++debugCounter > 100) {
      debugCounter = 0;
      juce::Logger::writeToLog("[ABC] audioSource = " + juce::String(audioSourceIndex) + " -> " + (inputEnabled ? "EXT INPUT" : "INTERNAL SAMPLE"));
  }


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

  // DEBUG: Log bpmMode value once per second
  static int bpmModeDebugCounter = 0;
  if (++bpmModeDebugCounter > 44100 / 256) {
      bpmModeDebugCounter = 0;
      DBG("[ABC] bpmMode: " << bpmMode << " (val: " << bpmModeVal << "), isPlaying will be: " << (bpmMode == 0 ? "from host" : "true"));
  }

  if (bpmMode == 1) { // MIDI Clock
      bpm = mMidiClockTracker.detectedBpm;
      ppqAtStartOfBlock = mMidiClockPpq;
      // We assume playing if using MIDI clock logic (or check clock active?)
      isPlaying = true; 
  } else if (bpmMode == 2) { // Manual
      if (auto* p = mValueTreeState.getRawParameterValue("internalBpm")) {
          bpm = (double)p->load();
      }
      // Use Internal Accumulator for Manual Mode (not MIDI clock)
      ppqAtStartOfBlock = mInternalPpqAccumulator;
      isPlaying = true;
  } else { // Host (0)
      bpm = positionInfo.getBpm().orFallback(120.0);
      ppqAtStartOfBlock = positionInfo.getPpqPosition().orFallback(0.0);
      isPlaying = positionInfo.getIsPlaying();
      
      // Force playback in Standalone mode (no host to provide transport)
      // Use wrapper type to detect standalone, not platform
      juce::PluginHostType hostType;
      bool isStandaloneEnv = (hostType.getPluginLoadedAs() == juce::AudioProcessor::wrapperType_Standalone);
      if (!isStandaloneEnv && juce::JUCEApplicationBase::isStandaloneApp()) {
          isStandaloneEnv = true; // Fallback for non-plugin builds
      }

      if (isStandaloneEnv) {
          isPlaying = true;
          // FORCE Internal Accumulator usage on iOS Standalone regardless of mode to ensure movement
          // If we rely on Host PPQ (which is 0) or MIDI Clock (which might be 0), we freeze.
          // So we override to use our internal accumulator.
          ppqAtStartOfBlock = mInternalPpqAccumulator;
          
          // Also force BPM to internal if it looks invalid
          if (bpm < 10.0) {
              if (auto* p = mValueTreeState.getRawParameterValue("internalBpm"))
                  bpm = (double)p->load();
              if (bpm < 10.0) bpm = 120.0;
          }
      }
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
  const double sampleRate = mSampleRate; // Use our sanitized rate
  const double ppqPerSample = bpm / (60.0 * sampleRate);

  // --- Handle transport jumps or looping ---
  if (isPlaying && !useMidiClock) {
      if (positionInfo.getIsLooping() ||
          (ppqAtStartOfBlock < mNextEighthNotePpq - 0.5)) {
        mNextEighthNotePpq = std::ceil(ppqAtStartOfBlock * 2.0) / 2.0;
      }
  }


  // Update BPM for UI
  mCurrentBpm.store((float)bpm);
  mUsingMidiClock.store(useMidiClock); // For UI

  // --- Sequencer Tick Logic (Block-based) ---
  const int bufferLength = buffer.getNumSamples();
  double ppqAtEndOfBlock = ppqAtStartOfBlock + (bufferLength * ppqPerSample);
  
  // Advance MIDI Clock PPQ for next block
  // Advance MIDI Clock PPQ for next block
  if (bpmMode >= 1) { // MIDI Clock or Manual
      mMidiClockPpq = ppqAtEndOfBlock;
  }
  
  if (bpmMode >= 1) { // MIDI Clock or Manual
      mMidiClockPpq = ppqAtEndOfBlock;
  }
  
  bool isStandaloneEnv = false;
#if JUCE_IOS
  isStandaloneEnv = true;
#else
  isStandaloneEnv = juce::JUCEApplicationBase::isStandaloneApp();
#endif
  
  // Always update accumulator in Standalone/iOS so we have continuity
  if (isStandaloneEnv) {
      mInternalPpqAccumulator = ppqAtEndOfBlock;
      
      // Safety initialization for first run
      if (mNextEighthNotePpq < mInternalPpqAccumulator - 24.0) { // If drifted too far back (initial state)
           mNextEighthNotePpq = mInternalPpqAccumulator;
      }
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
      mSequencePosition = 0;
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
            
            // When switching to an internal sample, always set to MANUAL mode
            // This allows the sample's BPM to be used
            if (auto* p = mValueTreeState.getParameter("bpmSyncMode")) {
                p->setValueNotifyingHost(1.0f); // Manual
            }
            
            // Disable Input to use the loaded sample
            if (auto* p = mValueTreeState.getParameter("inputEnabled"))
                p->setValueNotifyingHost(0.0f);

            // 3. Reset State
            mIsSampleLoaded = true;
            mSampleReadPos = 0.0;
            mSequencePosition = 0;
            mNoteSequencePosition = 0;
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

  // Guard against zero-sized buffer (e.g. uninitialized or 0 sample rate)
  if (delayBufferLength <= 0) return;

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
            float l = mSampleBuffers[idx].getSample(0, (int)mSampleReadPos);
            float r = (mSampleBuffers[idx].getNumChannels() > 1) ? mSampleBuffers[idx].getSample(1, (int)mSampleReadPos) : l;
            
            mDelayBuffer.getWritePointer(0)[(mWritePosition + sample) % delayBufferLength] = l;
            mDelayBuffer.getWritePointer(1)[(mWritePosition + sample) % delayBufferLength] = r;

            // Increment based on ratio
            double ratio = (mSampleRate > 0.0) ? (mSampleBufferRates[idx] / mSampleRate) : 1.0;
            mSampleReadPos += ratio;
            if (mSampleReadPos >= mSampleBuffers[idx].getNumSamples()) mSampleReadPos = 0.0;
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
    if ((currentDelayTime != 0 || mIsSampleLoaded) && isPlaying) {
      double eighthNoteTime = (60.0 / bpm) / 2.0;
      // Recalculate or reuse sampleRate if scope issue
      // We need sampleRate here. 'sampleRate' is defined at line 634 (in previous chunk view)
      // BUT if I messed up the scope with the duplicate ELSE, the compiler might be confused.
      // However, looking at the code, sampleRate is defined in the main block.
      // Wait, line 634: const double sampleRate = getSampleRate();
      // If that is inside the main processBlock, it should be visible here.
      // Unless the duplicated } else { closed the scope early! (Line 582)
      // Yes, } else { ... } ... 
      // The duplicated } closes the previous if (bpmMode == 2).
      // Then else { ... } opens a new block?
      // No, syntax error "Expected expression" at 582:5.
      
      // So fixing the duplicate else should fix the scope of sampleRate IF sampleRate is defined after it.
      // sampleRate is defined at 634. Usage is at 809.
      // So ensuring sampleRate is defined correctly is key.
      int delayTimeInSamples =
          static_cast<int>(eighthNoteTime * currentDelayTime * sampleRate);

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
  } else {
      mSamplesToNextBeat.store(0.0);
  }

  // --- MIDI Controller Output ---
  // Only send MIDI if controller is enabled
  bool midiControllerEnabled = mValueTreeState.getRawParameterValue("midiControllerEnabled")->load() > 0.5f;
  
  if (midiControllerEnabled) {
    // Radio Button Group
    int radioGroupSelection = (int)mValueTreeState.getRawParameterValue("radioGroupSelection")->load();
    if (radioGroupSelection != mLastRadioGroupSelection) {
      int radioChannel = (int)mValueTreeState.getRawParameterValue("radioGroupChannel")->load();
      int radioCC = (int)mValueTreeState.getRawParameterValue("radioGroupCC")->load();
      
      // Send note-off for previous selection (if valid)
      if (mLastRadioGroupSelection >= 0 && mLastRadioGroupSelection <= 7) {
        juce::MidiMessage noteOff = juce::MidiMessage::controllerEvent(
            radioChannel + 1, radioCC, 0);
        processedMidi.addEvent(noteOff, 0);
      }
      
      // Send note-on for new selection (or 0 if -1 = no selection)
      if (radioGroupSelection >= 0 && radioGroupSelection <= 7) {
        // Calculate CC value: floor(127 / 10 * (index + 1))
        int ccValue = static_cast<int>(std::floor(127.0 / 10.0 * (radioGroupSelection + 1)));
        juce::MidiMessage noteOn = juce::MidiMessage::controllerEvent(
            radioChannel + 1, radioCC, ccValue);
        processedMidi.addEvent(noteOn, 0);
      } else if (radioGroupSelection == -1) {
        // Finger released, send 0
        juce::MidiMessage noteOff = juce::MidiMessage::controllerEvent(
            radioChannel + 1, radioCC, 0);
        processedMidi.addEvent(noteOff, 0);
      }
      
      mLastRadioGroupSelection = radioGroupSelection;
    }
    
    // Push Button
    bool pushButtonState = mValueTreeState.getRawParameterValue("pushButtonState")->load() > 0.5f;
    if (pushButtonState != mLastPushButtonState) {
      int pushChannel = (int)mValueTreeState.getRawParameterValue("pushButtonChannel")->load();
      int pushCC = (int)mValueTreeState.getRawParameterValue("pushButtonCC")->load();
      
      int ccValue = pushButtonState ? 127 : 0;
      juce::MidiMessage msg = juce::MidiMessage::controllerEvent(
          pushChannel + 1, pushCC, ccValue);
      processedMidi.addEvent(msg, 0);
      
      mLastPushButtonState = pushButtonState;
    }
    
    // Slider
    float sliderValue = mValueTreeState.getRawParameterValue("sliderValue")->load();
    // Only send if value changed significantly (avoid flooding)
    if (std::abs(sliderValue - mLastSliderValue) > 0.008f) { // ~1/127 resolution
      int sliderChannel = (int)mValueTreeState.getRawParameterValue("sliderChannel")->load();
      int sliderCC = (int)mValueTreeState.getRawParameterValue("sliderCC")->load();
      
      int ccValue = static_cast<int>(sliderValue * 127.0f);
      ccValue = std::max(0, std::min(127, ccValue)); // Clamp to valid range
      
      juce::MidiMessage msg = juce::MidiMessage::controllerEvent(
          sliderChannel + 1, sliderCC, ccValue);
      processedMidi.addEvent(msg, 0);
      
      mLastSliderValue = sliderValue;
    }

    // Toggle Buttons (4 square buttons)
    int toggleChannel = (int)mValueTreeState.getRawParameterValue("toggleButtonChannel")->load();
    
    // Toggle Button 1 (CC 30)
    bool toggleButton1 = mValueTreeState.getRawParameterValue("toggleButton1")->load() > 0.5f;
    if (toggleButton1 != mLastToggleButton1) {
      int ccValue = toggleButton1 ? 127 : 0;
      juce::MidiMessage msg = juce::MidiMessage::controllerEvent(
          toggleChannel + 1, 30, ccValue);
      processedMidi.addEvent(msg, 0);
      mLastToggleButton1 = toggleButton1;
    }
    
    // Toggle Button 2 (CC 31)
    bool toggleButton2 = mValueTreeState.getRawParameterValue("toggleButton2")->load() > 0.5f;
    if (toggleButton2 != mLastToggleButton2) {
      int ccValue = toggleButton2 ? 127 : 0;
      juce::MidiMessage msg = juce::MidiMessage::controllerEvent(
          toggleChannel + 1, 31, ccValue);
      processedMidi.addEvent(msg, 0);
      mLastToggleButton2 = toggleButton2;
    }
    
    // Toggle Button 3 (CC 32)
    bool toggleButton3 = mValueTreeState.getRawParameterValue("toggleButton3")->load() > 0.5f;
    if (toggleButton3 != mLastToggleButton3) {
      int ccValue = toggleButton3 ? 127 : 0;
      juce::MidiMessage msg = juce::MidiMessage::controllerEvent(
          toggleChannel + 1, 32, ccValue);
      processedMidi.addEvent(msg, 0);
      mLastToggleButton3 = toggleButton3;
    }
    
    // Toggle Button 4 (CC 33)
    bool toggleButton4 = mValueTreeState.getRawParameterValue("toggleButton4")->load() > 0.5f;
    if (toggleButton4 != mLastToggleButton4) {
      int ccValue = toggleButton4 ? 127 : 0;
      juce::MidiMessage msg = juce::MidiMessage::controllerEvent(
          toggleChannel + 1, 33, ccValue);
      processedMidi.addEvent(msg, 0);
      mLastToggleButton4 = toggleButton4;
    }
    
    // --- MIDI Controller Advanced ---
    int midiChannelAdvanced = (int)mValueTreeState.getRawParameterValue("midiChannelAdvanced")->load();
    
    // Faders (5 faders)
    for (int i = 1; i <= 5; ++i) {
      juce::String faderParamId = "faderAdvanced" + juce::String(i);
      juce::String ccParamId = "ccFaderAdvanced" + juce::String(i);
      
      float faderValue = mValueTreeState.getRawParameterValue(faderParamId)->load();
      float* lastValue = nullptr;
      
      if (i == 1) lastValue = &mLastFaderAdvanced1;
      else if (i == 2) lastValue = &mLastFaderAdvanced2;
      else if (i == 3) lastValue = &mLastFaderAdvanced3;
      else if (i == 4) lastValue = &mLastFaderAdvanced4;
      else if (i == 5) lastValue = &mLastFaderAdvanced5;
      
      if (lastValue && std::abs(faderValue - *lastValue) > 0.008f) {
        int ccNumber = (int)mValueTreeState.getRawParameterValue(ccParamId)->load();
        int ccValue = static_cast<int>(faderValue * 127.0f);
        ccValue = std::max(0, std::min(127, ccValue));
        
        juce::MidiMessage msg = juce::MidiMessage::controllerEvent(
            midiChannelAdvanced, ccNumber, ccValue);
        processedMidi.addEvent(msg, 0);
        
        *lastValue = faderValue;
      }
    }
    
    // Toggles (2 toggles)
    for (int i = 1; i <= 2; ++i) {
      juce::String toggleParamId = "toggleAdvanced" + juce::String(i);
      juce::String ccParamId = "ccToggleAdvanced" + juce::String(i);
      
      bool toggleState = mValueTreeState.getRawParameterValue(toggleParamId)->load() > 0.5f;
      bool* lastState = (i == 1) ? &mLastToggleAdvanced1 : &mLastToggleAdvanced2;
      
      if (toggleState != *lastState) {
        int ccNumber = (int)mValueTreeState.getRawParameterValue(ccParamId)->load();
        int ccValue = toggleState ? 127 : 0;
        
        juce::MidiMessage msg = juce::MidiMessage::controllerEvent(
            midiChannelAdvanced, ccNumber, ccValue);
        processedMidi.addEvent(msg, 0);
        
        *lastState = toggleState;
      }
    }
  }

  // Merge generated MIDI with output
  midiMessages.swapWith(processedMidi);
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
  DBG("[ABC] setStateInformation: CALLED with " << sizeInBytes << " bytes");
  
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

  // Debug: Log inputEnabled value after state restoration
  if (auto* p = mValueTreeState.getRawParameterValue("inputEnabled")) {
      DBG("[ABC] setStateInformation: inputEnabled = " << p->load());
  }

  // Force Input disable on state load for Standalone (Safe Raw Set)
#if JUCE_IOS
  if (true) {
#else
  if (juce::JUCEApplicationBase::isStandaloneApp()) {
#endif
      if (auto* p = mValueTreeState.getRawParameterValue("inputEnabled"))
          p->store(0.0f);
      DBG("[ABC] setStateInformation: Forced inputEnabled = 0.0 for Standalone");
  }
  
  // Load sample based on restored audioSource parameter
  // This ensures the correct sample is loaded when opening saved projects
  int audioSourceIndex = 0;
  if (auto* p = mValueTreeState.getParameter("audioSource")) {
      if (auto* choice = dynamic_cast<juce::AudioParameterChoice*>(p)) {
          audioSourceIndex = choice->getIndex();
      }
  }
  
  DBG("[ABC] setStateInformation: audioSourceIndex = " << audioSourceIndex);
  
  // audioSource: 0=EXT INPUT, 1=Amen140, 2=Amen160, 3=Amen180, 4=Amen200
  if (audioSourceIndex > 0) {
      juce::String sampleToLoad = "";
      if (audioSourceIndex == 1) sampleToLoad = "amen140.wav";
      else if (audioSourceIndex == 2) sampleToLoad = "amen160.wav";
      else if (audioSourceIndex == 3) sampleToLoad = "amen180.wav";
      else if (audioSourceIndex == 4) sampleToLoad = "amen200.wav";
      
      if (sampleToLoad.isNotEmpty()) {
          DBG("[ABC] setStateInformation: Loading sample: " << sampleToLoad);
          loadBuiltInSample(sampleToLoad);
          
          // Force immediate switch
          if (mPendingSampleSwitch.load()) {
              mActiveBufferIndex.store(1 - mActiveBufferIndex.load());
              mIsSampleLoaded = true;
              mPendingSampleSwitch = false;
              mWaveformDirty = true;
              DBG("[ABC] setStateInformation: Sample switched successfully");
          }
      }
  } else {
      DBG("[ABC] setStateInformation: EXT INPUT mode, no sample to load");
  }
}

//==============================================================================

//==============================================================================
//==============================================================================
void AmenBreakChopperAudioProcessor::loadBuiltInSample(const juce::String& resourceName) {
    juce::Logger::writeToLog("[ABC] loadBuiltInSample called: " + resourceName);
    int size = 0;
    const char* data = BinaryData::getNamedResource(resourceName.toRawUTF8(), size);
    
    // Fallback
    if (data == nullptr) {
        // Try underscore version (amen140_wav)
        juce::String mangled = resourceName.replaceCharacter('.', '_');
        data = BinaryData::getNamedResource(mangled.toRawUTF8(), size);
    }
    
    if (data == nullptr) {
        // Try capitalized ID version (Amen140) - assuming specific convention
        juce::String idName = resourceName.upToFirstOccurrenceOf(".", false, false);
        idName = idName.substring(0, 1).toUpperCase() + idName.substring(1);
        data = BinaryData::getNamedResource(idName.toRawUTF8(), size);
    }
    
    // Explicit hardcoded check for amen140.wav (most common default)
    if (data == nullptr && resourceName == "amen140.wav") {
         data = BinaryData::getNamedResource("amen140_wav", size);
         if (data == nullptr) data = BinaryData::getNamedResource("Amen140", size);
    }

    if (data == nullptr) {
        // Try Samples/ path
        juce::String pathName = "Samples_" + resourceName.replaceCharacter('.', '_');
        data = BinaryData::getNamedResource(pathName.toRawUTF8(), size);
    }
    
    if (data != nullptr && size > 0) {
        juce::Logger::writeToLog("[ABC] Sample data found, size: " + juce::String(size));
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
        DBG("AmenBreakChopper: Failed to load built-in sample " << resourceName << " - Generating fallback tone");
        
        // Final Fallback: Generate a test tone (Sine Wave) so we know the engine is working
        // but the file system/BinaryData is failing.
        int targetIndex = 1 - mActiveBufferIndex.load();
        int length = 44100 * 2; // 2 seconds
        mSampleBuffers[targetIndex].setSize(2, length);
        auto* w = mSampleBuffers[targetIndex].getArrayOfWritePointers();
        double phase = 0.0;
        double inc = 440.0 * 2.0 * 3.14159 / 44100.0;
        for (int i=0; i<length; ++i) {
            float s = (float)std::sin(phase) * 0.5f;
            w[0][i] = s;
            w[1][i] = s;
            phase += inc;
        }
        mSampleBufferRates[targetIndex] = 44100.0;
        
        // Queue the Switch
        mPendingBpm.store(120.0f);
        mPendingSampleSwitch.store(true);
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
