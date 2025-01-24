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
#include "BinaryData.h"
#include "DeviceResponse.h"
#include "MidiSysexProcessor.h"
#include "PannelButton.h"
#include "ProgramParser.h"
#include <functional>

using namespace juce;

PlasticTexture::PlasticTexture(const Image& textureImage) : textureImage(textureImage) { setInterceptsMouseClicks(false, true); }

void PlasticTexture::paint(Graphics& g) {
    if (textureImage.isValid()) {
        g.setOpacity(0.13f);
        g.drawImage(textureImage, getLocalBounds().toFloat());
    }
}
void PlasticTexture::resized() {
    if (textureImage.isValid()) {
        textureImage = textureImage.rescaled(getWidth(), getHeight());
    }
}

//==============================================================================
MainComponent::MainComponent() {
    tooltipWindow = std::make_unique<TooltipWindow>(this, 1500);
    currentModel = UNKNOWN;
    refreshButton = std::make_unique<PannelButton>(REFRESH_BUTTON_COLOURS[getCurrentSynthModel()], "Refresh", 640, 130);
    sendButton = std::make_unique<PannelButton>(SEND_BUTTON_COLOURS[getCurrentSynthModel()], "Send...", 150, 410);
    progSaveButton = std::make_unique<PannelButton>(SAVE_BUTTON_COLOUR, "Save Prog \u00B7 As...", 313, 410);
    bankSaveButton = std::make_unique<PannelButton>(SAVE_BUTTON_COLOUR, "Save Bank \u00B7 As...", 477, 410);
    seqSaveButton = std::make_unique<PannelButton>(SAVE_BUTTON_COLOUR, "Save Seq \u00B7 As...", 640, 410);

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

    createLabel(disconnectedUnderline, statusSection, "____________", 560, 10, 300, 30);
    createLabel(sysexDisabledUnderline, statusSection, "_______________________", 500, 10, 300, 30);
    createLabel(statusTitleLabel, statusSection, "5tatu5    =", 398, 5, 250, 30);
    createLabel(statusLabel, statusSection, "Di5connected", 560, 5, 300, 30);
    createLabel(modelLabel, statusSection, "", 580, 5, 300, 30);
    sysexDisabledUnderline.setVisible(false);

    refreshButton->setTooltip("Scan for a connected Ensoniq SQ-80 or ESQ-1 and for MIDI device changes");
    refreshButton->onClick = [this] {
        attemptConnection();
        refreshMidiDevices();
    };
    sendButton->setTooltip("Send a program, bank or sequence to the SQ-80 / ESQ-1");
    sendButton->onClick = [this] {};
    progSaveButton->setTooltip("Save the current program from the SQ-80 / ESQ-1:\nCtrl/Cmd: Save all programs separatly in a directory");
    progSaveButton->onClick = [this] {};
    bankSaveButton->setTooltip("Save the current bank loaded in internal memory from the SQ-80 / ESQ-1");
    bankSaveButton->onClick = [this] {};
    seqSaveButton->setTooltip("Save the current sequence from the SQ-80 / ESQ-1:\nCtrl/Cmd: Save all sequences and sequencer data in a single file");
    seqSaveButton->onClick = [this] {};

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
                }
                break;
            }
        }
        attemptConnection();
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

    const int oscControlsYPos[3] = {100, 130, 160};
    const StringArray OSC_OCTAVE_MENU_OPTIONS = {"-3   to   +5", "+6", "+7"};
    const StringArray OSC_SEMI_OPTIONS = {"+0", "+1", "+2", "+3", "+4", "+5", "+6", "+7", "+8", "+9", "+10", "+11"};

    for (int osc = 0; osc < 3; osc++) {
        String waveMenuTooltip = "Waveform for oscillator " + String(osc + 1) + ":\nThe first option is normal waveforms, the rest are hidden waveforms, if supported.";
        String octMenuTooltip = "Octave for oscillator " + String(osc + 1) + ":\nNormal range is -3 to +5, while +6 and +7 are extended values.";
        String semiMenuTooltip = "Semitone for oscillator " + String(osc + 1);
        String LFButtonTooltip = "Low-Frequency mode:\nShifts the frequency range down by a couple of octaves internally for oscillator " + String(osc + 1) +
                                 " when enabled. Displayed values are unchanged.";

        createComboBox(waveMenus[osc], programControls, 90, oscControlsYPos[osc], 230, 25, waveMenuTooltip,
                       [this, osc] { return midiProcessor.changeOscWaveform(osc, waveMenus[osc].getSelectedItemIndex() + NB_OF_WAVES[getCurrentSynthModel()] - 1); });

        createComboBox(
            octMenus[osc], programControls, 350, oscControlsYPos[osc], 125, 25, octMenuTooltip,
            [this, osc] {
                return midiProcessor.changeOscPitch(osc, octMenus[osc].getSelectedItemIndex() + 5, semiMenus[osc].getSelectedItemIndex(),
                                                    LFButtons[osc].getToggleState());
            },
            OSC_OCTAVE_MENU_OPTIONS);

        createComboBox(
            semiMenus[osc], programControls, 480, oscControlsYPos[osc], 70, 25, semiMenuTooltip,
            [this, osc] {
                return midiProcessor.changeOscPitch(osc, octMenus[osc].getSelectedItemIndex() + 5, semiMenus[osc].getSelectedItemIndex(),
                                                    LFButtons[osc].getToggleState());
            },
            OSC_SEMI_OPTIONS);

        createToggleButton(LFButtons[osc], programControls, 570, oscControlsYPos[osc] + 2, 20, 20, LFButtonTooltip,
                           [this, osc] { return midiProcessor.toggleLowFrequencyMode(osc, LFButtons[osc].getToggleState()); });
    }


    // ------------------ Filter self-oscillation ------------------
    String selfOscButtonTooltip = "Filter self-oscillation:\nShifts the whole resonance range up internally to what would be values of 32-63 when enabled.";
    createToggleButton(selfOscButton, programControls, 680, 132, 20, 20, selfOscButtonTooltip, [this] { return midiProcessor.toggleSelfOscillation(selfOscButton); });

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
    midiControls.addAndMakeVisible(*refreshButton);
    midiControls.addAndMakeVisible(*sendButton);
    midiControls.addAndMakeVisible(*progSaveButton);
    midiControls.addAndMakeVisible(*bankSaveButton);
    midiControls.addAndMakeVisible(*seqSaveButton);

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

    refreshButton->changeColour(REFRESH_BUTTON_COLOURS[selectedThemeOption == AUTOMATIC_THEME ? currentModel
                                                       : selectedThemeOption == NEUTRAL_THEME ? UNKNOWN
                                                                                              : selectedThemeOption - 1]);
    sendButton->changeColour(SEND_BUTTON_COLOURS[selectedThemeOption == AUTOMATIC_THEME ? currentModel
                                                 : selectedThemeOption == NEUTRAL_THEME ? UNKNOWN
                                                                                        : selectedThemeOption - 1]);
    Colour gradientColours[2];
    for (int i = 0; i < 2; i++) {
        gradientColours[i] = BACKGROUND_COLOURS[selectedThemeOption == AUTOMATIC_THEME ? getCurrentSynthModel()
                                                : selectedThemeOption == NEUTRAL_THEME ? UNKNOWN
                                                                                       : selectedThemeOption - 1][i];
    }

    g.setGradientFill(ColourGradient(gradientColours[0], 0, 0, gradientColours[1], (float)(windowWidth / 2), (float)windowHeight, true));
    g.fillAll();

    // Top red line
    g.setColour(ACCENT_COLOURS[selectedThemeOption == AUTOMATIC_THEME ? getCurrentSynthModel()
                               : selectedThemeOption == NEUTRAL_THEME ? UNKNOWN
                                                                      : selectedThemeOption - 1]);
    g.drawLine(0, 0, (float)windowWidth, 0, (float)(SEPARATOR_WIDTH * 1.5));

    // Thin red lines under logo
    g.drawLine(25, 105, 345, 105);
    g.drawLine(25, 125, 345, 125);
    g.drawLine(25, 145, 345, 145);
    g.drawLine(25, 165, 345, 165);

    // Top seperator line
    g.setColour(Colour::fromRGB(150, 150, 150));
    g.drawLine(370, 20, 370, 170, SEPARATOR_WIDTH);
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

    for (int themeOption = 0; themeOption < THEME_OPTIONS.size(); themeOption++) {
        if (themeOption == AUTOMATIC_THEME + 1)
            themeSubMenu.addSeparator();
        themeSubMenu.addItem(PopupMenu::Item(THEME_OPTIONS[themeOption]).setTicked(selectedThemeOption == themeOption).setAction([themeOption, this]() {
            selectedThemeOption = themeOption;
            repaint();
        }));
    }

    menu.addSubMenu("Theme", themeSubMenu);
    menu.addItem(2, "About SideQick...");
    menu.addItem(3, "Quit");
    menu.showMenuAsync(PopupMenu::Options(), [this](int result) {
        if (result == 2) {
            AlertWindow::showMessageBoxAsync(AlertWindow::NoIcon, "SideQick",
                                             "Ensoniq SQ-80/ESQ-1 Expansion Software\nVersion Beta 2.1\n\nCopyright Vincent Zauhar, 2025\nReleased under the "
                                             "GNU GPL v3 license\n\nhttps://github.com/VincyZed/SideQick");
        } else if (result == 3)
            JUCEApplication::getInstance()->systemRequestedQuit();
    });
}

