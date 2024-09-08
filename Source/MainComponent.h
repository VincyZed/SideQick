#pragma once

#include <JuceHeader.h>
#include "MidiSysexProcessor.h"
#include "Display.h"
#include "PannelButton.h"
#include "Logo.h"

using namespace juce;

class PlasticTexture : public Component {
public:
    PlasticTexture(const Image& textureImage);

    void paint(Graphics& g) override;
    void resized() override;

private:
    juce::Image textureImage;
    std::unique_ptr<PlasticTexture> overlayComponent;
};

class MainComponent  : public AudioAppComponent, public MidiInputCallback, public Timer
{
public:
    //==============================================================================
    MainComponent();
    ~MainComponent() override;
    // No notification
    NotificationType NO = NotificationType::dontSendNotification;

    std::unique_ptr<DisplayLookAndFeel> lookAndFeel;

    Logo logo;
    
    Font descriptionTextFont = Font(FontOptions("Arial", 16.0f, Font::italic));
    Colour descriptionTextColour = Colour::fromRGB(170, 170, 170);

    Font sectionsTextFont = Font(FontOptions("Arial", 16.0f, Font::bold));
    Colour sectionsTextColour = Colour::fromRGB(255, 255, 225);

    Font menusTextFont = Font(FontOptions("Arial", 16.0f, Font::plain));
    Colour menusTextColour = Colour::fromRGB(255, 255, 240);

    // Colours depending on the currently connected model. First is SQ-80, second is ESQ-1, then unknown.
    const Colour backgroundColours[3][2] = {
        { Colour::fromRGB(110, 110, 115), Colour::fromRGB(60, 60, 65) },
        { Colour::fromRGB(80, 80, 85), Colour::fromRGB(50, 50, 55) },
        { Colour::fromRGB(90, 90, 95), Colour::fromRGB(55, 55, 60) }
    };
    const Colour accentColours[3] = { Colour::fromRGB(150, 0, 0), Colour::fromRGB(0, 150, 175), Colour::fromRGB(175, 175, 175) };
    const Colour refreshButtonColours [3] = { Colour::fromRGB(0, 180, 180), Colour::fromRGB(200, 150, 0), Colour::fromRGB(175, 175, 175) };
    const Colour importButtonColours [3] = { Colour::fromRGB(127, 0, 55), Colour::fromRGB(30, 30, 30), Colour::fromRGB(127, 0, 55) };
    const Colour exportButtonColours [3] = { Colour::fromRGB(175, 175, 175), Colour::fromRGB(175, 175, 175), Colour::fromRGB(175, 175, 175) };
    
    const float separatorThickness = 10.0f;

    enum ThemeOptions { AUTOMATIC_THEME, SQ80_THEME, ESQ1_THEME, NEUTRAL_THEME };
    unsigned int selectedThemeOption = AUTOMATIC_THEME;

    MidiSysexProcessor midiProcessor;
    void handleIncomingMidiMessage(MidiInput* source, const MidiMessage& message) override;
    void updateStatus(DeviceResponse response);
    void attemptConnection();
    void refreshMidiDevices(bool allowMenuSwitch = false);
    void updateTheme();
    void timerCallback() override;
    SynthModel getCurrentSynthModel() const;
    //==============================================================================
    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override;
    void getNextAudioBlock (const AudioSourceChannelInfo& bufferToFill) override;
    void releaseResources() override;

    //==============================================================================
    void paint (Graphics& g) override;
    void resized() override;

    SynthModel currentModel;

    enum Oscillators { OSC1, OSC2, OSC3 };

    //Array<MidiDeviceInfo> midiInDevices;
    //Array<MidiDeviceInfo> midiOutDevices;

    StringArray midiInDeviceNames;
    StringArray midiOutDeviceNames;

    Label descriptionLabel;

    Label midiSectionLabel;


    Label midiInLabel;
    Label midiOutLabel;

    ComboBox midiInMenu;
    ComboBox midiOutMenu;

    Label programTitleLabel;
    Label programNameLabel;

    Label statusTitleLabel;
    Label statusLabel;
    Label modelLabel;
    unsigned int modelLabelXPos = 650;
    Label disconnectedUnderline;
    Label sysexDisabledUnderline;

    PannelButton refreshButton;

    Label osc1Label;
    Label osc2Label;
    Label osc3Label;
    Label waveLabel;
    Label octLabel;
    Label semiLabel;
    Label LFLabel;
    Label selfOscLabel;


    Display display;

    GroupComponent midiControls;
    
    // Contains the programControls and the labels
    GroupComponent programSection;
    GroupComponent programControls;
    // The top row of the display
    GroupComponent statusSection;


    StringArray waveMenuOpts;
    ComboBox osc1WaveMenu;
    ComboBox osc2WaveMenu;
    ComboBox osc3WaveMenu;
    ComboBox osc1OctMenu;
    ComboBox osc1SemiMenu;
    ComboBox osc2OctMenu;
    ComboBox osc2SemiMenu;
    ComboBox osc3OctMenu;
    ComboBox osc3SemiMenu;
    ToggleButton osc1LFButton;
    ToggleButton osc2LFButton;
    ToggleButton osc3LFButton;

    ToggleButton selfOscButton;

    void mouseDown(const juce::MouseEvent& event) override;


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)

private:

    const StringArray ignoredMidiDevices = { "Microsoft GS Wavetable Synth" };
    TooltipWindow tooltipWindow;

    std::unique_ptr<PlasticTexture> textureOverlay;

    void showContextMenu();

    unsigned int windowWidth = 830;
    unsigned int windowHeight = 410;

    unsigned int displayWidth = 780;
    unsigned int displayHeight = 200;
};

