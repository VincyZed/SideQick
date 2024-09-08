/*
  ==============================================================================

    PannelButton.h
    Created: 18 Jul 2024 3:41:59pm
    Author:  Vincent

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

using namespace juce;

class PannelButton : public ShapeButton
{
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