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

                    if (resourceFile.existsAsFile()) {
                      juce::Logger::writeToLog("Found resource: " +
                                               resourceFile.getFullPathName());

                      auto extension = resourceFile.getFileExtension();
                      juce::String mimeType = "application/octet-stream";

                      if (extension == ".html")
                        mimeType = "text/html";
                      else if (extension == ".js")
                        mimeType = "application/javascript";
                      else if (extension == ".css")
                        mimeType = "text/css";
                      else if (extension == ".png")
                        mimeType = "image/png";
                      else if (extension == ".svg")
                        mimeType = "image/svg+xml";
                      else if (extension == ".json")
                        mimeType = "application/json";

                      juce::MemoryBlock mb;
                      resourceFile.loadFileAsData(mb);

                      std::vector<std::byte> data(mb.getSize());
                      memcpy(data.data(), mb.getData(), mb.getSize());

                      return juce::WebBrowserComponent::Resource{
                          std::move(data), mimeType};
                    } else {
                      juce::Logger::writeToLog("Resource NOT found for: " +
                                               resourcePath);
                      for (const auto &c : candidates)
                        juce::Logger::writeToLog("Checked: " +
                                                 c.getFullPathName());
                    }

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

                      if (auto *param =
                              audioProcessor.getValueTreeState().getParameter(
                                  paramId)) {
                        // For now, assume value is normalized [0, 1] from UI
                        param->setValueNotifyingHost(value);
                      }
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
  webView.goToURL(juce::WebBrowserComponent::getResourceProviderRoot());

  auto &params = audioProcessor.getValueTreeState();
  for (auto *param : audioProcessor.getParameters()) {
    if (auto *p = dynamic_cast<juce::AudioProcessorParameterWithID *>(param)) {
      params.addParameterListener(p->paramID, this);
    }
  }
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

    juce::String js =
        "if (window.juce_updateParameter) window.juce_updateParameter(" +
        juce::JSON::toString(juce::var(obj)) + ");";
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