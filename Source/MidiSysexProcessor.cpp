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

#include "MidiSysexProcessor.h"
#include "DeviceResponse.h"
#include "MainComponent.h"
#include "ProgramParser.h"

using namespace juce;

void MidiSysexProcessor::processIncomingMidiData(MidiInput* source, const MidiMessage& message) {
    if (message.isSysEx())
        // Add the received SysEx message to be processed
        receivedSysExMessages.add(message);
}

DeviceResponse MidiSysexProcessor::requestDeviceInquiry() {
    if (selectedMidiOut != nullptr) {
        selectedMidiOut->sendMessageNow(MidiMessage::createSysExMessage(REQUEST_ID, sizeof(REQUEST_ID)));
        Thread::sleep(SYSEX_DELAY);

        // There may be more than one device that responds to the DeviceInquiry request, since it's part of the MIDI standard.
        // We need to get all the responses and check if any of them are from an SQ-80 or ESQ-1.
        Array<MidiMessage> deviceIdMessages = receivedSysExMessages;
        receivedSysExMessages.clear();

        auto verifySysexEnabled = [this](MidiMessage deviceIdMessage) {
            auto currentProg = requestProgramDump();
            if (currentProg.getSysExDataSize() == SQ_ESQ_PROG_SIZE)
                return DeviceResponse(STATUS_MESSAGES[CONNECTED], deviceIdMessage, currentProg);
            else {
                return DeviceResponse(STATUS_MESSAGES[SYSEX_DISABLED], deviceIdMessage, currentProg);
            }
        };

        Array<MidiMessage> sqEsqMessages;

        // Check if we are connected to an ESQ-1
        for (auto deviceIdMessage : deviceIdMessages) {
            if (deviceIdMessage.getSysExDataSize() == DEVICE_ID_SIZE) {
                const uint8_t* deviceIdData = deviceIdMessage.getSysExData();
                if (deviceIdData[FAMILY] == SQ_ESQ_FAMILY_ID) {
                    // If we find an ESQ-1, we don't need to check for the SQ-80 because the ESQ-1 has more hidden waves.
                    if (deviceIdData[MODEL] == ESQ1_ID)
                        return verifySysexEnabled(deviceIdMessage);
                    else if (deviceIdData[MODEL] == SQ80_ID)
                        sqEsqMessages.add(deviceIdMessage);
                }
            }
        }
        // Check if we are connected to an SQ-80 or ESQ-1, and if so, verify if SysEx is enabled
        if (!sqEsqMessages.isEmpty())
            return verifySysexEnabled(sqEsqMessages.getFirst());
        else
            return DeviceResponse(STATUS_MESSAGES[DISCONNECTED]);
    } else
        return DeviceResponse(STATUS_MESSAGES[DISCONNECTED]);
}

MidiMessage MidiSysexProcessor::requestProgramDump() {
    // Send the program dump request
    if (selectedMidiOut != nullptr) {
        selectedMidiOut->sendMessageNow(MidiMessage::createSysExMessage(REQUEST_PGM_DUMP_PT_1, sizeof(REQUEST_PGM_DUMP_PT_1)));
        selectedMidiOut->sendMessageNow(MidiMessage::createSysExMessage(REQUEST_PGM_DUMP_PT_2, sizeof(REQUEST_PGM_DUMP_PT_2)));
    }

    Thread::sleep(SYSEX_DELAY);

    // Read the incoming SysEx message
    auto program = receivedSysExMessages.getFirst();

    // Clear the array of received messages
    receivedSysExMessages.clear();

    return program;
}

void MidiSysexProcessor::sendProgramDump(HeapBlock<uint8_t>& progData) {
    // Create a new SysEx message with the modified data
    MidiMessage modifiedProgram = MidiMessage::createSysExMessage(progData, SQ_ESQ_PROG_SIZE);
    selectedMidiOut->sendMessageNow(modifiedProgram);

    // Create and send a message immediately to press Soft Button 5 (exit program save prompt)
    const unsigned char sb5Data[] = {0xF0, 0x0F, 0x02, 0x00, 0x0E, 0x2F, 0x62, 0xF7};
    MidiMessage sb5Message = MidiMessage::createSysExMessage(sb5Data, sizeof(sb5Data));
    selectedMidiOut->sendMessageNow(sb5Message);
}

