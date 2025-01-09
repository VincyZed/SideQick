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
// For the display's ON and OFF states
static const Colour DISPLAY_COLOURS[2] = {Colour::fromRGB(0, 255, 220), Colour::fromRGB(0, 128, 110)};
enum DisplayState { ON, OFF };

class DisplayLookAndFeel : public juce::LookAndFeel_V4 {
  public:
    Colour displayColour;

    DisplayLookAndFeel();
    Typeface::Ptr getCustomTypeface();
    Font getLabelFont(Label& label) override;
    Font getPopupMenuFont() override;
    void drawComboBox(Graphics& g, int width, int height, bool isButtonDown, int buttonX, int buttonY, int buttonW, int buttonH, ComboBox& box) override;
    void drawToggleButton(Graphics& g, ToggleButton& button, bool isMouseOverButton, bool isButtonDown) override;

  private:
    const Colour DISPLAY_BACK_COLOUR = Colour::fromRGB(6, 0, 10);
    Typeface::Ptr customTypeface;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DisplayLookAndFeel)
};

class Display : public GroupComponent {
  public:
    Display();
    ~Display() override;

    void paint(Graphics& g) override;
    void resized() override;

    void toggleProgramSection(DisplayState state);
    void toggleComponent(Component& component, DisplayState state);

  private:
    DisplayLookAndFeel displayLookAndFeel;
    Colour displayBackgroundColour = Colour::fromRGB(6, 0, 10);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Display)
};
