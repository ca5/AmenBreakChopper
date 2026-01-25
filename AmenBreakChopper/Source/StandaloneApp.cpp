/*
  ==============================================================================

    StandaloneApp.cpp
    Part of AmenBreakChopper
    
    Implements a custom Standalone Application to fully control the UI
    and avoid the mandatory "Audio input is muted" warning bar provided
    by the default JUCE StandaloneFilterWindow.

  ==============================================================================
*/

#include <JuceHeader.h>
#include "PluginEditor.h"
#include <stdio.h> // For printf

#if JUCE_USE_CUSTOM_PLUGIN_STANDALONE_APP
// #error "CONFIRMED_CUSTOM_APP_IS_ACTIVE" // VERIFIED: Macro is active

#include <juce_audio_plugin_client/Standalone/juce_StandaloneFilterWindow.h>

//==============================================================================
//==============================================================================
class CustomLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawDocumentWindowTitleBar (juce::DocumentWindow& window, juce::Graphics& g,
                                     int w, int h, int titleSpaceX, int titleSpaceW,
                                     const juce::Image* icon, bool drawTitleTextOnLeft) override
    {
        g.fillAll (juce::Colours::black);
    }
};

class CustomStandaloneWindow : public juce::DocumentWindow,
                               public juce::Button::Listener
{
public:
    CustomStandaloneWindow (const juce::String& name,
                            juce::Colour backgroundColour,
                            juce::PropertySet* settingsToUse)
#if JUCE_IOS
        : DocumentWindow (name, backgroundColour, 0)
#else
        : DocumentWindow (name, backgroundColour, DocumentWindow::allButtons)
#endif
    {
        DBG("[ABC-Standalone] CustomStandaloneWindow constructor starting");
#if JUCE_IOS
        // Disable native title bar
        setUsingNativeTitleBar (false);
        setTitleBarHeight (60); 
        
        // Force title bar background to be explicitly black
        setBackgroundColour(juce::Colours::black);
        setColour(juce::ResizableWindow::backgroundColourId, juce::Colours::black);
        
        setFullScreen (true); // Force Fullscreen
        // Ensure bounds cover the screen
        if (auto* display = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay())
            setBounds (display->totalArea);
#else
        setUsingNativeTitleBar (true);
#endif
        
        // 1. Initialize the Plugin Holder (The Engine)
        DBG("[ABC-Standalone] About to create StandalonePluginHolder");
        // getInstance() might return null if we haven't created it yet. 
        // We take ownership.
        pluginHolder.reset (new juce::StandalonePluginHolder (settingsToUse));

        DBG("[ABC-Standalone] StandalonePluginHolder created successfully");

        // 2. Programmatically force MUTE (Critical Fix)
        // Setting to FALSE enables hardware monitoring (input passthrough)
        // Setting to TRUE forces audio through processBlock
        pluginHolder->getMuteInputValue().setValue(true);
        
        // 3. Force Audio Device Setup (iOS fix)
        // Ensure we request Input AND Output channels.
        // StandalonePluginHolder defaults can be weird on iOS.
        auto desc = juce::AudioDeviceManager::AudioDeviceSetup();
        
        // Get current setup or defaults
        pluginHolder->deviceManager.getAudioDeviceSetup(desc);
        
        desc.inputChannels.setBit(0); // Enable Input 1
        desc.inputChannels.setBit(1); // Enable Input 2
        desc.outputChannels.setBit(0); // Enable Output 1
        desc.outputChannels.setBit(1); // Enable Output 2
        
        // Error handling omitted for brevity, but this forces channel count request
        juce::String err = pluginHolder->deviceManager.initialise(2, 2, nullptr, true, 
                                                                  juce::String(), &desc);
        DBG("[ABC-Standalone] Device Init Result: " << err);

        // 3. Create and show the Plugin Editor
        if (auto* processor = pluginHolder->processor.get())
        {
            if (auto* editor = processor->createEditor())
            {
                // We use a container to hold the editor
                // Inject DeviceManager Reference if it's our specific editor type
                if (auto* myEditor = dynamic_cast<AmenBreakChopperAudioProcessorEditor*>(editor))
                {
                    myEditor->setDeviceManager(&pluginHolder->deviceManager);
                }
                
                // Set editor as content
#if JUCE_IOS
                // On iOS, we want the Editor to Resize to fit the Fullscreen Window
                setContentOwned(editor, false);
#else
                // On Desktop, resize window to fit the Editor's fixed size
                setContentOwned(editor, true);
#endif
            }
        }

        setResizable (false, false);
        
        // Restore window state
#if !JUCE_IOS
        if (settingsToUse != nullptr)
            restoreWindowStateFromString (settingsToUse->getValue ("windowState"));
#endif
        
        setVisible (true);
    }

    ~CustomStandaloneWindow() override
    {
        // Save window state
        if (auto* props = juce::StandalonePluginHolder::getInstance()->settings.get())
            props->setValue ("windowState", getWindowStateAsString());
            
        setContentOwned(nullptr, true); // Explicitly delete editor first
        pluginHolder = nullptr;  // shut down audio
    }

    void closeButtonPressed() override
    {
        juce::JUCEApplication::getInstance()->systemRequestedQuit();
    }
    
    void buttonClicked (juce::Button* b) override {}

private:
    std::unique_ptr<juce::StandalonePluginHolder> pluginHolder;
};


//==============================================================================
class AmenBreakChopperStandaloneApp : public juce::JUCEApplication
{
public:
    AmenBreakChopperStandaloneApp() {}

    const juce::String getApplicationName() override              { return JucePlugin_Name; }
    const juce::String getApplicationVersion() override           { return JucePlugin_VersionString; }
    bool moreThanOneInstanceAllowed() override                    { return true; }
    void anotherInstanceStarted (const juce::String&) override    {}

    void initialise (const juce::String&) override
    {
        // Setup settings file
        juce::PropertiesFile::Options options;
        options.applicationName     = getApplicationName();
        options.filenameSuffix      = ".settings";
        options.osxLibrarySubFolder = "Application Support";
        options.folderName          = getApplicationName();
        options.storageFormat       = juce::PropertiesFile::storeAsXML;

        settings.setStorageParameters (options);
        
        // Force muteAudioInput to TRUE in property set (CRITICAL FIX)
        // This disables hardware monitoring and forces audio through processBlock
        if (auto* props = settings.getUserSettings())
        {
            props->setValue("muteAudioInput", true);
            // props->setShouldSave(true);
        }

        // Create customized window that DOES NOT have the warning logic
        mainWindow.reset (new CustomStandaloneWindow (getApplicationName(),
                                                      juce::Colours::black,
                                                      settings.getUserSettings()));
        
        // Apply custom look and feel to ensure black title bar
        mainWindow->setLookAndFeel(&customLookAndFeel);
    }

    void shutdown() override
    {
        mainWindow = nullptr;
    }

    void systemRequestedQuit() override
    {
        quit();
    }

    void suspended() override {}
    void resumed() override {}

private:
    juce::ApplicationProperties settings;
    CustomLookAndFeel customLookAndFeel; // Keep instance alive
    std::unique_ptr<CustomStandaloneWindow> mainWindow;
};

#endif // JUCE_USE_CUSTOM_PLUGIN_STANDALONE_APP

// CRITICAL: This must be OUTSIDE the #if block to always be active
#if JUCE_USE_CUSTOM_PLUGIN_STANDALONE_APP
START_JUCE_APPLICATION (AmenBreakChopperStandaloneApp)
#endif
