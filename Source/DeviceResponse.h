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
const StringArray SYNTH_MODELS = {"5Q-80", "E5Q-1", "E5Q-M", "5Q-80M", "Unknown", "Unchanged"};
const unsigned int NB_OF_WAVES[4] = {75, 32, 32, 75};

const unsigned int ESQ1_HIDDEN_WAVES_MIN_VERSION = 350;

const int SQ_ESQ_FAMILY_ID = 0x02;
// Model codes. The SQ-80M has the same code as the ESQ-M
const int ESQ1_ID = 0x01;
const int ESQM_ID = 0x02;
const int SQ80_ID = 0x03;


class DeviceResponse {
  public:
    String status = STATUS_MESSAGES[DISCONNECTED];
    unsigned int model = UNKNOWN;
    unsigned int osVersion = 0;
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

        // OS version, e.g. 353 for version 3.53
        this->osVersion = deviceIdData[OS_VERSION[MAJOR]] * 100 + deviceIdData[OS_VERSION[MINOR]];

        if (deviceIdData[ENSONIQ_FAMILY] == SQ_ESQ_FAMILY_ID) {
            if (deviceIdData[ENSONIQ_MODEL] == ESQ1_ID)
                model = ESQ1;
            else if (deviceIdData[ENSONIQ_MODEL] == ESQM_ID)
                osVersion < 130 ? model = ESQM : model = SQ80M;
            else
                deviceIdData[ENSONIQ_MODEL] == SQ80_ID ? model = SQ80 : model = UNKNOWN;
        } else
            model = UNKNOWN;

        // TODO: Check this logic in case the ESQ-M or SQ-80M supports hidden waves
        if (model == ESQ1 && osVersion < ESQ1_HIDDEN_WAVES_MIN_VERSION || model == ESQM || model == SQ80M)
            supportsHiddenWaves = false;
    }

  private:
    // Indexes in the SysEx message, we subtracted 1 for all of these to remove the SysEx header
    const int ENSONIQ_FAMILY = 5;
    const int ENSONIQ_MODEL = 7;
    const int OS_VERSION[2] = {11, 12};

    enum VersionNumber { MINOR, MAJOR };
};
