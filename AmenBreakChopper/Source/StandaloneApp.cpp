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

#if JUCE_USE_CUSTOM_PLUGIN_STANDALONE_APP

#include <juce_audio_plugin_client/Standalone/juce_StandaloneFilterWindow.h>

//==============================================================================
class CustomStandaloneWindow : public juce::DocumentWindow,
                               public juce::Button::Listener
{
public:
    CustomStandaloneWindow (const juce::String& name,
                            juce::Colour backgroundColour,
                            juce::PropertySet* settingsToUse)
        : DocumentWindow (name, backgroundColour, DocumentWindow::allButtons)
    {
        setUsingNativeTitleBar (true);
        
        // 1. Initialize the Plugin Holder (The Engine)
        // getInstance() might return null if we haven't created it yet. 
        // We take ownership.
        pluginHolder.reset (new juce::StandalonePluginHolder (settingsToUse));

        // 2. Programmatically force Unmute (Step 1 of Research)
        pluginHolder->getMuteInputValue().setValue(false);
        
        // 3. Create and show the Plugin Editor
        if (auto* processor = pluginHolder->getProcessor())
        {
            // Reset logic or state if needed
            // processor->programmaticReset(); // Example if needed

            if (auto* editor = processor->createEditor())
            {
                // We use a container to hold the editor + specific standalone controls (like Settings)
                auto* container = new juce::Component();
                container->addAndMakeVisible(editor);
                
                // Add a permanent "Audio Settings" button overlay or ensure the Editor has one.
                // For now, we'll put the editor as the main content and rely on the
                // native menu bar or a keyboard shortcut, OR we add a small button.
                // Let's add a small overlay button for "Settings" in the container.
                settingsButton.setButtonText("Audio Settings");
                settingsButton.addListener(this);
                container->addAndMakeVisible(settingsButton);
                
                // Layout logic for container
                // We need a simple ResizableLayout or just resized()
                // Let's use a lambda or simple component subclass for the container would be cleaner,
                // but for a single file, we can just do setContentOwned with a clever customized component.
                // Actually, let's just set the Editor as content and rely on a Title Bar button?
                // JUCE Native title bars don't easily allow custom buttons.
                
                // Let's stick to the simplest robust approach:
                // Editor is the main content. We assume the user can access settings via the Editor's UI
                // IF the editor serves that function. But `ControlPanel.tsx` is React.
                // So we NEED a way to open Audio Settings.
                // We'll revert to holding the editor directly, but maybe add a KeyListener or Menu?
                // Better: Create a MainComponent that holds Editor + Button.
                delete container; // scrap previous thought
                
                mainComponent.reset(new MainComponent(editor, *this));
                setContentNonOwned(mainComponent.get(), true);
            }
        }

        setResizable (true, true);
        
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
            
        mainComponent = nullptr; // delete content before holder
        pluginHolder = nullptr;  // shut down audio
    }

    void closeButtonPressed() override
    {
        juce::JUCEApplication::getInstance()->systemRequestedQuit();
    }
    
    void buttonClicked (juce::Button* b) override
    {
        if (b == &settingsButton)
        {
            pluginHolder->showAudioSettingsDialog();
        }
    }
    
    void showAudioSettings()
    {
        pluginHolder->showAudioSettingsDialog();
    }

private:
    // A simple container to hold Editor + Settings Button
    class MainComponent : public juce::Component
    {
    public:
        MainComponent(juce::AudioProcessorEditor* editorToOwn, CustomStandaloneWindow& w)
            : editor(editorToOwn), owner(w)
        {
            addAndMakeVisible(editor);
            
            settingsBtn.setButtonText("⚙"); // Gear icon approximation
            settingsBtn.setTooltip("Audio Device Settings");
            settingsBtn.onClick = [this] { owner.showAudioSettings(); };
            addAndMakeVisible(settingsBtn);
            
            // Match editor size initially
            setSize(editor->getWidth(), editor->getHeight());
        }
        
        void resized() override
        {
            if (editor)
                editor->setBounds(getLocalBounds());
            
            // Floating Settings Button (Top Right)
            settingsBtn.setBounds(getWidth() - 30, 5, 25, 25);
        }
        
    private:
        std::unique_ptr<juce::AudioProcessorEditor> editor;
        CustomStandaloneWindow& owner;
        juce::TextButton settingsBtn;
    };

    std::unique_ptr<juce::StandalonePluginHolder> pluginHolder;
    std::unique_ptr<MainComponent> mainComponent;
    juce::TextButton settingsButton; // (Unused in new design, kept in class for legacy)
};


//==============================================================================
class AmenBreakChopperStandaloneApp : public juce::JUCEApplication
{
public:
    AmenBreakChopperStandaloneApp() {}

    const juce::String getApplicationName() override              { return JucePlugin_Name; }
    const juce::String getApplicationVersion() override           { return JucePlugin_Version; }
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
            props->setShouldSave(true);
        }

        // Create customized window that DOES NOT have the warning logic
        mainWindow.reset (new CustomStandaloneWindow (getApplicationName(),
                                                      juce::Colours::black,
                                                      settings.getUserSettings()));
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
    std::unique_ptr<CustomStandaloneWindow> mainWindow;
};

START_JUCE_APPLICATION (AmenBreakChopperStandaloneApp)

#endif
