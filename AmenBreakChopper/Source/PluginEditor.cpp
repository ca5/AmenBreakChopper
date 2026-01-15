/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginEditor.h"
#include "PluginProcessor.h"
// Required for Bluetooth Pairing Dialog
#if JUCE_IOS || JUCE_ANDROID
 #include <juce_audio_utils/juce_audio_utils.h>
#endif
#include <juce_audio_plugin_client/Standalone/juce_StandaloneFilterWindow.h> // For StandalonePluginHolder
#include <optional>

//==============================================================================
AmenBreakChopperAudioProcessorEditor::AmenBreakChopperAudioProcessorEditor(
    AmenBreakChopperAudioProcessor &p)
    : AudioProcessorEditor(&p), audioProcessor(p),
      webView(juce::WebBrowserComponent::Options()
          .withNativeIntegrationEnabled()
#if JUCE_WINDOWS
          .withBackend(juce::WebBrowserComponent::Options::Backend::webview2)
          .withWinWebView2Options(juce::WebBrowserComponent::Options::WinWebView2Options()
              .withUserDataFolder(juce::File::getSpecialLocation(juce::File::tempDirectory)))
#endif
          .withResourceProvider(
                  [this](const juce::String &url)
                      -> std::optional<juce::WebBrowserComponent::Resource> {
                    // 1. Determine relative path from URL
                    juce::String resourcePath =
                        (url == "/" || url == "") ? "index.html" : url;
                    if (resourcePath.startsWithChar('/'))
                      resourcePath = resourcePath.substring(1);

                    juce::Logger::writeToLog("WebView Request: " + url +
                                             " -> " + resourcePath);

                    // 2. Locate the file on disk
                    auto exe = juce::File::getSpecialLocation(
                        juce::File::currentApplicationFile);
                    juce::File bundleRoot;

                    if (exe.isDirectory())
                      bundleRoot =
                          exe; // It's the .app bundle itself (iOS/macOS)
                    else
                      bundleRoot =
                          exe.getParentDirectory(); // It's the executable
                                                    // binary

                    juce::Logger::writeToLog("Bundle Root: " +
                                             bundleRoot.getFullPathName());

                    juce::Array<juce::File> candidates;

                    // A: iOS Bundle Root / Flattened Structure Check
                    // Case 1: Path exactly as requested (e.g.
                    // ABC.app/assets/index.js or ABC.app/index.html)
                    candidates.add(bundleRoot.getChildFile(resourcePath));

                    // Case 2: Flattened assets (Common Xcode "Create groups"
                    // mistake) Request: assets/index.js -> Look for:
                    // ABC.app/index.js
                    candidates.add(
                        bundleRoot.getChildFile(resourcePath.substring(
                            resourcePath.lastIndexOfChar('/') + 1)));

                    // B: iOS Bundle Root / dist folder (If 'dist' folder
                    // reference was used)
                    candidates.add(bundleRoot.getChildFile("dist").getChildFile(
                        resourcePath));
                    // Case B2: Flattened inside dist
                    candidates.add(bundleRoot.getChildFile("dist").getChildFile(
                        resourcePath.substring(
                            resourcePath.lastIndexOfChar('/') + 1)));

                    // C: macOS Resources
                    if (bundleRoot.getFileName() == "MacOS") {
                      auto resources =
                          bundleRoot.getParentDirectory().getChildFile(
                              "Resources");
                      candidates.add(
                          resources.getChildFile("dist").getChildFile(
                              resourcePath));
                      candidates.add(resources.getChildFile(resourcePath));
                    }

                    juce::File resourceFile;
                    for (const auto &c : candidates) {
                      if (c.existsAsFile()) {
                        resourceFile = c;
                        break;
                      }
                    }

                    auto getMimeType = [](const juce::String &ext) {
                      if (ext == ".html")
                        return "text/html";
                      if (ext == ".js")
                        return "text/javascript";
                      if (ext == ".css")
                        return "text/css";
                      if (ext == ".png")
                        return "image/png";
                      if (ext == ".jpg")
                        return "image/jpeg";
                      if (ext == ".svg")
                        return "image/svg+xml";
                      return "application/octet-stream";
                    };

                    // 1. Try Disk
                    if (resourceFile.existsAsFile()) {
                      juce::Logger::writeToLog("Found resource: " +
                                               resourceFile.getFullPathName());
                      juce::MemoryBlock mb;
                      resourceFile.loadFileAsData(mb);

                      auto mimeType =
                          getMimeType(resourceFile.getFileExtension());
                      const auto *bytes =
                          reinterpret_cast<const std::byte *>(mb.getData());

                      return juce::WebBrowserComponent::Resource{
                          std::vector<std::byte>(bytes, bytes + mb.getSize()),
                          mimeType};
                    }

                    // 2. Try BinaryData Fallback
                    juce::String filename = resourcePath.substring(
                        resourcePath.lastIndexOfChar('/') + 1);
                    int bdSize = 0;
                    const char *bdData = nullptr;

                    // Try 1: Exact filename
                    bdData = BinaryData::getNamedResource(filename.toRawUTF8(),
                                                          bdSize);

                    // Try 2: Mangled A (Standard: dots and hyphens to
                    // underscores)
                    if (bdData == nullptr) {
                      juce::String mangled =
                          filename.replaceCharacter('.', '_').replaceCharacter(
                              '-', '_');
                      bdData = BinaryData::getNamedResource(mangled.toRawUTF8(),
                                                            bdSize);
                    }

                    // Try 3: Mangled B (Observed: hyphens removed, dots to
                    // underscores)
                    if (bdData == nullptr) {
                      juce::String mangled =
                          filename.removeCharacters("-").replaceCharacter('.',
                                                                          '_');
                      bdData = BinaryData::getNamedResource(mangled.toRawUTF8(),
                                                            bdSize);
                    }

                    if (bdData != nullptr) {
                      juce::Logger::writeToLog("Found in BinaryData: " +
                                               filename);
                      auto mimeType = getMimeType(
                          "." + filename.substring(
                                    filename.lastIndexOfChar('.') + 1));

                      const auto *bytes =
                          reinterpret_cast<const std::byte *>(bdData);
                      return juce::WebBrowserComponent::Resource{
                          std::vector<std::byte>(bytes, bytes + bdSize),
                          mimeType};
                    }

                    // Debug: List what we have in BinaryData
                    // (Partial/Heuristic) We can't iterate easily, but we can
                    // log that we failed.
                    juce::Logger::writeToLog(
                        "Resource NOT found in BinaryData: " + filename);

                    juce::Logger::writeToLog("Resource NOT found: " +
                                             resourcePath);
                    return std::nullopt;
                  })
              .withNativeFunction(
                  "sendParameterValue",
                  [this](const juce::Array<juce::var> &args,
                         juce::WebBrowserComponent::NativeFunctionCompletion
                             completion) {
                    if (args.size() == 2 && args[0].isString()) {
                      juce::String paramId = args[0].toString();
                      float value = static_cast<float>(args[1]);

                      auto *param =
                          audioProcessor.getValueTreeState().getParameter(
                              paramId);
                      if (param != nullptr) {
                        // Convert world value to normalized 0-1
                        float normalized = param->convertTo0to1(value);
                        param->setValueNotifyingHost(normalized);
                      }
                    }
                    completion(juce::var());
                  })
              // --- Register Reset Commands ---
              .withNativeFunction(
                  "performSequenceReset",
                  [this](const juce::Array<juce::var> &,
                         juce::WebBrowserComponent::NativeFunctionCompletion
                             completion) {
                    audioProcessor.performSequenceReset();
                    completion(juce::var());
                  })
              .withNativeFunction(
                  "performSoftReset",
                  [this](const juce::Array<juce::var> &,
                         juce::WebBrowserComponent::NativeFunctionCompletion
                             completion) {
                    audioProcessor.performSoftReset();
                    completion(juce::var());
                  })
              .withNativeFunction(
                  "performHardReset",
                  [this](const juce::Array<juce::var> &,
                         juce::WebBrowserComponent::NativeFunctionCompletion
                             completion) {
                    audioProcessor.performHardReset();
                    completion(juce::var());
                  })
              .withNativeFunction(
                  "triggerNoteFromUi",
                  [this](const juce::Array<juce::var> &args,
                         juce::WebBrowserComponent::NativeFunctionCompletion
                             completion) {
                    if (args.size() == 1 && args[0].isInt()) {
                      audioProcessor.triggerNoteFromUi(
                          static_cast<int>(args[0]));
                    }
                    completion(juce::var());
                  })

              .withNativeFunction(
                  "requestInitialState",
                  [this](const juce::Array<juce::var> &,
                         juce::WebBrowserComponent::NativeFunctionCompletion
                             completion) {
                    hasFrontendConnected = true;
                    syncAllParametersToFrontend();

                    // Send Environment Info
                    bool isStandalone = false;
                    if (juce::JUCEApplicationBase::isStandaloneApp())
                        isStandalone = true;
                    
                    juce::DynamicObject* obj = new juce::DynamicObject();
                    obj->setProperty("isStandalone", isStandalone);
                    juce::String js = "if (typeof window.juce_emitEvent === 'function') { "
                                      "window.juce_emitEvent('environment', " + juce::JSON::toString(juce::var(obj)) + "); }";
                    webView.evaluateJavascript(js);

                    completion(juce::var());
                  })
              // --- Device Management (Standalone) ---
              .withNativeFunction(
                  "getDeviceList",
                  [this](const juce::Array<juce::var> &,
                         juce::WebBrowserComponent::NativeFunctionCompletion
                             completion) {
                    sendDeviceList();
                    completion(juce::var());
                  })
              .withNativeFunction(
                  "setAudioDevice",
                  [this](const juce::Array<juce::var> &args,
                         juce::WebBrowserComponent::NativeFunctionCompletion
                             completion) {
                    if (deviceManager != nullptr && args.size() > 0 && args[0].isString()) {
                        juce::String deviceName = args[0].toString();
                        
                        // Setup Audio
                        juce::AudioDeviceManager::AudioDeviceSetup setup;
                        deviceManager->getAudioDeviceSetup(setup);
                        setup.inputDeviceName = deviceName;
                        setup.outputDeviceName = deviceName; // Usually input/output are same driver on macOS/CoreAudio
                        
                        deviceManager->setAudioDeviceSetup(setup, true);
                    }
                    sendDeviceList(); // Always refresh to sync UI
                    completion(juce::var());
                  })
              .withNativeFunction(
                  "setMidiInput",
                  [this](const juce::Array<juce::var> &args,
                         juce::WebBrowserComponent::NativeFunctionCompletion
                             completion) {
                    if (args.size() > 1 && args[0].isString()) {
                        juce::String identifier = args[0].toString();
                        
                        // Robust boolean extraction
                        bool enable = false;
                        if (args[1].isBool()) enable = (bool)args[1];
                        else if (args[1].isInt()) enable = (int)args[1] != 0;
                        else if (args[1].isDouble()) enable = (double)args[1] != 0.0;
                        else enable = args[1].toString().getIntValue() != 0;

                        // Find canonical identifier
                        auto available = juce::MidiInput::getAvailableDevices();
                        bool found = false;
                        for (auto& dev : available) {
                            if (dev.identifier == identifier) {
                                found = true;
                                
                                // Use StandalonePluginHolder's deviceManager if available (standalone mode)
                                if (auto* holder = juce::StandalonePluginHolder::getInstance()) {
                                    holder->deviceManager.setMidiInputDeviceEnabled(dev.identifier, enable);
                                    bool actualState = holder->deviceManager.isMidiInputDeviceEnabled(dev.identifier);
                                    lastMidiDebugLog = "Set " + identifier.substring(0,8) + " -> " + (enable ? "ON" : "OFF") + " (Standalone) Res:" + (actualState ? "ON" : "OFF");
                                }
                                // Fallback to deviceManager (plugin mode)
                                else if (deviceManager != nullptr) {
                                    deviceManager->setMidiInputDeviceEnabled(dev.identifier, enable);
                                    bool actualState = deviceManager->isMidiInputDeviceEnabled(dev.identifier);
                                    lastMidiDebugLog = "Set " + identifier.substring(0,8) + " -> " + (enable ? "ON" : "OFF") + " (Plugin) Res:" + (actualState ? "ON" : "OFF");
                                }
                                break;
                            }
                        }
                        if (!found) {
                            lastMidiDebugLog = "Dev Not Found: " + identifier;
                        }
                    } else {
                        lastMidiDebugLog = "Invalid Args";
                    }

                    sendDeviceList(); // Always refresh
                    completion(juce::var());
                  })
              .withNativeFunction(
                  "setAudioInputChannel",
                  [this](const juce::Array<juce::var> &args,
                         juce::WebBrowserComponent::NativeFunctionCompletion
                             completion) {
                    if (deviceManager != nullptr && args.size() > 0 && args[0].isInt()) {
                        int channelIndex = (int)args[0];
                        auto setup = deviceManager->getAudioDeviceSetup();
                        
                        juce::BigInteger inputMask;
                        inputMask.setBit(channelIndex);
                        
                        setup.inputChannels = inputMask;
                        setup.useDefaultInputChannels = false;
                        
                        juce::String error = deviceManager->setAudioDeviceSetup(setup, true);
                        if (error.isNotEmpty()) {
                             lastMidiDebugLog = "Audio Set Err: " + error;
                        } else {
                             lastMidiDebugLog = "Set Audio Ch " + juce::String(channelIndex) + " (OK)";
                        }
                    } else {
                        lastMidiDebugLog = "Audio Args Err";
                    }

                    sendDeviceList();
                    completion(juce::var());
                  })
              .withNativeFunction(
                  "loadSample",
                  [this](const juce::Array<juce::var> &args,
                         juce::WebBrowserComponent::NativeFunctionCompletion
                             completion) {
                    if (args.size() > 0 && args[0].isString()) {
                        audioProcessor.loadBuiltInSample(args[0].toString());
                    }
                    completion(juce::var());
                  })
              .withNativeFunction(
                  "openBluetoothPairingDialog",
                  [this](const juce::Array<juce::var> &args,
                         juce::WebBrowserComponent::NativeFunctionCompletion
                             completion) {
                    ignoreUnused(args);
                    printf("Native: openBluetoothPairingDialog called.\n");
                    
                    #if JUCE_IOS
                    if (juce::BluetoothMidiDevicePairingDialogue::isAvailable())
                        juce::BluetoothMidiDevicePairingDialogue::open();
                    #elif JUCE_ANDROID
                    if (juce::BluetoothMidiDevicePairingDialogue::isAvailable())
                        juce::BluetoothMidiDevicePairingDialogue::open();
                    #endif
                    
                    completion(juce::var());
                  }))
{
    // --- Research Step 1: Programmatic Unmute ---
    if (juce::JUCEApplicationBase::isStandaloneApp())
    {
        if (auto* pluginHolder = juce::StandalonePluginHolder::getInstance())
        {
            pluginHolder->getMuteInputValue().setValue(false); 
        }
    }
  addAndMakeVisible(webView);

  setResizable(true, true);
  setSize(768, 1024);

  // Load from local ResourceProvider using callAsync to ensure the component is initialized
  // This helps prevent white screen issues on startup, especially on iOS
  // juce::Component::SafePointer<AmenBreakChopperAudioProcessorEditor> safeThis(this);
  // juce::MessageManager::callAsync([safeThis] {
  //   if (safeThis != nullptr) {
  //     // #if JUCE_DEBUG && !JUCE_IOS
  //     //   safeThis->webView.goToURL("http://localhost:5173");
  //     // #else
  //     safeThis->webView.goToURL(juce::WebBrowserComponent::getResourceProviderRoot());
  //     // #endif
  //   }
  // });

  // Initialize parameter cache
  for (auto *param : audioProcessor.getParameters()) {
    if (auto *p = dynamic_cast<juce::AudioProcessorParameterWithID *>(param)) {
      float val = p->getValue();
      // Try to convert to world value if possible
      if (auto *rp = dynamic_cast<juce::RangedAudioParameter *>(param)) {
        val = rp->convertFrom0to1(val);
      }
      lastParameterValues[p->paramID] = val;
    }
  }

  // --- UI Event Bridge ---
  // Forward MIDI events from Processor to WebView (Thread-Safe)
  audioProcessor.onNoteEvent = [this](int note1, int note2) {
    juce::MessageManager::callAsync([this, note1, note2] {
      // WebView might be null or deleted if editor is closing, but 'this' is
      // captured safely? Editor owns WebView, so if 'this' is alive, WebView is
      // alive. NoteEvent is bound to 'this', so we need to clear it in
      // destructor to avoid dangling pointer callAsync? Actually, callAsync
      // captures 'this'. If Editor is deleted, the lambda might still run? We
      // should use SafePointer or WeakReference if strict, but
      // Component::BailOutChecker is built-in for some things. For now, simple
      // check:
      if (isWebViewLoaded) {
        juce::String js = "if (typeof window.juce_emitEvent === 'function') { "
                          "window.juce_emitEvent('note', { note1: " +
                          juce::String(note1) +
                          ", note2: " + juce::String(note2) + " }); }";
        webView.evaluateJavascript(js);
      }
    });
  };

  startTimerHz(30); // Start 30Hz polling for waveform updates
}

