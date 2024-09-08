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
	label.setFont(Font("Arial", 16.0f, Font::italic));
	label.setBounds(xPos - buttonWidth / 2, yPos - 20, 2 * buttonWidth, 0.5 * buttonHeight);
}

// Set the default constructor from the ShapeButton class
PannelButton::PannelButton() : ShapeButton("button", Colours::white, Colours::white, Colours::white) {}

PannelButton::~PannelButton() {
	setLookAndFeel(nullptr);
}

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
	repaint();
}