 # SideQick

SideQick is a software companion to be used with Ensoniq SQ-80 or ESQ-1 hardware synthesizers. Its purpose is to extend the functionnality of those units, mainly by enabling access to illegal (out of bound) values for some parameters found in these synthesizers.


# Features

- Access to illegal parameter values for the following parameters:
    - Hidden waveforms: especially useful if not using the patched [1.83](http://www.buchty.net/~buchty/sq80/customize.html) SQ-80 or [3.53](http://www.buchty.net/~buchty/esq1/customize.html) ESQ-1 OS.
    - Oscillator pitch: allows for extreme oscillator pitch settings (OCT +6 and +7, and a whole new Low-Frequency range)
    - Higher filter resonance: enables filter self-oscillation


# About Illegal Parameter Values

Upon sending programs to the synth or recalling them, the bounds for some parameter values are not enforced by its operating system. Even though parameter values are contained within a certain range when programming a sound on the unit itself, nothing prevents us from modifying the raw SysEx data of a program to input any value for such parameters, as long as they are within the [internal width allocated to that parameter](http://www.synthmanuals.com/manuals/ensoniq/sq-80/owners_manual/sq80ownersmanual.pdf#page=214).

While some features are hardware-limited or are linked to deeper restrictions baked in the operating system (such as OSC Sync and AM not being able to be enabled at the same time, a real shame), others are not and are supported by the hardware, such as allowing higher filter resonance values for example.
Although this is a relatively unknown trick, there are a few demonstrations out there such as [this one](https://www.youtube.com/watch?v=Usa-v3nnpAU) that showcase the result of manually editing illegal parameter values.

As a reminder, from a perspective of loading and saving programs into internal memory, on a cartridge or on disk, the operating system will treat parameters with illegal values just like the others. Therefore those illegal values can be included into saved programs and **should be compatible with any SQ-80 or ESQ-1**.


# How It Works

SideQick is a C++ standalone application made with the [JUCE](https://juce.com/) framework. Since the goal of this project is the seamless integration with the process of programming on the hardware, you can treat the options on SideQick's display as if they were additional options in the synth itself, hence the software's somewhat skeuomorphic design.

Changes to the current program are thus applied and directly sent to the synth. To achieve this, when clicking on an option in SideQick, the program being currently edited will be requested by the computer by sending a [Current Program Dump Request](http://www.synthmanuals.com/manuals/ensoniq/sq-80/owners_manual/sq80ownersmanual.pdf#page=204) SysEx message. Once the SysEx response containing the program is received, changes to the program data are made accordingly to the selected option, then the resulting program is sent back to the SQ-80 / ESQ-1.


# Requirements
- A Windows, macOS or Linux computer (macOS and Linux are still untested but should both work);
- An Ensoniq SQ-80 or ESQ-1 hardware synthesizer connected in MIDI to the computer bidirectionaly;
    - Make sure to enable SysEx on the unit by going into MIDI, then setting **KEYS+CT+PC+SS+SX** to ENABLE.
    - On the ESQ-1, OS version 3.5+ is required to access hidden waveforms, even through SideQick. More on that [here](http://buchty.net/ensoniq/hidden-wave.html).

# Additional Planned Features:
- Seamless patch management: Easy to use program/bank request, loading and saving (.syx)