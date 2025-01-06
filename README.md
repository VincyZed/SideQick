 # SideQick

 ![image](https://github.com/user-attachments/assets/476f15c1-da74-4278-9f33-3f358d8b89bc)

SideQick is a software companion to be used with Ensoniq SQ-80 or ESQ-1 hardware synthesizers. Its purpose is to extend the functionnality of those units, mainly by enabling access to illegal (out of bound) values for some parameters found in these synthesizers. This allows for even more possibilities and avenues of sound design exploration.

# Downloads

Downloads for Windows, macOS and Linux are available from the [Releases page](https://github.com/VincyZed/SideQick/releases/latest).

<br>

# Features

As it stands, SideQick provides easy access to illegal value ranges for the following parameters:
- Hidden waveforms: especially useful when not using the patched [1.83](http://www.buchty.net/~buchty/sq80/customize.html) SQ-80 or [3.53](http://www.buchty.net/~buchty/esq1/customize.html) ESQ-1 OS.
- Oscillator pitch: allows for extreme oscillator pitch settings (OCT +6 and +7, and a whole new "Low-Frequency" range)
- Higher filter resonance: enables filter self-oscillation

<br>

# Requirements
- A computer running Windows, macOS or Linux;
- An Ensoniq SQ-80 or ESQ-1 hardware synthesizer connected in MIDI to the computer **bidirectionaly**;
    - Make sure to enable SysEx on the unit by going into MIDI, then setting **KEYS+CT+PC+SS+SX** to ENABLE.
     - On the ESQ-1, currently OS version 3.0 or newer is needed to get recognized by SideQick. Version 3.5 or newer is required to access hidden waveforms, even through SideQick. More on that [here](http://buchty.net/ensoniq/hidden-wave.html).
    - The ESQ-m and [SQ-80m](http://www.buchty.net/ensoniq/files/sq80m.pdf) are currently not supported, but will likely be soon.

 
# Troubleshooting
Known issues are listed [here](https://github.com/VincyZed/SideQick/issues). Otherwise, here are some steps in case you have trouble running SideQick.
## Windows
- You may get a Microsoft Defender SmartScreen warning saying that SideQick is an unrecognized application and might put your PC at risk. Or course, **SideQick is not a virus**, so you can simply click on **More Info**, then **Run Anyway**.
- If you get error messages about missing DLLs, you probably need to install the [Microsoft Visual C++ 2015 - 2022 Redistributable](https://answers.microsoft.com/en-us/windows/forum/all/vcruntime140dll-and-msvcp140dll-missing-in-windows/caf454d1-49f4-4d2b-b74a-c83fb7c38625).

## macOS
As with other software from unverified developers, you may need to [allow applications from **Anywhere**](https://discussions.apple.com/thread/255759797?answerId=260852615022&sortBy=rank#260852615022) to be run. You may also need to run the `xattr -c <path/to/SideQick.app>` command if you get an error message saying that SideQick is *damaged*.

<br>

# About Illegal Parameter Values

Upon sending programs to the synth or recalling them, the bounds for some parameter values are not enforced by its operating system. Even though parameter values are contained within a certain range when programming a sound on the unit itself, nothing prevents us from modifying the raw SysEx data of a program to input any value for such parameters, as long as they are within the [internal width allocated to that parameter](http://www.buchty.net/ensoniq/files/manuals/SQ80.pdf#page=214).

While some features are hardware-limited or are linked to deeper restrictions baked in the operating system (such as OSC Sync and AM not being able to be enabled at the same time, a real shame), others are not and are supported by the hardware, such as allowing higher filter resonance values for example.
Although this is a relatively unknown trick, there are a few demonstrations out there such as [this one](https://www.youtube.com/watch?v=Usa-v3nnpAU) that showcase the result of manually editing hexadecimal values of a SysEx file to achieve filter self-oscillation.

As a reminder, from a perspective of loading and saving programs into internal memory, on a cartridge or on disk, the operating system will treat parameters with illegal values just like the others. Therefore those illegal values can be included into saved programs and **should be compatible with any SQ-80 or ESQ-1**.

# How It Works

SideQick is a C++ standalone application made with the [JUCE](https://juce.com/) framework. Since the goal of this project is the seamless integration with the process of programming on the hardware, you can treat the options on SideQick's display as if they were additional options in the synth itself, hence the software's somewhat skeuomorphic design.

Changes to the current program are thus applied and directly sent to the synth. To achieve this, when clicking on an option in SideQick, the program being currently edited will be requested by the computer by sending a [Current Program Dump Request](http://www.buchty.net/ensoniq/files/manuals/SQ80.pdf#page=204) SysEx message. Once the SysEx response containing the program is received, changes to the program data are made accordingly to the selected option, then the resulting program is sent back to the SQ-80 / ESQ-1.

<br>

# Building SideQick
If you want to build SideQick from source yourself, here's the requirements and necessary steps. Otherwise if you just want to download and run the software, please refer to the [Downloads](#downloads) section.

## Installing dependencies
Some dependencies are required to be able to build SideQick. First off, on any operating system, you will need to have [CMake](https://cmake.org/download/) installed.

### Windows
To build with the MSVC compiler, either the [Visual Studio Build Tools](https://visualstudio.microsoft.com/downloads/?q=build+tools) or [Visual Studio](https://visualstudio.microsoft.com/downloads/) itself is needed. In any case, make sure you then install the `Desktop development with C++` workload from the Visual Studio Installer as well.

### Linux
If you're using a Debian-based distribution, you can install the required dependencies by running this command:

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
- Seamless patch management: Implementation of easy program/bank request, loading and saving (.syx)
