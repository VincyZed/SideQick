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

#include "MainComponent.h"
#include "DeviceResponse.h"
#include "MidiSysexProcessor.h"
#include "PannelButton.h"
#include "ProgramParser.h"
#include <functional>

using namespace juce;

PlasticTexture::PlasticTexture(const Image& textureImage) : textureImage(textureImage) { setInterceptsMouseClicks(false, true); }

void PlasticTexture::paint(Graphics& g) {
    if (textureImage.isValid()) {
        g.setOpacity(0.125f);
        g.drawImage(textureImage, getLocalBounds().toFloat());
    }
}
void PlasticTexture::resized() {
    if (textureImage.isValid()) {
        textureImage = textureImage.rescaled(getWidth(), getHeight());
    }
}

//==============================================================================
MainComponent::MainComponent() : refreshButton(refreshButtonColours[getCurrentSynthModel()], "Refresh", 640, 130), currentModel(UNKNOWN), tooltipWindow(this, 1500) {

    // Set the look and feel, plastic texture and logo

    auto customLookAndFeel = std::make_unique<DisplayLookAndFeel>();

    statusSection.setLookAndFeel(customLookAndFeel.get());
    lookAndFeel = std::move(customLookAndFeel);

    // Load the texture image from binary data
    MemoryInputStream memoryInputStream(BinaryData::plastic_png, BinaryData::plastic_pngSize, false);
    auto textureImage = ImageFileFormat::loadFrom(memoryInputStream);

    textureOverlay = std::make_unique<PlasticTexture>(textureImage);
    textureOverlay->setBounds(0, 0, windowWidth, windowHeight);

    logo.setBounds(25, 30, 395, 60);
    addAndMakeVisible(logo);

    // ========================= Sections labels =========================

    createLabel(descriptionLabel, *this, "SQ-80 / ESQ-1 expansion software", 25, 145, 395, 20, descriptionTextColour, descriptionTextFont);
    createLabel(midiSectionLabel, *this, "Midi", 400, 150, 50, 30, sectionsTextColour, sectionsTextFont);
    createLabel(midiInLabel, *this, "Input device :", 400, 20, 200, 30, menusTextColour, menusTextFont);
    createLabel(midiOutLabel, *this, "Output device :", 400, 55, 200, 30, menusTextColour, menusTextFont);

    // ==================== Program Name =====================
    // createLabel(programTitleLabel, programControls, "Current   Program   =", 10, 5, 395, 30);
    // createLabel(programNameLabel, programControls, "______", 210, 5, 300, 30);
    // ==================== Status Section =====================

    createLabel(statusTitleLabel, statusSection, "5tatu5    =", 398, 5, 250, 30);
    createLabel(statusLabel, statusSection, "Di5connected", 560, 5, 300, 30);
    createLabel(modelLabel, statusSection, "", 580, 5, 300, 30);
    createLabel(disconnectedUnderline, statusSection, "____________", 560, 10, 300, 30);
    createLabel(sysexDisabledUnderline, statusSection, "_______________________", 500, 10, 300, 30);


    refreshButton.setTooltip("Scan for a connected Ensoniq SQ-80 or ESQ-1");
    refreshButton.onClick = [this] {
        attemptConnection();
        refreshMidiDevices();
    };

    // ========================== MIDI Options ==========================

    refreshMidiDevices(true);

    midiInMenu.setBounds(540, 20, 240, 25);
    midiInMenu.setText("Select MIDI Input Device");
    midiInMenu.setTooltip("MIDI input device used to receive data from the SQ-80 or ESQ-1");
    // Select None or the first device in the list
    midiInMenu.setSelectedItemIndex((midiInMenu.getNumItems() > 1));

    midiControls.addAndMakeVisible(midiInMenu);
    midiInMenu.onChange = [this] {
        auto midiInDevices = MidiInput::getAvailableDevices();
        for (auto device : midiInDevices) {
            // Search for the right midi device to open from its name from the context menu
            if (device.name == midiInMenu.getText()) {
                midiProcessor.selectedMidiIn = MidiInput::openDevice(device.identifier, this);
                if (midiProcessor.selectedMidiIn) {
                    midiProcessor.selectedMidiIn->start();
                    attemptConnection();
                }
                break;
            }
        }
    };

    midiOutMenu.setBounds(540, 55, 240, 25);
    midiOutMenu.setText("Select MIDI Output Device");
    midiOutMenu.setTooltip("MIDI output device used to send data to the SQ-80 or ESQ-1");
    // Select None or the first device in the list
    midiOutMenu.setSelectedItemIndex((midiOutMenu.getNumItems() > 1));

    midiControls.addAndMakeVisible(midiOutMenu);
    midiOutMenu.onChange = [this] {
        auto midiOutDevices = MidiOutput::getAvailableDevices();
        for (auto device : midiOutDevices) {
            if (device.name == midiOutMenu.getText()) {
                midiProcessor.selectedMidiOut = MidiOutput::openDevice(device.identifier);
                break;
            }
        }
        attemptConnection();
    };

    midiControls.setBounds(0, 0, windowWidth, windowHeight);

    // ========================== Display elements ==========================

    // --------------------------- Labels ---------------------------
    createLabel(osc1Label, programSection, "O5C 1", 10, 100, 125, 25);
    createLabel(osc2Label, programSection, "O5C 2", 10, 130, 125, 25);
    createLabel(osc3Label, programSection, "O5C 3", 10, 160, 125, 25);
    createLabel(waveLabel, programSection, "WAVEFORM", 90, 70, 125, 25);
    createLabel(octLabel, programSection, "OCT", 350, 70, 125, 25);
    createLabel(semiLabel, programSection, "5EMI", 480, 70, 125, 25);
    createLabel(LFLabel, programSection, "LF", 565, 70, 125, 25);
    createLabel(selfOscLabel, programSection, "Filt    5elf-O5c", 610, 102, 250, 20);

    // ------------------ OSC Octaves and semitones ------------------

    const StringArray OSC_OCTAVE_MENU_OPTIONS = {"-3   to   +5", "+6", "+7"};

    createComboBox(osc1WaveMenu, programControls, 90, 100, 230, 25, "Waveform for oscillator 1:\nStarts with normal waveforms, then goes into hidden waveforms if supported.",
                   [this] { return midiProcessor.changeOscWaveform(OSC1, osc1WaveMenu.getSelectedItemIndex() + NB_OF_WAVES[getCurrentSynthModel()] - 1); });
    createComboBox(osc2WaveMenu, programControls, 90, 130, 230, 25, "Waveform for oscillator 2:\nStarts with normal waveforms, then goes into hidden waveforms if supported.",
                   [this] { return midiProcessor.changeOscWaveform(OSC2, osc2WaveMenu.getSelectedItemIndex() + NB_OF_WAVES[getCurrentSynthModel()] - 1); });
    createComboBox(osc3WaveMenu, programControls, 90, 160, 230, 25, "Waveform for oscillator 3:\nStarts with normal waveforms, then goes into hidden waveforms if supported.",
                   [this] { return midiProcessor.changeOscWaveform(OSC3, osc3WaveMenu.getSelectedItemIndex() + NB_OF_WAVES[getCurrentSynthModel()] - 1); });

    createComboBox(
        osc1OctMenu, programControls, 350, 100, 125, 25, "Octave for oscillator 1:\nNormal range is -3 to +5, while +6 and +7 are extended values.",
        [this] { return midiProcessor.changeOscPitch(OSC1, osc1OctMenu.getSelectedItemIndex() + 5, osc1SemiMenu.getSelectedItemIndex(), osc1LFButton.getToggleState()); }, OSC_OCTAVE_MENU_OPTIONS);
    createComboBox(
        osc2OctMenu, programControls, 350, 130, 125, 25, "Octave for oscillator 2:\nNormal range is -3 to +5, while +6 and +7 are extended values.",
        [this] { return midiProcessor.changeOscPitch(OSC2, osc2OctMenu.getSelectedItemIndex() + 5, osc2SemiMenu.getSelectedItemIndex(), osc2LFButton.getToggleState()); }, OSC_OCTAVE_MENU_OPTIONS);
    createComboBox(
        osc3OctMenu, programControls, 350, 160, 125, 25, "Octave for oscillator 3:\nNormal range is -3 to +5, while +6 and +7 are extended values.",
        [this] { return midiProcessor.changeOscPitch(OSC3, osc3OctMenu.getSelectedItemIndex() + 5, osc3SemiMenu.getSelectedItemIndex(), osc3LFButton.getToggleState()); }, OSC_OCTAVE_MENU_OPTIONS);


    const StringArray OSC_SEMI_OPTIONS = {"+0", "+1", "+2", "+3", "+4", "+5", "+6", "+7", "+8", "+9", "+10", "+11"};

    createComboBox(osc1SemiMenu, programControls, 480, 100, 70, 25, "Semitone for oscillator 1", [this] { return midiProcessor.changeOscPitch(OSC1, osc1OctMenu.getSelectedItemIndex() + 5, osc1SemiMenu.getSelectedItemIndex(), osc1LFButton.getToggleState()); }, OSC_SEMI_OPTIONS);
    createComboBox(osc2SemiMenu, programControls, 480, 130, 70, 25, "Semitone for oscillator 2", [this] { return midiProcessor.changeOscPitch(OSC2, osc2OctMenu.getSelectedItemIndex() + 5, osc2SemiMenu.getSelectedItemIndex(), osc2LFButton.getToggleState()); }, OSC_SEMI_OPTIONS);
    createComboBox(osc3SemiMenu, programControls, 480, 160, 70, 25, "Semitone for oscillator 3", [this] { return midiProcessor.changeOscPitch(OSC3, osc3OctMenu.getSelectedItemIndex() + 5, osc3SemiMenu.getSelectedItemIndex(), osc3LFButton.getToggleState()); }, OSC_SEMI_OPTIONS);


    createToggleButton(osc1LFButton, programControls, 565, 100, 20, 20, "Low-Frequency mode:\nShifts the whole frequency range down by a couple of octaves for oscillator 1 when enabled.", [this] { return midiProcessor.toggleLowFrequencyMode(OSC1, osc1LFButton); });
    createToggleButton(osc2LFButton, programControls, 565, 130, 20, 20, "Low-Frequency mode:\nShifts the whole frequency range down by a couple of octaves for oscillator 2 when enabled.", [this] { return midiProcessor.toggleLowFrequencyMode(OSC2, osc2LFButton); });
    createToggleButton(osc3LFButton, programControls, 565, 160, 20, 20, "Low-Frequency mode:\nShifts the whole frequency range down by a couple of octaves for oscillator 3 when enabled.", [this] { return midiProcessor.toggleLowFrequencyMode(OSC3, osc3LFButton); });

    // ------------------ Filter self-oscillation ------------------

    createToggleButton(selfOscButton, programControls, 680, 130, 20, 20, "Filter self-oscillation:\nShifts the whole resonance range internally up to 32-63 when enabled.", [this] { return midiProcessor.toggleSelfOscillation(selfOscButton); });

    programControls.setBounds(0, 0, displayWidth, displayHeight);
    programControls.setColour(GroupComponent::outlineColourId, Colours::transparentBlack);
    programControls.setEnabled(false);

    display.setBounds(25, 180, displayWidth, displayHeight);

    programSection.setBounds(0, 0, displayWidth, displayHeight);
    programSection.setColour(GroupComponent::outlineColourId, Colours::transparentBlack);
    programSection.addAndMakeVisible(programControls);
    display.addAndMakeVisible(programSection);

    statusSection.setBounds(0, 0, displayWidth, displayHeight);
    statusSection.setColour(GroupComponent::outlineColourId, Colours::transparentBlack);
    display.addAndMakeVisible(statusSection);

    midiControls.setColour(GroupComponent::outlineColourId, Colours::transparentBlack);

    addAndMakeVisible(display);

    addAndMakeVisible(*textureOverlay);
    addAndMakeVisible(midiControls);
    midiControls.addAndMakeVisible(refreshButton);

    setSize(windowWidth, windowHeight);

    // For the blinking underline on status errors
    startTimer(500);
}

