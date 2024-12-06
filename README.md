 # SideQick

 ![image](https://github.com/user-attachments/assets/476f15c1-da74-4278-9f33-3f358d8b89bc)


SideQick is a software companion to be used with Ensoniq SQ-80 or ESQ-1 hardware synthesizers. Its purpose is to extend the functionnality of those units, mainly by enabling access to illegal (out of bound) values for some parameters found in these synthesizers. This allows for even more possibilities and avenues of sound design exploration.

**Note**: This project is still a work-in-progress. There are still bugs and other small issues to be ironed out.

# Features

As it stands, SideQick provides easy access to illegal value ranges for the following parameters:
- Hidden waveforms: especially useful when not using the patched [1.83](http://www.buchty.net/~buchty/sq80/customize.html) SQ-80 or [3.53](http://www.buchty.net/~buchty/esq1/customize.html) ESQ-1 OS.
- Oscillator pitch: allows for extreme oscillator pitch settings (OCT +6 and +7, and a whole new "Low-Frequency" range)
- Higher filter resonance: enables filter self-oscillation


# About Illegal Parameter Values

Upon sending programs to the synth or recalling them, the bounds for some parameter values are not enforced by its operating system. Even though parameter values are contained within a certain range when programming a sound on the unit itself, nothing prevents us from modifying the raw SysEx data of a program to input any value for such parameters, as long as they are within the [internal width allocated to that parameter](http://www.buchty.net/ensoniq/files/manuals/SQ80.pdf#page=214).

While some features are hardware-limited or are linked to deeper restrictions baked in the operating system (such as OSC Sync and AM not being able to be enabled at the same time, a real shame), others are not and are supported by the hardware, such as allowing higher filter resonance values for example.
Although this is a relatively unknown trick, there are a few demonstrations out there such as [this one](https://www.youtube.com/watch?v=Usa-v3nnpAU) that showcase the result of manually editing hexadecimal values of a SysEx file to achieve filter self-oscillation.

As a reminder, from a perspective of loading and saving programs into internal memory, on a cartridge or on disk, the operating system will treat parameters with illegal values just like the others. Therefore those illegal values can be included into saved programs and **should be compatible with any SQ-80 or ESQ-1**.


# How It Works

SideQick is a C++ standalone application made with the [JUCE](https://juce.com/) framework. Since the goal of this project is the seamless integration with the process of programming on the hardware, you can treat the options on SideQick's display as if they were additional options in the synth itself, hence the software's somewhat skeuomorphic design.

Changes to the current program are thus applied and directly sent to the synth. To achieve this, when clicking on an option in SideQick, the program being currently edited will be requested by the computer by sending a [Current Program Dump Request](http://www.buchty.net/ensoniq/files/manuals/SQ80.pdf#page=204) SysEx message. Once the SysEx response containing the program is received, changes to the program data are made accordingly to the selected option, then the resulting program is sent back to the SQ-80 / ESQ-1.


# Requirements
- A Windows or Linux computer (no testing has been done yet for macOS. Theoretically it should/could work, but I'm still sorting out some issues. More to come...);
- An Ensoniq SQ-80 or ESQ-1 hardware synthesizer connected in MIDI to the computer **bidirectionaly**;
    - Make sure to enable SysEx on the unit by going into MIDI, then setting **KEYS+CT+PC+SS+SX** to ENABLE.
    - On the ESQ-1, OS version 3.5 or newer is required to access hidden waveforms, even through SideQick. More on that [here](http://buchty.net/ensoniq/hidden-wave.html).

# Troubleshooting
If you run SideQick on Windows and get error messages about missing DLLs, you probably have this issue and simply need to install [this](https://answers.microsoft.com/en-us/windows/forum/all/vcruntime140dll-and-msvcp140dll-missing-in-windows/caf454d1-49f4-4d2b-b74a-c83fb7c38625).

# Building SideQick

## Installing dependencies

### Linux
On linux, some dependencies are required to be able to build the project. If you're using a Debian-based distribution, you can install them by running this command:

```
sudo apt-get update && sudo apt-get install -y build-essential cmake pkg-config libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libfreetype6-dev libfontconfig1-dev libasound2-dev
```

## Building with CMake
Once your environment has all the necessary dependencies, a reliable way to build SideQick on all platforms would be:

```
git clone https://github.com/VincyZed/SideQick
cd SideQick
git submodule update --init --recursive
cmake -B cmake-build
cmake --build cmake-build --config Release
```

# Next on the To Do List:
- Full compatibility with and binary generation for macOS
- More documentation about building on different platforms
- Seamless patch management: Implementation of easy program/bank request, loading and saving (.syx)
