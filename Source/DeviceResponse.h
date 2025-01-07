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
#include <JuceHeader.h>

using namespace juce;

enum Status { CONNECTED, DISCONNECTED, SYSEX_DISABLED, MODIFYING_PROGRAM, REFRESHING };
const StringArray STATUS_MESSAGES = {"Connected", "Di5connected", "5y5ex    Di5abled", "Modifying    Program    .    .    .", "Refre5hing    .    .    ."};
enum SynthModel { SQ80, ESQ1, ESQM, SQ80M, UNKNOWN, UNCHANGED };
const StringArray SYNTH_MODELS = {"5Q-80", "E5Q-1", "E5Q-M", "5Q80M", "Unknown", "Unchanged"};
const unsigned int NB_OF_WAVES[4] = {75, 32, 32, 75};

const int SQ_ESQ_FAMILY_ID = 0x02;
// Model codes. The SQ-80M has the same code as the ESQ-M
const int ESQ1_ID = 0x01;
const int ESQM_ID = 0x02;
const int SQ80_ID = 0x03;

// We subtract 2 to exclude the SysEx header and footer
const int DEVICE_ID_SIZE = 15 - 2;

// Indexes for corresponding nibbles in the SysEx message, we subtracted 1 for all of these to remove the SysEx header
const int FAMILY_IDX = 5;
const int MODEL_IDX = 7;
const int OS_VERSION_IDX[2] = {11, 12};


class DeviceResponse {
  public:
    String status = STATUS_MESSAGES[DISCONNECTED];
    unsigned int model = UNKNOWN;
    String majorOsVersion;
    String minorOsVersion;
    bool supportsHiddenWaves = true;

    MidiMessage currentProgram;

    // This constructor should be called when we set the status to Disconnected
    DeviceResponse(String status) {
        this->status = status;
        this->model = UNCHANGED;
    }
    DeviceResponse(String status, MidiMessage currentProgram) {
        this->status = status;
        this->model = UNCHANGED;
        this->currentProgram = currentProgram;
    }
    // This constructor should be called when we set the status to Connected or Sysex Disabled
    DeviceResponse(String status, MidiMessage deviceIdMessage, MidiMessage currentProgram) {
        this->status = status;
        this->currentProgram = currentProgram;

        const uint8_t* deviceIdData = deviceIdMessage.getSysExData();
        // Check if a supported model responded to the DeviceInquiry request
        if (deviceIdMessage.getSysExDataSize() == DEVICE_ID_SIZE) {

            majorOsVersion = static_cast<String>(deviceIdData[OS_VERSION_IDX[MAJOR]]);
            minorOsVersion = static_cast<String>(deviceIdData[OS_VERSION_IDX[MINOR]]);
            unsigned int combinedOsVersion = deviceIdData[OS_VERSION_IDX[MAJOR]] * 100 + deviceIdData[OS_VERSION_IDX[MINOR]];

            if (deviceIdData[FAMILY_IDX] == SQ_ESQ_FAMILY_ID) {
                if (deviceIdData[MODEL_IDX] == ESQ1_ID) {
                    model = ESQ1;
                    // An ESQ-1 with OS 3.53 will unfortunately report 3.50, so the tooltip must indicate that both are possible
                    minorOsVersion = majorOsVersion == "3" && minorOsVersion == "50" ? "50/53" : minorOsVersion;
                } else if (deviceIdData[MODEL_IDX] == ESQM_ID)
                    // After looking at the SQ-80M OS's assembly, an SQ-80M with os v1.30 will report 1.30. We can thus distinguish the ESQ-M from the SQ-80M!
                    combinedOsVersion < 130 ? model = ESQM : model = SQ80M;
                else if (deviceIdData[MODEL_IDX] == SQ80_ID) {
                    model = SQ80;
                    // An SQ-80 with OS 1.81 will unfortunately report 1.80, so the tooltip must indicate that both are possible
                    minorOsVersion = majorOsVersion == "1" && minorOsVersion == "80" ? "80/81" : minorOsVersion;
                } else
                    model = UNKNOWN;
            } else
                model = UNKNOWN;

            // TODO: Check this logic in case the ESQ-M or SQ-80M supports hidden waves
            if (model == ESQ1 && combinedOsVersion < ESQ1_HIDDEN_WAVES_MIN_VERSION || model == ESQM || model == SQ80M)
                supportsHiddenWaves = false;

        } else if (deviceIdMessage.getSysExDataSize() == 0) {
            // ESQ-1 (since we already received a valid program dump but not an inquiry response) with OS version lower than 3.00 (we can't know the exact version)
            model = ESQ1;
            majorOsVersion = "< 3";
            minorOsVersion = "00";
        }
    }

  private:
    enum VersionNumber { MINOR, MAJOR };
    const unsigned int ESQ1_HIDDEN_WAVES_MIN_VERSION = 350;
};