MainComponent::~MainComponent() {

    if (midiProcessor.selectedMidiIn)
        midiProcessor.selectedMidiIn->stop();

    display.setLookAndFeel(nullptr);
    stopTimer();
}

//==============================================================================
void MainComponent::prepareToPlay(int samplesPerBlockExpected, double sampleRate) {}
void MainComponent::getNextAudioBlock(const AudioSourceChannelInfo& bufferToFill) {}
void MainComponent::releaseResources() {}
//==============================================================================

void MainComponent::paint(Graphics& g) {
    g.setGradientFill(ColourGradient(backgroundColours[selectedThemeOption == AUTOMATIC_THEME ? getCurrentSynthModel() : selectedThemeOption - 1][0], 0, 0, backgroundColours[selectedThemeOption == AUTOMATIC_THEME ? getCurrentSynthModel() : selectedThemeOption - 1][1], (float)(windowWidth / 2),
                                     (float)windowHeight, true));
    g.fillAll();

    // Top red line
    g.setColour(accentColours[selectedThemeOption == AUTOMATIC_THEME ? getCurrentSynthModel() : selectedThemeOption - 1]);
    g.drawLine(0, 0, (float)windowWidth, 0, (float)(separatorThickness * 1.5));

    // Thin red lines under logo
    g.drawLine(25, 105, 345, 105);
    g.drawLine(25, 125, 345, 125);
    g.drawLine(25, 145, 345, 145);
    g.drawLine(25, 165, 345, 165);

    // Top seperator line
    g.setColour(Colour::fromRGB(150, 150, 150));
    g.drawLine(370, 20, 370, 170, separatorThickness);
}

