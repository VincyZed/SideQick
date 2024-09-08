#pragma once
#include <JuceHeader.h>
#include "DeviceResponse.h"

using namespace juce;

class ProgramParser
{
public:
    int currentWave[3];
    int currentOct[3];
    int currentSemi[3];
    bool currentOscLF[3];
    bool currentSelfOsc;

    int currentRealOct[3];
    int currentRealSemi[3];

    ProgramParser(MidiMessage programData, SynthModel currentModel);

    // Indexes for corresponding nibbles in the SysEx message, we subtracted 1 for all of these to remove the SysEx header
    static constexpr int RES[2] = { 184, 185 };
    static constexpr int WAVE[3][2] = { { 130, 131 }, { 150, 151 }, { 170, 171 } };
    static constexpr int PITCH[3][2] = { { 120, 121 }, { 140, 141 }, { 160, 161 } };
    static const int MAX_SEMI_NORMAL_RANGE = 127;

private:
    enum Oscillators { OSC1, OSC2, OSC3 };
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ProgramParser)
};