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

class PannelButton : public ShapeButton {
  public:
    NotificationType NO = NotificationType::dontSendNotification;

    float buttonWidth = 40.0f;
    float buttonHeight = 25.0f;
    float cornerSize = 3.0f;

    Colour buttonTextColour = Colour::fromRGB(192, 192, 192);

    PannelButton(const Colour& color, const String& text, const int& xPos, const int& yPos);
    PannelButton();
    ~PannelButton() override;
    void paint(Graphics& g) override;
    void changeColour(const Colour& newColour);

  private:
    Label label;
    DropShadow shadow;
    Path buttonShape;
};