void MainComponent::resized() {}

void MainComponent::mouseDown(const juce::MouseEvent& event) {
    if (event.mods.isRightButtonDown())
        showContextMenu();
}

void MainComponent::showContextMenu() {

    // Create a menu
    PopupMenu menu;
    PopupMenu themeSubMenu;
    // Add theme options with checkmarks and actions
    themeSubMenu.addItem(PopupMenu::Item("Automatic").setTicked(selectedThemeOption == AUTOMATIC_THEME).setAction([this]() {
        selectedThemeOption = AUTOMATIC_THEME;
        updateTheme();
    }));
    themeSubMenu.addSeparator();
    themeSubMenu.addItem(PopupMenu::Item("SQ-80").setTicked(selectedThemeOption == SQ80_THEME).setAction([this]() {
        selectedThemeOption = SQ80_THEME;
        updateTheme();
    }));
    themeSubMenu.addItem(PopupMenu::Item("ESQ-1").setTicked(selectedThemeOption == ESQ1_THEME).setAction([this]() {
        selectedThemeOption = ESQ1_THEME;
        updateTheme();
    }));
    themeSubMenu.addItem(PopupMenu::Item("Neutral").setTicked(selectedThemeOption == NEUTRAL_THEME).setAction([this]() {
        selectedThemeOption = NEUTRAL_THEME;
        updateTheme();
    }));

    menu.addSubMenu("Theme", themeSubMenu);
    menu.addItem(2, "About SideQick...");
    menu.addItem(3, "Quit");
    menu.showMenuAsync(PopupMenu::Options(), [this](int result) {
        if (result == 2) {
            AlertWindow::showMessageBoxAsync(AlertWindow::NoIcon, "SideQick", "Ensoniq SQ-80/ESQ-1 Expansion Software\nVersion 0.43\n\nCopyright Vincent Zauhar, 2024\nReleased under the GNU GPL v3 license\n\nhttps://github.com/VincyZed/SideQick");
        } else if (result == 3)
            JUCEApplication::getInstance()->systemRequestedQuit();
    });
}

