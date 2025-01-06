/*
 * SideQick - SQ-80/ESQ-1 expansion software
 * Copyright Vincent Zauhar, 2024
 *
 * Released under the GNU General Public Licence v3
 * or later (GPL-3.0-or-later). The license is found in the "LICENSE"
 * file in the root of this repository, or at
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 *
 *
 * All source for SideQick is available at
 * https://github.com/VincyZed/SideQick
 */

#pragma once

#include "Display.h"
#include "Logo.h"
#include "MidiSysexProcessor.h"
#include "PannelButton.h"
#include <JuceHeader.h>

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

class MainComponent : public AudioAppComponent, public MidiInputCallback, public Timer {
  public:
    //==============================================================================
    MainComponent();
    ~MainComponent() override;


  private:
    void handleIncomingMidiMessage(MidiInput* source, const MidiMessage& message) override;
    void updateStatus(DeviceResponse response);
    void attemptConnection();
    void refreshMidiDevices(bool allowMenuSwitch = false);
    void timerCallback() override;
    SynthModel getCurrentSynthModel() const;
    //==============================================================================
    void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
    void getNextAudioBlock(const AudioSourceChannelInfo& bufferToFill) override;
    void releaseResources() override;

    //==============================================================================
    void paint(Graphics& g) override;
    void resized() override;

    void showContextMenu();
    void mouseDown(const juce::MouseEvent& event) override;

    void createLabel(Label& label, Component& parent, const String& text, const int x, const int y, const int width, const int height, const Colour& colour = Colour(),
                     const Font& font = Font(Font::getDefaultSansSerifFontName(), 16.0f, Font::plain));

    void createComboBox(ComboBox& comboBox, Component& parent, const int x, const int y, const int width, const int height, const String& tooltip,
                        const std::function<DeviceResponse()>& onChangeFunc, const StringArray& items = {});
    void displayControlOnChange(const std::function<DeviceResponse()>& onChangeFunc);

    void createToggleButton(ToggleButton& button, Component& parent, const int x, const int y, const int width, const int height, const String& tooltip,
                            const std::function<DeviceResponse()>& onClickFunc);

    unsigned int windowWidth = 830;
    unsigned int windowHeight = 410;

    unsigned int displayWidth = 780;
    unsigned int displayHeight = 200;

    NotificationType NO = NotificationType::dontSendNotification;

    std::unique_ptr<DisplayLookAndFeel> lookAndFeel;
    Logo logo;
    TooltipWindow tooltipWindow;
    std::unique_ptr<PlasticTexture> textureOverlay;

    enum Themes { AUTOMATIC_THEME, SQ80_THEME, ESQ1_THEME, NEUTRAL_THEME };
    const StringArray THEME_OPTIONS = {"Automatic", "SQ-80", "ESQ-1", "Neutral"};
    unsigned int selectedThemeOption = AUTOMATIC_THEME;

    MidiSysexProcessor midiProcessor;
    const StringArray ignoredMidiDevices = {"Microsoft GS Wavetable Synth"};


    SynthModel currentModel;
    enum Oscillators { OSC1, OSC2, OSC3 };


    Font descriptionTextFont = Font(Font::getDefaultSansSerifFontName(), 16.0f, Font::italic);
    Colour descriptionTextColour = Colour::fromRGB(170, 170, 170);

    Font sectionsTextFont = Font(Font::getDefaultSansSerifFontName(), 16.0f, Font::bold);
    Colour sectionsTextColour = Colour::fromRGB(255, 255, 225);

    Font menusTextFont = Font(Font::getDefaultSansSerifFontName(), 16.0f, Font::plain);
    Colour menusTextColour = Colour::fromRGB(255, 255, 240);

    // Colours depending on the currently connected model. First is SQ-80, second is ESQ-1, then unknown.
    const Colour backgroundColours[5][2] = {{Colour::fromRGB(110, 110, 115), Colour::fromRGB(60, 60, 65)},
                                            {Colour::fromRGB(80, 80, 85), Colour::fromRGB(50, 50, 55)},
                                            {Colour::fromRGB(80, 80, 85), Colour::fromRGB(50, 50, 55)},
                                            {Colour::fromRGB(80, 80, 85), Colour::fromRGB(50, 50, 55)},
                                            {Colour::fromRGB(90, 90, 95), Colour::fromRGB(55, 55, 60)}};
    const Colour accentColours[5] = {Colour::fromRGB(150, 0, 0), Colour::fromRGB(0, 150, 175), Colour::fromRGB(0, 150, 175), Colour::fromRGB(0, 150, 175),
                                     Colour::fromRGB(175, 175, 175)};
    const Colour refreshButtonColours[5] = {Colour::fromRGB(0, 180, 180), Colour::fromRGB(200, 150, 0), Colour::fromRGB(200, 150, 0), Colour::fromRGB(200, 150, 0),
                                            Colour::fromRGB(175, 175, 175)};
    const Colour importButtonColours[5] = {Colour::fromRGB(127, 0, 55), Colour::fromRGB(30, 30, 30), Colour::fromRGB(30, 30, 30), Colour::fromRGB(30, 30, 30),
                                           Colour::fromRGB(127, 0, 55)};
    const Colour exportButtonColours[5] = {Colour::fromRGB(175, 175, 175), Colour::fromRGB(175, 175, 175), Colour::fromRGB(175, 175, 175), Colour::fromRGB(175, 175, 175),
                                           Colour::fromRGB(175, 175, 175)};

    const float separatorThickness = 10.0f;

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

    // The programSection contains both the programControls and the labels
    GroupComponent programSection;
    GroupComponent programControls;
    // The top row of the display
    GroupComponent statusSection;

    StringArray waveMenuOpts;
    ComboBox waveMenus[3];
    ComboBox octMenus[3];
    ComboBox semiMenus[3];
    ToggleButton LFButtons[3];
    ToggleButton selfOscButton;


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