DeviceResponse MidiSysexProcessor::changeOscWaveform(int oscNumber, int waveformIndex) {
    auto currentProg = requestProgramDump();
    const uint8_t* progData = currentProg.getSysExData();

    // Check if we received a valid program dump from the synth
    if (currentProg.getSysExDataSize() == SQ_ESQ_PROG_SIZE) {
        // Make a copy of the program data to modify
        HeapBlock<uint8_t> modifiedProgData(SQ_ESQ_PROG_SIZE);
        memcpy(modifiedProgData.getData(), progData, SQ_ESQ_PROG_SIZE);

        // Modify the appropriate nibbles to change the oscillator waveform
        modifiedProgData[ProgramParser::WAVE[oscNumber][0]] = waveformIndex % 16;
        modifiedProgData[ProgramParser::WAVE[oscNumber][1]] = static_cast<uint8_t>(waveformIndex / 16);

        // Send the modified program dump by passing a reference to the modifiedprogData
        sendProgramDump(modifiedProgData);

        // Update the program that will be sent back to the updateStatus method
        MidiMessage modifiedProg = MidiMessage::createSysExMessage(modifiedProgData, SQ_ESQ_PROG_SIZE);
        return DeviceResponse(STATUS_MESSAGES[CONNECTED], modifiedProg);
    } else
        return DeviceResponse(STATUS_MESSAGES[DISCONNECTED]);
}

DeviceResponse MidiSysexProcessor::changeOscPitch(int oscNumber, int octave, int semitone, bool inLowFreqRange) {

    auto currentProg = requestProgramDump();
    const uint8_t* progData = currentProg.getSysExData();

    // Check if we received a valid program dump from the synth
    if (currentProg.getSysExDataSize() == SQ_ESQ_PROG_SIZE) {
        // Make a copy of the program data to modify
        HeapBlock<uint8_t> modifiedProgData(SQ_ESQ_PROG_SIZE);
        memcpy(modifiedProgData.getData(), progData, SQ_ESQ_PROG_SIZE);

        // If we are going to be in illegal range
        if (octave - 5 > 0 || inLowFreqRange) {
            int modifiedTotalSemi = (octave + 3) * 12 + semitone + inLowFreqRange * 128;

            // Save the pitch values for the normal range if we were already in the normal range
            if (ProgramParser(currentProg, UNCHANGED).currentSemi[oscNumber] <= ProgramParser::MAX_SEMI_NORMAL_RANGE) {
                pitchNormal[oscNumber][0] = progData[ProgramParser::PITCH[oscNumber][0]];
                pitchNormal[oscNumber][1] = progData[ProgramParser::PITCH[oscNumber][1]];
            }

            // Calculate the values of the two nibbles to modify
            modifiedProgData[ProgramParser::PITCH[oscNumber][0]] = modifiedTotalSemi % 16;
            modifiedProgData[ProgramParser::PITCH[oscNumber][1]] = static_cast<uint8_t>(modifiedTotalSemi / 16);
        } else {
            auto currentProgSettings = ProgramParser(currentProg, UNCHANGED);
            // If the current program is in the illegal range, we need to revert it to the normal range
            if (currentProgSettings.currentSemi[oscNumber] > 96) {
                // Set to the last known normal range pitch values (by default, +5 OCT and +0 SEMI).
                // Put them back in the normal range in case we were in the illegal range

                if (pitchNormal[oscNumber][0] + pitchNormal[oscNumber][1] * 16 > 96) {
                    pitchNormal[oscNumber][0] = defaultPitchNormal[0];
                    pitchNormal[oscNumber][1] = defaultPitchNormal[1];
                }
                modifiedProgData[ProgramParser::PITCH[oscNumber][0]] = pitchNormal[oscNumber][0];
                modifiedProgData[ProgramParser::PITCH[oscNumber][1]] = pitchNormal[oscNumber][1];
            } else {
                // This means we have changed the oscillator semitone value while in the normal range
                int modifiedTotalSemi = currentProgSettings.currentRealOct[oscNumber] * 12 + semitone + inLowFreqRange * 128;
                modifiedProgData[ProgramParser::PITCH[oscNumber][0]] = modifiedTotalSemi % 16;
                modifiedProgData[ProgramParser::PITCH[oscNumber][1]] = static_cast<uint8_t>(modifiedTotalSemi / 16);
            }
        }

        // Send the modified program dump by passing a reference to the modifiedprogData
        sendProgramDump(modifiedProgData);

        // Update the program that will be sent back to the updateStatus method
        MidiMessage modifiedProg = MidiMessage::createSysExMessage(modifiedProgData, SQ_ESQ_PROG_SIZE);
        return DeviceResponse(STATUS_MESSAGES[CONNECTED], modifiedProg);
    } else
        return DeviceResponse(STATUS_MESSAGES[DISCONNECTED]);
}

