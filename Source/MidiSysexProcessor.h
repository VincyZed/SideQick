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

#include "DeviceResponse.h"
#include <JuceHeader.h>

using namespace juce;

class MidiSysexProcessor {
  public:
    unsigned char requestPgmDumpMsg[6] = {0xF0, 0x0F, 0x02, 0x00, 0x09, 0xF7};
    const int CHANNEL_IDX = 3;

    std::unique_ptr<MidiInput> selectedMidiIn;
    std::unique_ptr<MidiOutput> selectedMidiOut;

    void processIncomingMidiData(MidiInput* source, const MidiMessage& message);

    DeviceResponse requestDeviceInquiry();
    MidiMessage requestProgramDump();
    void sendProgramDump(HeapBlock<uint8_t>& progData);
    DeviceResponse toggleSelfOscillation(ToggleButton& selfOscButton);
    DeviceResponse changeOscWaveform(int oscNumber, int waveformIndex);
    DeviceResponse changeOscPitch(int oscNumber, int octave, int semitone, bool inLowFreqRange);
    DeviceResponse toggleLowFrequencyMode(int oscNumber, bool lowFreqEnabled);

  private:
    Array<MidiMessage> receivedSysExMessages;

    const int SYSEX_DELAY = 700;

    enum VersionNumber { MINOR, MAJOR };

    // We subtract 2 to exclude the SysEx header and footer
    const int SQ_ESQ_PROG_SIZE = 210 - 2;

    const unsigned char REQUEST_ID_MSG[6] = {0xF0, 0x7E, 0x7F, 0x06, 0x01, 0xF7};

    // If we have toggleable ranges, this is to remember the values for each state. The ones here are the default values
    uint8_t resValuesNormal[2] = {0x0, 0x1};
    uint8_t resValuesSelfOsc[2] = {0x0, 0x2};

    // For the octave and semitone settings. Only normal, since the extended range values are taken directly from the current settings
    const uint8_t defaultPitchNormal[2] = {0x0, 0x6};
    uint8_t pitchNormal[3][2] = {defaultPitchNormal[0], defaultPitchNormal[0], defaultPitchNormal[0]};

    // For the low-frequency mode
    uint8_t pitchToggleNormal[3][2] = {{0x4, 0x2}, {0x4, 0x2}, {0x4, 0x2}};
    uint8_t pitchToggleLowFreq[3][2] = {{0xC, 0x8}, {0xC, 0x8}, {0xC, 0x8}};

    DeviceResponse getConnectionStatus(MidiMessage deviceIdMessage);
};