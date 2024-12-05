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
    descriptionLabel.setBounds(25, 145, 395, 20);
    descriptionLabel.setText("SQ-80 / ESQ-1 expansion software", NO);
    descriptionLabel.setFont(descriptionTextFont);
    descriptionLabel.setColour(Label::textColourId, descriptionTextColour);
    addAndMakeVisible(descriptionLabel);

    midiSectionLabel.setBounds(400, 150, 50, 30);
    midiSectionLabel.setText("Midi", NO);
    midiSectionLabel.setFont(sectionsTextFont);
    midiSectionLabel.setColour(Label::textColourId, sectionsTextColour);
    addAndMakeVisible(midiSectionLabel);

    midiInLabel.setBounds(400, 20, 200, 30);
    midiInLabel.setText("Input device :", NO);
    midiInLabel.setFont(menusTextFont);
    midiInLabel.setColour(Label::textColourId, menusTextColour);
    addAndMakeVisible(midiInLabel);

    midiOutLabel.setBounds(400, 55, 200, 30);
    midiOutLabel.setText("Output device :", NO);
    midiOutLabel.setFont(menusTextFont);
    midiOutLabel.setColour(Label::textColourId, menusTextColour);
    addAndMakeVisible(midiOutLabel);

    // ==================== Program Name =====================
    programTitleLabel.setBounds(10, 5, 395, 30);
    programTitleLabel.setText("Current   Program   =", NO);

    programNameLabel.setBounds(210, 5, 300, 30);
    programNameLabel.setText("______", NO);

    // ==================== MIDI Section =====================

    statusTitleLabel.setBounds(398, 5, 250, 30);
    statusTitleLabel.setText("5tatu5    =", NO);
    statusSection.addAndMakeVisible(statusTitleLabel);

    statusLabel.setBounds(560, 5, 300, 30);
    statusLabel.setText("Di5connected", NO);
    statusSection.addAndMakeVisible(statusLabel);

    modelLabel.setBounds(580, 5, 300, 30);
    modelLabel.setText("", NO);
    statusSection.addChildComponent(modelLabel);

    disconnectedUnderline.setBounds(560, 10, 300, 30);
    disconnectedUnderline.setText("____________", NO);
    statusSection.addChildComponent(disconnectedUnderline);
    sysexDisabledUnderline.setBounds(500, 10, 300, 30);
    sysexDisabledUnderline.setText("_______________________", NO);
    statusSection.addChildComponent(sysexDisabledUnderline);

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

    // ========================== Illegal parameter values controls ==========================

    // ------------------ OSC Octaves and semitones ------------------

    // Label for the OSC1
    osc1Label.setBounds(10, 100, 125, 25);
    osc1Label.setText("O5C 1", NO);
    programSection.addAndMakeVisible(osc1Label);

    // Label for the OSC2
    osc2Label.setBounds(10, 130, 125, 25);
    osc2Label.setText("O5C 2", NO);
    programSection.addAndMakeVisible(osc2Label);

    // Label for the OSC3
    osc3Label.setBounds(10, 160, 125, 25);
    osc3Label.setText("O5C 3", NO);
    programSection.addAndMakeVisible(osc3Label);

    // Label for the wave column
    waveLabel.setBounds(90, 70, 125, 25);
    waveLabel.setText("WAVEFORM", NO);
    programSection.addAndMakeVisible(waveLabel);

    // Label for the octave column
    octLabel.setBounds(350, 70, 125, 25);
    octLabel.setText("OCT", NO);
    programSection.addAndMakeVisible(octLabel);

    // Label for the semitone column
    semiLabel.setBounds(480, 70, 125, 25);
    semiLabel.setText("5EMI", NO);
    programSection.addAndMakeVisible(semiLabel);

    LFLabel.setBounds(565, 70, 125, 25);
    LFLabel.setText("LF", NO);
    programSection.addAndMakeVisible(LFLabel);

    // Menu to select the OSC1 wave
    osc1WaveMenu.setBounds(90, 100, 230, 25);
    osc1WaveMenu.setTooltip("Waveform for oscillator 1:\nStarts with normal waveforms, then goes into hidden waveforms if supported.");
    programControls.addAndMakeVisible(osc1WaveMenu);
    osc1WaveMenu.onChange = [this] {
        updateStatus(DeviceResponse(STATUS_MESSAGES[MODIFYING_PROGRAM]));
        Thread::launch([this] {
            auto status = midiProcessor.changeOscWaveform(OSC1, osc1WaveMenu.getSelectedItemIndex() + NB_OF_WAVES[getCurrentSynthModel()] - 1);
            MessageManager::callAsync([this, status] { updateStatus(status); });
        });
    };

    // Menu to select the OSC2 wave
    osc2WaveMenu.addItemList(waveMenuOpts, 1);
    osc2WaveMenu.setBounds(90, 130, 230, 25);
    osc2WaveMenu.setSelectedItemIndex(0, NO);
    osc2WaveMenu.setTooltip("Waveform for oscillator 2:\nStarts with normal waveforms, then goes into hidden waveforms if supported.");
    programControls.addAndMakeVisible(osc2WaveMenu);
    osc2WaveMenu.onChange = [this] {
        updateStatus(DeviceResponse(STATUS_MESSAGES[MODIFYING_PROGRAM]));
        Thread::launch([this] {
            auto status = midiProcessor.changeOscWaveform(OSC2, osc2WaveMenu.getSelectedItemIndex() + NB_OF_WAVES[getCurrentSynthModel()] - 1);
            MessageManager::callAsync([this, status] { updateStatus(status); });
        });
    };

    // Menu to select the OSC3 wave
    osc3WaveMenu.addItemList(waveMenuOpts, 1);
    osc3WaveMenu.setBounds(90, 160, 230, 25);
    osc3WaveMenu.setSelectedItemIndex(0, NO);
    osc3WaveMenu.setTooltip("Waveform for oscillator 3:\nStarts with normal waveforms, then goes into hidden waveforms if supported.");
    programControls.addAndMakeVisible(osc3WaveMenu);
    osc3WaveMenu.onChange = [this] {
        updateStatus(DeviceResponse(STATUS_MESSAGES[MODIFYING_PROGRAM]));
        Thread::launch([this] {
            auto status = midiProcessor.changeOscWaveform(OSC3, osc3WaveMenu.getSelectedItemIndex() + NB_OF_WAVES[getCurrentSynthModel()] - 1);
            MessageManager::callAsync([this, status] { updateStatus(status); });
        });
    };

    // Menu to select the OSC1 octave
    StringArray oscOctaveMenuOptions = {"-3   to   +5", "+6", "+7"};
    osc1OctMenu.addItemList(oscOctaveMenuOptions, 1);
    osc1OctMenu.setBounds(350, 100, 125, 25);
    osc1OctMenu.setSelectedItemIndex(0, NO);
    osc1OctMenu.setTooltip("Octave for oscillator 1:\nNormal range is -3 to +5, while +6 and +7 are extended values.");
    programControls.addAndMakeVisible(osc1OctMenu);
    osc1OctMenu.onChange = [this] {
        updateStatus(DeviceResponse(STATUS_MESSAGES[MODIFYING_PROGRAM]));
        Thread::launch([this] {
            auto status = midiProcessor.changeOscPitch(OSC1, osc1OctMenu.getSelectedItemIndex() + 5, osc1SemiMenu.getSelectedItemIndex(), osc1LFButton.getToggleState());
            MessageManager::callAsync([this, status] { updateStatus(status); });
        });
    };

    // Menu to select the OSC2 octave
    osc2OctMenu.addItemList(oscOctaveMenuOptions, 1);
    osc2OctMenu.setBounds(350, 130, 125, 25);
    osc2OctMenu.setSelectedItemIndex(0, NO);
    osc2OctMenu.setTooltip("Octave for oscillator 2:\nNormal range is -3 to +5, while +6 and +7 are extended values.");
    programControls.addAndMakeVisible(osc2OctMenu);
    osc2OctMenu.onChange = [this] {
        updateStatus(DeviceResponse(STATUS_MESSAGES[MODIFYING_PROGRAM]));
        Thread::launch([this] {
            auto status = midiProcessor.changeOscPitch(OSC2, osc2OctMenu.getSelectedItemIndex() + 5, osc2SemiMenu.getSelectedItemIndex(), osc2LFButton.getToggleState());
            MessageManager::callAsync([this, status] { updateStatus(status); });
        });
    };

    // Menu to select the OSC3 octave
    osc3OctMenu.addItemList(oscOctaveMenuOptions, 1);
    osc3OctMenu.setBounds(350, 160, 125, 25);
    osc3OctMenu.setSelectedItemIndex(0, NO);
    osc3OctMenu.setTooltip("Octave for oscillator 3:\nNormal range is -3 to +5, while +6 and +7 are extended values.");
    programControls.addAndMakeVisible(osc3OctMenu);
    osc3OctMenu.onChange = [this] {
        updateStatus(DeviceResponse(STATUS_MESSAGES[MODIFYING_PROGRAM]));
        Thread::launch([this] {
            auto status = midiProcessor.changeOscPitch(OSC3, osc3OctMenu.getSelectedItemIndex() + 5, osc3SemiMenu.getSelectedItemIndex(), osc3LFButton.getToggleState());
            MessageManager::callAsync([this, status] { updateStatus(status); });
        });
    };

    // Menu to select the OSC1 semitone
    StringArray oscSemitoneOptions = {"+0", "+1", "+2", "+3", "+4", "+5", "+6", "+7", "+8", "+9", "+10", "+11"};
    osc1SemiMenu.addItemList(oscSemitoneOptions, 1);
    osc1SemiMenu.setBounds(480, 100, 70, 25);
    osc1SemiMenu.setSelectedItemIndex(0, NO);
    osc1SemiMenu.setEnabled(true);
    osc1SemiMenu.setTooltip("Semitone for oscillator 1");
    programControls.addAndMakeVisible(osc1SemiMenu);
    osc1SemiMenu.onChange = [this] {
        updateStatus(DeviceResponse(STATUS_MESSAGES[MODIFYING_PROGRAM]));
        Thread::launch([this] {
            auto status = midiProcessor.changeOscPitch(OSC1, osc1OctMenu.getSelectedItemIndex() + 5, osc1SemiMenu.getSelectedItemIndex(), osc1LFButton.getToggleState());
            MessageManager::callAsync([this, status] { updateStatus(status); });
        });
    };

    // Add a menu to select the OSC2 semitone
    osc2SemiMenu.addItemList(oscSemitoneOptions, 1);
    osc2SemiMenu.setBounds(480, 130, 70, 25);
    osc2SemiMenu.setSelectedItemIndex(0, NO);
    osc2SemiMenu.setEnabled(true);
    osc2SemiMenu.setTooltip("Semitone for oscillator 2");
    programControls.addAndMakeVisible(osc2SemiMenu);
    osc2SemiMenu.onChange = [this] {
        updateStatus(DeviceResponse(STATUS_MESSAGES[MODIFYING_PROGRAM]));
        Thread::launch([this] {
            auto status = midiProcessor.changeOscPitch(OSC2, osc2OctMenu.getSelectedItemIndex() + 5, osc2SemiMenu.getSelectedItemIndex(), osc2LFButton.getToggleState());
            MessageManager::callAsync([this, status] { updateStatus(status); });
        });
    };

    // Add a menu to select the OSC3 semitone
    osc3SemiMenu.addItemList(oscSemitoneOptions, 1);
    osc3SemiMenu.setBounds(480, 160, 70, 25);
    osc3SemiMenu.setSelectedItemIndex(0, NO);
    osc3SemiMenu.setEnabled(true);
    osc3SemiMenu.setTooltip("Semitone for oscillator 3");
    programControls.addAndMakeVisible(osc3SemiMenu);
    osc3SemiMenu.onChange = [this] {
        updateStatus(DeviceResponse(STATUS_MESSAGES[MODIFYING_PROGRAM]));
        Thread::launch([this] {
            auto status = midiProcessor.changeOscPitch(OSC3, osc3OctMenu.getSelectedItemIndex() + 5, osc3SemiMenu.getSelectedItemIndex(), osc3LFButton.getToggleState());
            MessageManager::callAsync([this, status] { updateStatus(status); });
        });
    };

    // Button to toggle OSC1 low-frequency range
    osc1LFButton.setBounds(570, 102, 20, 20);
    osc1LFButton.setTooltip("Low-Frequency mode:\nShifts the whole frequency range down by a couple of octaves for oscillator 1 when enabled.");
    programControls.addAndMakeVisible(osc1LFButton);
    osc1LFButton.onClick = [this] {
        updateStatus(DeviceResponse(STATUS_MESSAGES[MODIFYING_PROGRAM]));
        Thread::launch([this] {
            auto status = midiProcessor.toggleLowFrequencyMode(OSC1, osc1LFButton);
            MessageManager::callAsync([this, status] { updateStatus(status); });
        });
        osc1OctMenu.setSelectedItemIndex(0, NO);
    };

    // Button to toggle OSC2 low-frequency range
    osc2LFButton.setBounds(570, 132, 20, 20);
    osc2LFButton.setTooltip("Low-Frequency mode:\nShifts the whole frequency range down by a couple of octaves for oscillator 2 when enabled.");
    programControls.addAndMakeVisible(osc2LFButton);
    osc2LFButton.onClick = [this] {
        updateStatus(DeviceResponse(STATUS_MESSAGES[MODIFYING_PROGRAM]));
        Thread::launch([this] {
            auto status = midiProcessor.toggleLowFrequencyMode(OSC2, osc2LFButton);
            MessageManager::callAsync([this, status] { updateStatus(status); });
        });
        osc2OctMenu.setSelectedItemIndex(0, NO);
    };

    // Button to toggle OSC3 low-frequency range
    osc3LFButton.setBounds(570, 162, 20, 20);
    osc3LFButton.setTooltip("Low-Frequency mode:\nShifts the whole frequency range down by a couple of octaves for oscillator 3 when enabled.");
    programControls.addAndMakeVisible(osc3LFButton);
    osc3LFButton.onClick = [this] {
        ;
        updateStatus(DeviceResponse(STATUS_MESSAGES[MODIFYING_PROGRAM]));
        Thread::launch([this] {
            auto status = midiProcessor.toggleLowFrequencyMode(OSC3, osc3LFButton);
            MessageManager::callAsync([this, status] { updateStatus(status); });
        });
        osc3OctMenu.setSelectedItemIndex(0, NO);
    };

    // ------------------ Filter self-oscillation ------------------

    // Button to toggle self-oscillation
    selfOscLabel.setBounds(610, 102, 250, 20);
    selfOscLabel.setText("Filt    5elf-Osc", NO);
    programSection.addAndMakeVisible(selfOscLabel);

    selfOscButton.setBounds(680, 130, 20, 20);
    selfOscButton.setTooltip("Filter self-oscillation:\nShifts the whole resonance range internally up to 32-63 when enabled.");
    programControls.addAndMakeVisible(selfOscButton);

    selfOscButton.onClick = [this] {
        updateStatus(DeviceResponse(STATUS_MESSAGES[MODIFYING_PROGRAM]));
        Thread::launch([this] {
            auto status = midiProcessor.toggleSelfOscillation(selfOscButton);
            MessageManager::callAsync([this, status] { updateStatus(status); });
        });
    };

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
            AlertWindow::showMessageBoxAsync(AlertWindow::NoIcon, "SideQick", "Ensoniq SQ-80/ESQ-1 Expansion Software\nVersion 0.42\n\nCopyright Vincent Zauhar, 2024\nReleased under the GNU GPL v3 license\n\nhttps://github.com/VincyZed/SideQick");
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