AmenBreakChopperAudioProcessorEditor::~AmenBreakChopperAudioProcessorEditor() {
  stopTimer();
}

void AmenBreakChopperAudioProcessorEditor::paint(juce::Graphics &g) {
  g.fillAll(juce::Colours::black);
}

void AmenBreakChopperAudioProcessorEditor::timerCallback() {
  if (!isWebViewLoaded) {
    if (isShowing() && getWidth() > 0 && getHeight() > 0) {
      if (framesWaited++ > 5) {
        webView.goToURL(juce::WebBrowserComponent::getResourceProviderRoot());
        isWebViewLoaded = true;
        retryCounter = 0;
      }
    } else {
      framesWaited = 0;
    }
    return;
  }

  // Retry logic if frontend fails to connect
  if (isWebViewLoaded && !hasFrontendConnected) {
    if (retryCounter++ > 90) { // ~3 seconds at 30Hz
      juce::Logger::writeToLog(
          "Frontend connection timed out. Reloading WebView...");
      webView.goToURL(juce::WebBrowserComponent::getResourceProviderRoot());
      retryCounter = 0;
    }
  }

  // Poll for parameter changes
  if (isWebViewLoaded) {
    for (auto *param : audioProcessor.getParameters()) {
      if (auto *p = dynamic_cast<juce::AudioProcessorParameterWithID *>(param)) {
        float val = p->getValue();
        if (auto *rp = dynamic_cast<juce::RangedAudioParameter *>(param)) {
          val = rp->convertFrom0to1(val);
        }

        // Check against cache
        if (lastParameterValues.find(p->paramID) == lastParameterValues.end() ||
            std::abs(lastParameterValues[p->paramID] - val) > 0.0001f) {

          lastParameterValues[p->paramID] = val;
          sendParameterUpdate(p->paramID, val);
        }
      }
    }
  }

  if (audioProcessor.mWaveformDirty.exchange(false)) {
      std::vector<float> waveform = audioProcessor.getWaveformData();
      int currentSeqPos = audioProcessor.getSequencePosition(); // Use public getter
      
      // Manually construct JSON string for speed/simplicity or use JUCE JSON
      // We need to send an array of 512 floats.
      juce::DynamicObject* obj = new juce::DynamicObject();
      juce::Array<juce::var> dataArray;
      dataArray.ensureStorageAllocated(static_cast<int>(waveform.size()));
      
      for (float sample : waveform) {
          dataArray.add(sample);
      }
      
      obj->setProperty("data", dataArray);
      obj->setProperty("currentSeqPos", currentSeqPos); // Send synchronised position
      
      juce::String js = "if (typeof window.juce_emitEvent === 'function') { "
                        "window.juce_emitEvent('waveform', " + juce::JSON::toString(juce::var(obj)) + "); }";
      
      if (isWebViewLoaded) {
          webView.evaluateJavascript(js);
      }
  }
}