void MainComponent::handleIncomingMidiMessage(MidiInput* source, const MidiMessage& message) { midiProcessor.processIncomingMidiData(source, message); }

void MainComponent::updateStatus(DeviceResponse response) {

    auto updateStatusLabel = [this](const String& text, bool center) {
        statusLabel.setText(text, NO_NOTIF);
        statusLabel.setBounds(center ? 560 : 500, 5, 300, 30);
    };
    auto updateModelLabel = [this](DeviceResponse response) {
        modelLabel.setVisible(response.status == STATUS_MESSAGES[CONNECTED] || response.status == STATUS_MESSAGES[SYSEX_DISABLED]);
        modelLabel.setBounds(response.status == STATUS_MESSAGES[SYSEX_DISABLED] ? modelLabelXPos + 55 : modelLabelXPos, modelLabel.getY(), modelLabel.getWidth(),
                             modelLabel.getHeight());
        modelLabel.setTooltip("MIDI channel: " + midiProcessor.getChannel() + "\nSystem version: " + osVersion[MAJOR] + "." + osVersion[MINOR]);
    };
    auto setGroupComponents = [this](String& status, bool midiControlsEnabled, bool programControlsEnabled, bool programSectionOn) {
        midiControls.setEnabled(midiControlsEnabled);
        programControls.setEnabled(programControlsEnabled);
        display.toggleProgramSection(programSectionOn ? ON : OFF);
        disconnectedUnderline.setVisible(status == STATUS_MESSAGES[DISCONNECTED]);
        sysexDisabledUnderline.setVisible(status == STATUS_MESSAGES[SYSEX_DISABLED]);
    };


    if (response.status == STATUS_MESSAGES[CONNECTED]) {

        if (response.model != UNCHANGED && response.model != UNKNOWN) {
            modelLabel.setText(SYNTH_MODELS[response.model], NO_NOTIF);
            osVersion[MAJOR] = response.osVersion[MAJOR];
            osVersion[MINOR] = response.osVersion[MINOR];
        }

        currentModel = getCurrentSynthModel();

        if (response.model != UNCHANGED) {
            // Update the waveforms in the menus
            for (int osc = 0; osc < 3; osc++)
                waveMenus[osc].clear(NO_NOTIF);

            waveMenuOpts = {"WAV0    to    WAV" + String(NB_OF_WAVES[currentModel] - 1)};
            // Add hidden waveforms to the menu if the synth supports them
            if (response.supportsHiddenWaves) {
                for (int w = NB_OF_WAVES[currentModel]; w < 256; w++)
                    waveMenuOpts.add("WAV" + String(w));

                for (int osc = 0; osc < 3; osc++) {
                    waveMenus[osc].addItemList(waveMenuOpts, 1);
                    waveMenus[osc].setTooltip("Waveform for oscillator " + String(osc + 1) + ":\nThe first option is normal waveforms, the rest are hidden waveforms.");
                }
            } else {
                for (int osc = 0; osc < 3; osc++) {
                    display.toggleComponent(waveMenus[osc], OFF);
                    waveMenus[osc].setTextWhenNothingSelected("Un5upported");
                    if (currentModel == ESQ1)
                        waveMenus[osc].setTooltip("Hidden waveforms are only accessible with OS version 3.5 and above on the ESQ-1");
                    else
                        waveMenus[osc].setTooltip("Hidden waveforms are not supported on the ESQ-M");
                }
            }

            if (selectedThemeOption == AUTOMATIC_THEME && response.model != UNKNOWN)
                repaint();
        }

        auto parameterValues = ProgramParser(response.currentProgram, currentModel);
        // Update the display options according to current program values
        for (int osc = 0; osc < 3; osc++) {
            waveMenus[osc].setSelectedItemIndex(parameterValues.currentWave[osc], NO_NOTIF);
            octMenus[osc].setSelectedItemIndex(parameterValues.currentOct[osc], NO_NOTIF);
            semiMenus[osc].setSelectedItemIndex(parameterValues.currentRealSemi[osc], NO_NOTIF);
            LFButtons[osc].setToggleState(parameterValues.currentOscLF[osc], NO_NOTIF);
        }
        selfOscButton.setToggleState(parameterValues.currentSelfOsc, NO_NOTIF);

        updateStatusLabel(STATUS_MESSAGES[CONNECTED] + "    to    ", false);
        setGroupComponents(response.status, true, true, true);

    } else if (response.status == STATUS_MESSAGES[DISCONNECTED]) {
        updateStatusLabel(STATUS_MESSAGES[DISCONNECTED], true);
        programNameLabel.setText("______", NO_NOTIF);
        setGroupComponents(response.status, true, false, false);
    } else if (response.status == STATUS_MESSAGES[SYSEX_DISABLED]) {
        updateStatusLabel(STATUS_MESSAGES[SYSEX_DISABLED] + "    on    ", false);
        if (response.model != UNCHANGED && response.model != UNKNOWN) {
            modelLabel.setText(SYNTH_MODELS[response.model], NO_NOTIF);
            currentModel = getCurrentSynthModel();
            if (selectedThemeOption == AUTOMATIC_THEME)
                repaint();
        }
        setGroupComponents(response.status, true, false, false);
    } else if (response.status == STATUS_MESSAGES[MODIFYING_PROGRAM] || response.status == STATUS_MESSAGES[REFRESHING]) {
        updateStatusLabel(response.status, response.status == STATUS_MESSAGES[REFRESHING]);
        setGroupComponents(response.status, false, false, false);
    }

    updateModelLabel(response);
}

