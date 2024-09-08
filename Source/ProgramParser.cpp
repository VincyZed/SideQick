#include "ProgramParser.h"
//#include "MidiSysexProcessor.h"

using namespace juce;

ProgramParser::ProgramParser(MidiMessage program, SynthModel currentModel)
{
    const uint8* progData = program.getSysExData();

    // Get the waveform index for each oscillator
    auto parseWave = [this, progData, currentModel](int oscNumber) {
        auto firstWaveNibble = WAVE[oscNumber][0];
        auto secondWaveNibble = WAVE[oscNumber][1];
        auto waveIndex =  progData[firstWaveNibble] | (progData[secondWaveNibble] << 4);

        return waveIndex >= NB_OF_WAVES[currentModel] ? waveIndex - NB_OF_WAVES[currentModel] + 1 : 0;
    };
   
    // TODO: Make this whole current currentOscOctave/Semitone thing clearer

    // Octave, Semitone and Low Frequency Mode controls for each oscillator
    for (int osc = 0; osc < 3; osc++)
	{
        currentWave[osc] = parseWave(osc);

        // Total number of semitones from OCT-3 to the current pitch value
        // We combine the two nibbles to get the total number of semitones
        auto totalNbSemi = progData[PITCH[osc][0]] | (progData[PITCH[osc][1]] << 4);

        currentOscLF[osc] = totalNbSemi > MAX_SEMI_NORMAL_RANGE;

        currentRealOct[osc] = currentOscLF[osc] ? (totalNbSemi + 4) / 12 : totalNbSemi / 12;
        currentRealSemi[osc] = currentOscLF[osc] ? (totalNbSemi - 8) % 12 : totalNbSemi % 12;

        currentOct[osc] = currentRealOct[osc] % 11 < 9 ? 0 : currentRealOct[osc] % 11 >= 9 && currentRealOct[osc] % 11 < 10 ? 1 : 2;
        currentSemi[osc] = totalNbSemi;
	}
    // Filter resonance (self-oscillation)
    currentSelfOsc = progData[RES[1]] > 1;
}