void AmenBreakChopperAudioProcessorEditor::resized() {
  auto bounds = getLocalBounds();

#if JUCE_IOS
  // "Edge-to-Edge" Logic:
  // Since ComponentPeer::getSafeAreaInsets() is missing in this JUCE version,
  // we calculate the safe area by intersecting our Screen Bounds with the 
  // Display's userArea (which excludes the notch/home bar).
  auto& displays = juce::Desktop::getInstance().getDisplays();
  auto display = displays.getPrimaryDisplay(); // validated as existing via StandaloneApp.cpp usage
  
  auto safeArea = display->userArea;
  auto screenBounds = getScreenBounds();

  if (!screenBounds.isEmpty()) {
      auto safeIntersection = screenBounds.getIntersection(safeArea);
      // Convert the safe intersection rect (Screen Coords) into Local Coords
      bounds = getLocalArea(nullptr, safeIntersection);
  }
#endif

  webView.setBounds(bounds);
}

void AmenBreakChopperAudioProcessorEditor::sendParameterUpdate(
    const juce::String &paramId, float newValue) {
  // Directly evaluate since we are already on the Message Thread (TimerCallback)
  // or inside a callAsync block.
  // However, sendParameterUpdate might be called from syncAllParametersToFrontend which might be called from JS callback.
  // JS callbacks run on message thread. Timer runs on message thread.
  // So we don't need callAsync if called from these contexts.

  // But wait, syncAllParametersToFrontend is called from requestInitialState which is a native function.
  // Native functions are called on the Message Thread.

  juce::DynamicObject *obj = new juce::DynamicObject();
  obj->setProperty("id", paramId);
  obj->setProperty("value", newValue);

  juce::String js = "if (typeof window.juce_updateParameter === 'function') "
                    "{ window.juce_updateParameter(" +
                    juce::JSON::toString(juce::var(obj)) + "); }";

  if (isWebViewLoaded) {
      webView.evaluateJavascript(js);
  }
}

