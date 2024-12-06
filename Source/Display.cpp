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

#include "Display.h"

using namespace juce;

Display::Display() { setLookAndFeel(&displayLookAndFeel); }

Display::~Display() { setLookAndFeel(nullptr); }

void Display::paint(Graphics& g) {
    float cornerRadius = 8.0f;
    g.setColour(displayBackgroundColour);

    // Draw a rounded rectangle
    g.fillRoundedRectangle(getLocalBounds().toFloat(), cornerRadius);

    // Draw a small border to give the display a bit of depth
    g.setColour(Colours::darkgrey);
    g.drawRoundedRectangle(getLocalBounds().toFloat(), cornerRadius, 2.0f);
}

void Display::resized() {}

void Display::toggleProgramSection(DisplayState state) {
    displayLookAndFeel.setColour(ComboBox::outlineColourId, DISPLAY_COLOURS[state]);
    displayLookAndFeel.setColour(ComboBox::textColourId, DISPLAY_COLOURS[state]);
    displayLookAndFeel.setColour(ComboBox::arrowColourId, DISPLAY_COLOURS[state]);
    displayLookAndFeel.setColour(ToggleButton::tickColourId, DISPLAY_COLOURS[state]);
    displayLookAndFeel.setColour(ToggleButton::tickDisabledColourId, DISPLAY_COLOURS[state]);
    displayLookAndFeel.setColour(ToggleButton::textColourId, DISPLAY_COLOURS[state]);
    displayLookAndFeel.setColour(Label::textColourId, DISPLAY_COLOURS[state]);
}

DisplayLookAndFeel::DisplayLookAndFeel() {
    displayColour = DISPLAY_COLOURS[ON];
    setDefaultSansSerifTypeface(getCustomTypeface());
    setColour(ComboBox::outlineColourId, displayColour);
    setColour(ComboBox::textColourId, displayColour);
    setColour(ComboBox::backgroundColourId, DISPLAY_BACK_COLOUR);
    setColour(ComboBox::arrowColourId, displayColour);
    setColour(PopupMenu::backgroundColourId, Colours::black);
    setColour(PopupMenu::textColourId, displayColour);
    setColour(ToggleButton::tickColourId, displayColour);
    setColour(ToggleButton::tickDisabledColourId, displayColour);
    setColour(ToggleButton::textColourId, displayColour);
    setColour(Label::textColourId, displayColour);
}

Typeface::Ptr DisplayLookAndFeel::getCustomTypeface() {
    if (!customTypeface)
        customTypeface = Typeface::createSystemTypefaceFor(BinaryData::DSEG14ClassicItalic_ttf, BinaryData::DSEG14ClassicItalic_ttfSize);
    return customTypeface;
}

Font DisplayLookAndFeel::getLabelFont(Label& label) { return Font(FontOptions(getCustomTypeface())); }

Font DisplayLookAndFeel::getPopupMenuFont() { return Font(FontOptions(getCustomTypeface()).withHeight(12.0f)); }

void DisplayLookAndFeel::drawComboBox(Graphics& g, int width, int height, bool isButtonDown, int buttonX, int buttonY, int buttonW, int buttonH, ComboBox& box) {
    // Draw the background
    g.setColour(box.findColour(ComboBox::backgroundColourId));
    g.fillRect(0, 0, width, height);

    // Draw the outline
    g.setColour(box.findColour(ComboBox::outlineColourId));
    g.drawRect(0, 0, width, height, 1); // No corner radius, just a rectangle

    // Draw the arrow button
    Path p;
    p.startNewSubPath(buttonX + buttonW * 0.2f, buttonY + buttonH * 0.3f);
    p.lineTo(buttonX + buttonW * 0.8f, buttonY + buttonH * 0.3f);
    p.lineTo(buttonX + buttonW * 0.5f, buttonY + buttonH * 0.7f);
    p.closeSubPath();

    g.setColour(box.findColour(ComboBox::arrowColourId).withAlpha(isButtonDown ? 0.9f : 0.6f));
    g.fillPath(p);
}

void DisplayLookAndFeel::drawToggleButton(Graphics& g, ToggleButton& button, bool isMouseOverButton, bool isButtonDown) {
    auto fontSize = jmin(15.0f, button.getHeight() * 0.75f);
    auto tickWidth = fontSize * 1.1f;

    g.setColour(button.findColour(ToggleButton::textColourId));
    g.setFont(Font(FontOptions(getCustomTypeface()).withHeight(fontSize)));

    auto textBounds = button.getLocalBounds().reduced((int)tickWidth, 0);
    g.drawFittedText(button.getButtonText(), textBounds, Justification::centredLeft, 10);

    g.setColour(button.findColour(ToggleButton::tickColourId));

    Rectangle<float> tickBounds(2.0f, 2.0f, tickWidth, tickWidth);

    // Draw the squared-off outline for the toggle button
    g.drawRect(tickBounds);

    if (button.getToggleState()) {
        g.setColour(button.findColour(ToggleButton::tickColourId));
        g.fillRect(tickBounds.reduced(4.0f));
    }
}