DeviceResponse MidiSysexProcessor::toggleLowFrequencyMode(int oscNumber, bool lowFreqEnabled) {
    // OCT+7 SEMI+8 is when the DOC wraps around and generates very low frequencies. It will show on the unit as OCT-3.
    // It sets the oscillator in a different frequency range, a bit like what toggleSelfOscillation() does for resonance.
    // Here we set it to OCT-2 by default because OCT-3 is still a very high frequency but from a different waveform, because... reasons.

    auto currentProg = requestProgramDump();
    const uint8_t* progData = currentProg.getSysExData();

    // Check if we received a valid program dump from the synth
    if (currentProg.getSysExDataSize() == SQ_ESQ_PROG_SIZE) {
        // Make a copy of the program data to modify
        HeapBlock<uint8_t> modifiedprogData(SQ_ESQ_PROG_SIZE);
        memcpy(modifiedprogData.getData(), progData, SQ_ESQ_PROG_SIZE);

        if (lowFreqEnabled) {
            // Save the pitch values for the normal range if we were already in the normal range
            if (ProgramParser(currentProg, UNCHANGED).currentSemi[oscNumber] <= ProgramParser::MAX_SEMI_NORMAL_RANGE) {
                pitchToggleNormal[oscNumber][0] = progData[ProgramParser::PITCH[oscNumber][0]];
                pitchToggleNormal[oscNumber][1] = progData[ProgramParser::PITCH[oscNumber][1]];
            }

            // Calculate the values of the two nibbles to modify
            modifiedprogData[ProgramParser::PITCH[oscNumber][0]] = pitchToggleLowFreq[oscNumber][0];
            modifiedprogData[ProgramParser::PITCH[oscNumber][1]] = pitchToggleLowFreq[oscNumber][1];
        } else {
            // Save the pitch values for the low freq range
            pitchToggleLowFreq[oscNumber][0] = progData[ProgramParser::PITCH[oscNumber][0]];
            pitchToggleLowFreq[oscNumber][1] = progData[ProgramParser::PITCH[oscNumber][1]];

            // Set to normal range pitch values
            modifiedprogData[ProgramParser::PITCH[oscNumber][0]] = pitchToggleNormal[oscNumber][0];
            modifiedprogData[ProgramParser::PITCH[oscNumber][1]] = pitchToggleNormal[oscNumber][1];
        }

        // Send the modified program dump by passing a reference to the modifiedprogData
        sendProgramDump(modifiedprogData);

        // Update the program that will be sent back to the updateStatus method
        MidiMessage modifiedProgram = MidiMessage::createSysExMessage(modifiedprogData, SQ_ESQ_PROG_SIZE);
        return DeviceResponse(STATUS_MESSAGES[CONNECTED], modifiedProgram);
    } else
        return DeviceResponse(STATUS_MESSAGES[DISCONNECTED]);
}

DeviceResponse MidiSysexProcessor::toggleSelfOscillation(ToggleButton& selfOscButton) {
    auto currentProg = requestProgramDump();
    const uint8_t* progData = currentProg.getSysExData();

    // Check if we received a valid program dump from the synth
    if (currentProg.getSysExDataSize() == SQ_ESQ_PROG_SIZE) {
        // Make a copy of the program data to modify (modified program data)
        HeapBlock<uint8_t> modProgData(SQ_ESQ_PROG_SIZE);
        memcpy(modProgData.getData(), progData, SQ_ESQ_PROG_SIZE);

        if (selfOscButton.getToggleState()) {
            // Save the resonance value for the normal state
            resValuesNormal[0] = progData[ProgramParser::RES[0]];
            resValuesNormal[1] = progData[ProgramParser::RES[1]];

            // Modify the appropriate nibbles to shift the filter resonance range upwards
            modProgData[ProgramParser::RES[0]] = resValuesSelfOsc[0];
            modProgData[ProgramParser::RES[1]] = resValuesSelfOsc[1];
        } else {
            // Save the resonance value for the self-oscillating state
            resValuesSelfOsc[0] = progData[ProgramParser::RES[0]];
            resValuesSelfOsc[1] = progData[ProgramParser::RES[1]];

            // Modify the appropriate nibbles to shift the filter resonance range downwards
            modProgData[ProgramParser::RES[0]] = resValuesNormal[0];
            modProgData[ProgramParser::RES[1]] = resValuesNormal[1];
        }

        // Send the modified program dump by passing a reference to the modifiedprogData
        sendProgramDump(modProgData);

        // Update the program that will be sent back to the updateStatus method
        MidiMessage modifiedProgram = MidiMessage::createSysExMessage(modProgData, SQ_ESQ_PROG_SIZE);
        return DeviceResponse(STATUS_MESSAGES[CONNECTED], modifiedProgram);
    } else {
        return DeviceResponse(STATUS_MESSAGES[DISCONNECTED]);
    }
}