void AmenBreakChopperAudioProcessorEditor::syncAllParametersToFrontend() {
  for (auto *param : audioProcessor.getParameters()) {
    if (auto *p = dynamic_cast<juce::AudioProcessorParameterWithID *>(param)) {
      float val = p->getValue();
      // Try to convert to world value if possible
      if (auto *rp = dynamic_cast<juce::RangedAudioParameter *>(param)) {
        val = rp->convertFrom0to1(val);
      }
      sendParameterUpdate(p->paramID, val);
    }
  }
}

void AmenBreakChopperAudioProcessorEditor::sendDeviceList() {
    if (deviceManager == nullptr) return; // Not in standalone or not linked
    
    juce::DynamicObject* root = new juce::DynamicObject();
    
    // --- Audio Devices ---
    juce::DynamicObject* audioObj = new juce::DynamicObject();
    root->setProperty("audio", audioObj);
    
    // Current Device Info
    auto* currentDevice = deviceManager->getCurrentAudioDevice();
    if (currentDevice != nullptr) {
        audioObj->setProperty("currentDevice", currentDevice->getName());
        audioObj->setProperty("inputChannels", currentDevice->getInputChannelNames());
    }
    
    // Available Device Names (CoreAudio usually)
    juce::StringArray devNames;
    juce::String currentTypeName = deviceManager->getCurrentAudioDeviceType();
    
    // Find the actual type object to get the device list
    for (auto* type : deviceManager->getAvailableDeviceTypes()) {
        if (type->getTypeName() == currentTypeName) {
            devNames = type->getDeviceNames();
            break;
        }
    }
    
    // Also add other types? For now just current type (CoreAudio) is fine for Mac Standalone
    audioObj->setProperty("availableDevices", devNames);
    
    // --- MIDI Devices ---
    juce::Array<juce::var> midiInputs;
    auto availableInputs = juce::MidiInput::getAvailableDevices();
    for (auto& info : availableInputs) {
        juce::DynamicObject* midiObj = new juce::DynamicObject();
        midiObj->setProperty("name", info.name);
        midiObj->setProperty("id", info.identifier);
        
        bool isEnabled = deviceManager->isMidiInputDeviceEnabled(info.identifier);
        midiObj->setProperty("enabled", isEnabled);
        
        midiInputs.add(midiObj);
    }
    root->setProperty("midiInputs", midiInputs);

    // --- Active Input Channel Names ---
    juce::Array<juce::var> inputChannelNames;
    if (deviceManager != nullptr) {
        auto* device = deviceManager->getCurrentAudioDevice();
        if (device != nullptr) {
            auto activeChannels = device->getActiveInputChannels();
            auto channelNames = device->getInputChannelNames();
            for (int i = 0; i < activeChannels.getHighestBit() + 1; ++i) {
               // Show all potential channels for the device, or just active?
               // The MD suggests showing all so user can pick.
               // Let's iterate up to channelNames.size() actually, or just offer all input channels supported.
               // Actually, we want to show *Available* channels to select.
               // But 'activeInputNames' was showing *currently active*.
               // Let's change usage: We want to lists 'availableInputChannels' with their index.
               if (i < channelNames.size()) {
                   juce::DynamicObject* chan = new juce::DynamicObject();
                   chan->setProperty("name", channelNames[i]);
                   chan->setProperty("index", i);
                   chan->setProperty("active", activeChannels[i]);
                   inputChannelNames.add(chan);
               } else {
                   // Fallback for unnamed channels if any
                   juce::DynamicObject* chan = new juce::DynamicObject();
                   chan->setProperty("name", "Ch " + juce::String(i + 1));
                   chan->setProperty("index", i);
                   chan->setProperty("active", activeChannels[i]);
                   inputChannelNames.add(chan);
               }
            }
        }
    }
    audioObj->setProperty("inputChannelsList", inputChannelNames);

    // --- Debug Info ---
    juce::String debugInfo;
    if (deviceManager == nullptr) debugInfo << "DM: NULL";
    else {
        debugInfo << "DM: OK. ";
        int enabledCount = 0;
        for (auto& info : availableInputs) {
            bool en = deviceManager->isMidiInputDeviceEnabled(info.identifier);
            if (en) enabledCount++;
            debugInfo << "[" << info.name.substring(0, 5) << ":" << (en ? "ON" : "OFF") << "] ";
        }
        debugInfo << "Total: " << availableInputs.size();
        debugInfo << " | LastLog: " << lastMidiDebugLog;
    }
    root->setProperty("debugInfo", debugInfo);


    juce::String js = "if (typeof window.juce_emitEvent === 'function') { "
                      "window.juce_emitEvent('deviceList', " + juce::JSON::toString(juce::var(root)) + "); }";
                      
    if (isWebViewLoaded) {
        webView.evaluateJavascript(js);
    }
}