void MainComponent::handleIncomingMidiMessage(MidiInput* source, const MidiMessage& message) { midiProcessor.processIncomingMidiData(source, message); }

void MainComponent::updateStatus(DeviceResponse response) {

    if (response.status == STATUS_MESSAGES[CONNECTED]) {
        statusLabel.setText(STATUS_MESSAGES[CONNECTED] + "    to    ", NO);

        if (response.model != UNCHANGED && response.model != UNKNOWN)
            modelLabel.setText(SYNTH_MODELS[response.model], NO);

        modelLabel.setBounds(modelLabelXPos, modelLabel.getY(), modelLabel.getWidth(), modelLabel.getHeight());
        modelLabel.setVisible(true);

        currentModel = getCurrentSynthModel();

        if (response.model != UNCHANGED) {
            // Remove all the waveforms from the menu
            osc1WaveMenu.clear(NO);
            osc2WaveMenu.clear(NO);
            osc3WaveMenu.clear(NO);

            // Add the new waveforms to the menu
            waveMenuOpts = {"WAV0    to    WAV" + String(NB_OF_WAVES[currentModel] - 1)};
            if (currentModel == SQ80 || currentModel == ESQ1 && response.osVersion >= ESQ1_HIDDEN_WAVES_MIN_VERSION) {
                for (int i = NB_OF_WAVES[currentModel]; i < 256; i++) {
                    waveMenuOpts.add("WAV" + String(i));
                }
                osc1WaveMenu.addItemList(waveMenuOpts, 1);
                osc2WaveMenu.addItemList(waveMenuOpts, 1);
                osc3WaveMenu.addItemList(waveMenuOpts, 1);
            }

            // Update the theme
            if (selectedThemeOption == AUTOMATIC_THEME && response.model != UNKNOWN) {
                updateTheme();
            }
        }

        auto parameterValues = ProgramParser(response.currentProgram, currentModel);

        osc1WaveMenu.setSelectedItemIndex(parameterValues.currentWave[OSC1], NO);
        osc2WaveMenu.setSelectedItemIndex(parameterValues.currentWave[OSC2], NO);
        osc3WaveMenu.setSelectedItemIndex(parameterValues.currentWave[OSC3], NO);
        osc1OctMenu.setSelectedItemIndex(parameterValues.currentOct[OSC1], NO);
        osc2OctMenu.setSelectedItemIndex(parameterValues.currentOct[OSC2], NO);
        osc3OctMenu.setSelectedItemIndex(parameterValues.currentOct[OSC3], NO);
        osc1SemiMenu.setSelectedItemIndex(parameterValues.currentRealSemi[OSC1], NO);
        osc2SemiMenu.setSelectedItemIndex(parameterValues.currentRealSemi[OSC2], NO);
        osc3SemiMenu.setSelectedItemIndex(parameterValues.currentRealSemi[OSC3], NO);
        osc1LFButton.setToggleState(parameterValues.currentOscLF[OSC1], NO);
        osc2LFButton.setToggleState(parameterValues.currentOscLF[OSC2], NO);
        osc3LFButton.setToggleState(parameterValues.currentOscLF[OSC3], NO);
        selfOscButton.setToggleState(parameterValues.currentSelfOsc, NO);

        statusLabel.setBounds(500, 5, 300, 30);
        programControls.setEnabled(true);
        midiControls.setEnabled(true);
        disconnectedUnderline.setVisible(false);
        sysexDisabledUnderline.setVisible(false);
        display.toggleProgramSection(ON);
    } else if (response.status == STATUS_MESSAGES[DISCONNECTED]) {
        statusLabel.setBounds(560, 5, 300, 30);
        statusLabel.setText(STATUS_MESSAGES[DISCONNECTED], NO);
        modelLabel.setVisible(false);
        programNameLabel.setText("______", NO);
        midiControls.setEnabled(true);
        programControls.setEnabled(false);
        display.toggleProgramSection(OFF);
        sysexDisabledUnderline.setVisible(false);
    } else if (response.status == STATUS_MESSAGES[SYSEX_DISABLED]) {
        statusLabel.setBounds(500, 5, 300, 30);
        statusLabel.setText(STATUS_MESSAGES[SYSEX_DISABLED] + "    on    ", NO);
        if (response.model != UNCHANGED && response.model != UNKNOWN) {
            modelLabel.setText(SYNTH_MODELS[response.model], NO);
            currentModel = getCurrentSynthModel();
            if (selectedThemeOption == AUTOMATIC_THEME)
                updateTheme();
        }
        modelLabel.setBounds(modelLabelXPos + 55, modelLabel.getY(), modelLabel.getWidth(), modelLabel.getHeight());
        modelLabel.setVisible(true);
        programNameLabel.setText("______", NO);
        midiControls.setEnabled(true);
        programControls.setEnabled(false);
        display.toggleProgramSection(OFF);
        disconnectedUnderline.setVisible(false);
    } else if (response.status == STATUS_MESSAGES[MODIFYING_PROGRAM] || response.status == STATUS_MESSAGES[REFRESHING]) {
        response.status == STATUS_MESSAGES[MODIFYING_PROGRAM] ? statusLabel.setBounds(500, 5, 300, 30) : statusLabel.setBounds(560, 5, 300, 30);
        statusLabel.setText(response.status, NO);
        modelLabel.setVisible(false);
        midiControls.setEnabled(false);
        programControls.setEnabled(false);
        display.toggleProgramSection(OFF);
        disconnectedUnderline.setVisible(false);
        sysexDisabledUnderline.setVisible(false);
    }
}