void MainComponent::attemptConnection() {
    if (midiInMenu.getSelectedItemIndex() > 0 && midiOutMenu.getSelectedItemIndex() > 0) {
        updateStatus(DeviceResponse(STATUS_MESSAGES[REFRESHING], NO_PROG));
        Thread::launch([this] {
            auto response = midiProcessor.requestDeviceInquiry();
            MessageManager::callAsync([this, response] { updateStatus(response); });
        });
    } else
        updateStatus(DeviceResponse(STATUS_MESSAGES[DISCONNECTED], NO_PROG));
}

void MainComponent::refreshMidiDevices(bool allowMenuSwitch) {
    midiInDeviceNames.clear();
    midiOutDeviceNames.clear();
    // Check for MIDI input devices and populate the combo box
    for (auto& device : MidiInput::getAvailableDevices()) {
        if (!IGNORED_MIDI_DEVICES.contains(device.name))
            midiInDeviceNames.add(device.name);
    }

    String currentDevice = midiInMenu.getText();
    midiInMenu.clear(NO_NOTIF);
    midiInMenu.addItem("None", 1);
    midiInMenu.addItemList(midiInDeviceNames, 2);
    if (midiInDeviceNames.contains(currentDevice) && currentDevice != "None")
        // Select the previously selected MIDI input device if it's still available
        midiInMenu.setSelectedItemIndex(midiInDeviceNames.indexOf(currentDevice) + 1, NO_NOTIF);
    else
        // Select the first device in the list if the previously selected device is not available,
        // or None if no input devices are available or we don't change the context menu value
        midiInMenu.setSelectedItemIndex(allowMenuSwitch && midiInDeviceNames.size() > 0 ? 1 : 0, sendNotification);

    // Check for MIDI output devices and populate the combo box
    for (auto& device : MidiOutput::getAvailableDevices()) {
        if (!IGNORED_MIDI_DEVICES.contains(device.name))
            midiOutDeviceNames.add(device.name);
    }
    currentDevice = midiOutMenu.getText();
    midiOutMenu.clear(NO_NOTIF);
    midiOutMenu.addItem("None", 1);
    midiOutMenu.addItemList(midiOutDeviceNames, 2);
    if (midiOutDeviceNames.contains(currentDevice) && currentDevice != "None")
        // Same thing but for the output device.
        midiOutMenu.setSelectedItemIndex(midiOutDeviceNames.indexOf(currentDevice) + 1, NO_NOTIF);
    else
        midiOutMenu.setSelectedItemIndex(allowMenuSwitch && midiOutDeviceNames.size() > 0 ? 1 : 0, sendNotification);
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
    else if (modelLabel.getText() == SYNTH_MODELS[ESQM])
        return ESQM;
    else if (modelLabel.getText() == SYNTH_MODELS[SQ80M])
        return SQ80M;
    else
        return UNKNOWN;
}

