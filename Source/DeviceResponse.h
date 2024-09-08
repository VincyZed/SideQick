#pragma once
#include <JuceHeader.h>

using namespace juce;

enum Status { CONNECTED, DISCONNECTED, SYSEX_DISABLED, MODIFYING_PROGRAM, REFRESHING };
const StringArray STATUS_MESSAGES = { "Connected", "Di5connected", "5y5ex    Di5abled", "Modifying    Program    .    .    .", "Refre5hing    .    .    ." };
enum SynthModel { SQ80, ESQ1, UNKNOWN, UNCHANGED };
const StringArray SYNTH_MODELS = { "5Q-80", "E5Q-1", "Unknown", "Unchanged" };
const unsigned int NB_OF_WAVES[2] = { 75, 32 };

const unsigned int ESQ1_HIDDEN_WAVES_MIN_VERSION = 350;

class DeviceResponse
{
public:

    // Indexes in the SysEx message, we subtracted 1 for all of these to remove the SysEx header
    const int ENSONIQ_FAMILY = 5;
    const int ENSONIQ_MODEL = 7;
    const int OS_VERSION[2] = { 11, 12 };

    enum VersionNumber { MINOR, MAJOR };

    String status = STATUS_MESSAGES[DISCONNECTED];
    unsigned int model = UNKNOWN;
    unsigned int osVersion = 0;

    //const uint8_t* currentProgramData;
    MidiMessage currentProgram;

    // This constructor should be called when we set the status to Disconnected
    DeviceResponse(String status)
    {
        this->status = status;
        this->model = UNCHANGED;
    }
    DeviceResponse(String status, MidiMessage currentProgram)
    {
        this->status = status;
        this->model = UNCHANGED;
        this->currentProgram = currentProgram;
    }
    // This constructor should be called when we set the status to Connected or Sysex Disabled
    DeviceResponse(String status, MidiMessage deviceIdMessage, MidiMessage currentProgram)
    {
        this->status = status;
        this->currentProgram = currentProgram;

        const uint8_t* deviceIdData = deviceIdMessage.getSysExData();

        // TODO: Replace the literal values with constants
        if (deviceIdData[ENSONIQ_FAMILY] == 0x02 && deviceIdData[ENSONIQ_MODEL] == 0x03)
            this->model = SQ80;
        else if (deviceIdData[ENSONIQ_FAMILY] == 0x02 && deviceIdData[ENSONIQ_MODEL] == 0x01)
            this->model = ESQ1;
        else
            this->model = UNKNOWN;

        // This returns an integer corresponding to the OS version, e.g. 353 for version 3.53
        this->osVersion = deviceIdData[OS_VERSION[MAJOR]] * 100 + deviceIdData[OS_VERSION[MINOR]];      
    }
};
