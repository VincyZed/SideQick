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

#include "PannelButton.h"

using namespace juce;

PannelButton::PannelButton(const Colour& color, const String& text, const int& xPos, const int& yPos) : ShapeButton(text, color, color, color) {
    Colour hoverColour = Colour::fromRGB(color.getRed() * 1.2, color.getGreen() * 1.2, color.getBlue() * 1.2);
    Colour pressedColour = Colour::fromRGB(color.getRed() * 0.85, color.getGreen() * 0.85, color.getBlue() * 0.85);
    setColours(color, hoverColour, pressedColour);
    buttonShape.addRoundedRectangle(0, 0, buttonWidth, buttonHeight, cornerSize);
    setShape(buttonShape, true, true, false);
    setBounds(xPos, yPos, buttonWidth, buttonHeight);
    setOutline(Colours::black.withAlpha(0.75f), 1);

    label.setText(text, NO);
    label.setJustificationType(Justification::centred);
    label.attachToComponent(this, true);
    label.setColour(Label::textColourId, buttonTextColour);
    label.setFont(Font(FontOptions(Font::getDefaultSansSerifFontName(), 16.0f, Font::italic)));
    label.setBounds(xPos - buttonWidth / 2, yPos - 20, 2 * buttonWidth, 0.5 * buttonHeight);
}

// Set the default constructor from the ShapeButton class
PannelButton::PannelButton() : ShapeButton("button", Colours::white, Colours::white, Colours::white) {}

PannelButton::~PannelButton() { setLookAndFeel(nullptr); }

void PannelButton::paint(Graphics& g) {
    // Draw the shadow
    Path shadowPath;
    shadow.colour = Colours::black.withAlpha(0.25f);
    shadow.drawForPath(g, buttonShape);

    g.fillPath(buttonShape);
    ShapeButton::paint(g);
}

void PannelButton::changeColour(const Colour& newColour) {
    Colour hoverColour = Colour::fromRGB(newColour.getRed() * 1.2, newColour.getGreen() * 1.2, newColour.getBlue() * 1.2);
    Colour pressedColour = Colour::fromRGB(newColour.getRed() * 0.85, newColour.getGreen() * 0.85, newColour.getBlue() * 0.85);
    setColours(newColour, hoverColour, pressedColour);
}