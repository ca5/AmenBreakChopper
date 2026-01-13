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
// #error "CONFIRMED_CUSTOM_APP_IS_ACTIVE" // PROBE REMOVED

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
        : DocumentWindow (name, backgroundColour, DocumentWindow::allButtons)
    {
#if JUCE_IOS
        // Disable native title bar to avoid white system bars.
        // We will handle Safe Area manually in the Editor.
        setUsingNativeTitleBar (false);
        setTitleBarHeight (60); 
        
        // Force title bar background to be explicitly black
        setBackgroundColour(juce::Colours::black);
        setColour(juce::ResizableWindow::backgroundColourId, juce::Colours::black);
        
        setFullScreen (false);
        // Ensure bounds cover the screen
        if (auto* display = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay())
            setBounds (display->totalArea);
#else
        setUsingNativeTitleBar (true);
#endif
        
        // 1. Initialize the Plugin Holder (The Engine)
        // getInstance() might return null if we haven't created it yet. 
        // We take ownership.
        pluginHolder.reset (new juce::StandalonePluginHolder (settingsToUse));

        // 2. Programmatically force Unmute (Step 1 of Research)
        pluginHolder->getMuteInputValue().setValue(false);

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
                setContentOwned(editor, true);
            }
        }

        setResizable (false, false);
        
        // Restore window state
        if (settingsToUse != nullptr)
            restoreWindowStateFromString (settingsToUse->getValue ("windowState"));
        
        setVisible (true);
    }

    ~CustomStandaloneWindow() override
    {
        // Save window state
        if (auto* props = juce::StandalonePluginHolder::getInstance()->settings.get())
            props->setValue ("windowState", getWindowStateAsString());
            
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
        
        // Force muteAudioInput to false in property set as well
        if (auto* props = settings.getUserSettings())
        {
            props->setValue("muteAudioInput", false);
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

START_JUCE_APPLICATION (AmenBreakChopperStandaloneApp)

#endif