void MainComponent::attemptConnection() {
    if (midiInMenu.getSelectedItemIndex() > 0 && midiOutMenu.getSelectedItemIndex() > 0) {
        updateStatus(DeviceResponse(STATUS_MESSAGES[REFRESHING]));
        Thread::launch([this] {
            auto response = midiProcessor.requestDeviceInquiry();
            MessageManager::callAsync([this, response] { updateStatus(response); });
        });
    }
}

void MainComponent::refreshMidiDevices(bool allowMenuSwitch) {
    midiInDeviceNames.clear();
    midiOutDeviceNames.clear();
    // Check for MIDI input devices and populate the combo box
    for (auto& device : MidiInput::getAvailableDevices()) {
        if (!ignoredMidiDevices.contains(device.name))
            midiInDeviceNames.add(device.name);
    }

    String currentDevice = midiInMenu.getText();
    midiInMenu.clear(NO);
    midiInMenu.addItem("None", 1);
    midiInMenu.addItemList(midiInDeviceNames, 2);
    if (midiInDeviceNames.contains(currentDevice) && currentDevice != "None")
        // Select the previously selected MIDI input device if it's still available
        midiInMenu.setSelectedItemIndex(midiInDeviceNames.indexOf(currentDevice) + 1, NO);
    else
        // Select the first device in the list if the previously selected device is not available,
        // or None if no input devices are available or we don't changing the context menu value
        midiInMenu.setSelectedItemIndex(allowMenuSwitch && midiInDeviceNames.size() > 0 ? 1 : 0, sendNotification);

    // Check for MIDI output devices and populate the combo box
    for (auto& device : MidiOutput::getAvailableDevices()) {
        if (!ignoredMidiDevices.contains(device.name))
            midiOutDeviceNames.add(device.name);
    }
    currentDevice = midiOutMenu.getText();
    midiOutMenu.clear(NO);
    midiOutMenu.addItem("None", 1);
    midiOutMenu.addItemList(midiOutDeviceNames, 2);
    if (midiOutDeviceNames.contains(currentDevice) && currentDevice != "None")
        // Same thing but for the output device.
        midiOutMenu.setSelectedItemIndex(midiOutDeviceNames.indexOf(currentDevice) + 1, NO);
    else
        midiOutMenu.setSelectedItemIndex(allowMenuSwitch && midiOutDeviceNames.size() > 0 ? 1 : 0, sendNotification);
}