void MainComponent::createLabel(Label& label, Component& parent, const String& text, const int x, const int y, const int width, const int height, const Colour& colour,
                                const Font& font) {
    label.setBounds(x, y, width, height);
    label.setText(text, NO_NOTIF);
    label.setFont(font);
    if (colour != Colour())
        label.setColour(Label::textColourId, colour);
    parent.addAndMakeVisible(label);
}

void MainComponent::createComboBox(ComboBox& comboBox, Component& parent, const int x, const int y, const int width, const int height, const String& tooltip,
                                   const std::function<DeviceResponse()>& onChangeFunc, const StringArray& items) {
    comboBox.addItemList(items, 1);
    comboBox.setSelectedItemIndex(0, NO_NOTIF);
    comboBox.setBounds(x, y, width, height);
    comboBox.setTooltip(tooltip);
    comboBox.setText("", NO_NOTIF);
    parent.addAndMakeVisible(comboBox);
    comboBox.onChange = [this, onChangeFunc] { displayControlOnChange(onChangeFunc); };
}

void MainComponent::createToggleButton(ToggleButton& button, Component& parent, const int x, const int y, const int width, const int height, const String& tooltip,
                                       const std::function<DeviceResponse()>& onClickFunc) {
    button.setBounds(x, y, width, height);
    button.setTooltip(tooltip);
    parent.addAndMakeVisible(button);
    button.onClick = [this, onClickFunc] { displayControlOnChange(onClickFunc); };
}

void MainComponent::displayControlOnChange(const std::function<DeviceResponse()>& onChangeFunc) {
    updateStatus(DeviceResponse(STATUS_MESSAGES[MODIFYING_PROGRAM], NO_PROG));
    Thread::launch([this, &onChangeFunc] {
        DeviceResponse status = onChangeFunc();
        MessageManager::callAsync([this, status] { updateStatus(status); });
    });
}
