/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginEditor.h"
#include "PluginProcessor.h"

//==============================================================================
AmenBreakChopperAudioProcessorEditor::AmenBreakChopperAudioProcessorEditor(
    AmenBreakChopperAudioProcessor &p)
    : AudioProcessorEditor(&p), audioProcessor(p),
      webView(
          juce::WebBrowserComponent::Options()
#if JUCE_WINDOWS
              .withBackend(
                  juce::WebBrowserComponent::Options::Backend::webview2)
              .withWinWebView2Options(
                  juce::WebBrowserComponent::Options::WinWebView2Options()
                      .withUserDataFolder(juce::File::getSpecialLocation(
                          juce::File::tempDirectory)))
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
                    syncAllParametersToFrontend();
                    completion(juce::var());
                  })) {
  addAndMakeVisible(webView);

  setSize(800, 600);

  // Load from local ResourceProvider
#if JUCE_DEBUG
  webView.goToURL("http://localhost:5173");
#else
  webView.goToURL(juce::WebBrowserComponent::getResourceProviderRoot());
#endif

  auto &params = audioProcessor.getValueTreeState();
  for (auto *param : audioProcessor.getParameters()) {
    if (auto *p = dynamic_cast<juce::AudioProcessorParameterWithID *>(param)) {
      params.addParameterListener(p->paramID, this);
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
      if (webView.isVisible()) {
        juce::String js = "if (typeof window.juce_emitEvent === 'function') { "
                          "window.juce_emitEvent('note', { note1: " +
                          juce::String(note1) +
                          ", note2: " + juce::String(note2) + " }); }";
        webView.evaluateJavascript(js);
      }
    });
  };
}

AmenBreakChopperAudioProcessorEditor::~AmenBreakChopperAudioProcessorEditor() {
  auto &params = audioProcessor.getValueTreeState();
  for (auto *param : audioProcessor.getParameters()) {
    if (auto *p = dynamic_cast<juce::AudioProcessorParameterWithID *>(param)) {
      params.removeParameterListener(p->paramID, this);
    }
  }
}

void AmenBreakChopperAudioProcessorEditor::paint(juce::Graphics &g) {
  g.fillAll(juce::Colours::black);
}

void AmenBreakChopperAudioProcessorEditor::resized() {
  webView.setBounds(getLocalBounds());
}

void AmenBreakChopperAudioProcessorEditor::parameterChanged(
    const juce::String &parameterID, float newValue) {
  sendParameterUpdate(parameterID, newValue);
}

void AmenBreakChopperAudioProcessorEditor::sendParameterUpdate(
    const juce::String &paramId, float newValue) {
  juce::MessageManager::callAsync([this, paramId, newValue]() {
    juce::DynamicObject *obj = new juce::DynamicObject();
    obj->setProperty("id", paramId);
    obj->setProperty("value", newValue);

    juce::String js = "if (typeof window.juce_updateParameter === 'function') "
                      "{ window.juce_updateParameter(" +
                      juce::JSON::toString(juce::var(obj)) + "); }";
    webView.evaluateJavascript(js);
  });
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