void MainComponent::updateTheme() {
    refreshButton.changeColour(refreshButtonColours[selectedThemeOption == AUTOMATIC_THEME ? currentModel : selectedThemeOption - 1]);
    refreshButton.repaint();
    repaint();
}

void MainComponent::timerCallback() {
    // Blink the underline on status errors
    if (statusLabel.getText() == STATUS_MESSAGES[DISCONNECTED])
        disconnectedUnderline.setVisible(!disconnectedUnderline.isVisible());
    else if (statusLabel.getText().startsWith(STATUS_MESSAGES[SYSEX_DISABLED]))
        sysexDisabledUnderline.setVisible(!sysexDisabledUnderline.isVisible());
}

SynthModel MainComponent::getCurrentSynthModel() const {
    // Yes, I use the label text to keep track of the current synth model. This is really hacky but
    // it works for now with multithreading. PRs are welcome :)
    if (modelLabel.getText() == SYNTH_MODELS[SQ80])
        return SQ80;
    else if (modelLabel.getText() == SYNTH_MODELS[ESQ1])
        return ESQ1;
    else
        return UNKNOWN;
}

void MainComponent::createLabel(Label& label, Component& parent, const String& text, const int x, const int y, const int width, const int height, const Colour& colour, const Font& font) {
    label.setBounds(x, y, width, height);
    label.setText(text, NO);
    label.setFont(font);
    if (colour != Colour())
        label.setColour(Label::textColourId, colour);
    parent.addAndMakeVisible(label);
}

void MainComponent::createComboBox(ComboBox& comboBox, Component& parent, const int x, const int y, const int width, const int height, const String& tooltip, const std::function<DeviceResponse()>& onChangeFunc, const StringArray& items) {
    comboBox.addItemList(items, 1);
    comboBox.setSelectedItemIndex(0, NO);
    comboBox.setBounds(x, y, width, height);
    comboBox.setTooltip(tooltip);
    parent.addAndMakeVisible(comboBox);
    comboBox.onChange = [this, onChangeFunc] { displayControlOnChange(onChangeFunc); };
}

void MainComponent::createToggleButton(ToggleButton& button, Component& parent, const int x, const int y, const int width, const int height, const String& tooltip, const std::function<DeviceResponse()>& onClickFunc) {
    button.setBounds(x, y, width, height);
    button.setTooltip(tooltip);
    parent.addAndMakeVisible(button);
    button.onClick = [this, onClickFunc] { displayControlOnChange(onClickFunc); };
}

void MainComponent::displayControlOnChange(const std::function<DeviceResponse()>& onChangeFunc) {
    updateStatus(DeviceResponse(STATUS_MESSAGES[MODIFYING_PROGRAM]));
    Thread::launch([this, &onChangeFunc] {
        DeviceResponse status = onChangeFunc();
        MessageManager::callAsync([this, status] { updateStatus(status); });
    });
}