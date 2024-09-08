/*
  ==============================================================================

    Logo.cpp
    Created: 19 Aug 2024 12:28:43pm
    Author:  Vincent

  ==============================================================================
*/

#include "Logo.h"

using namespace juce;

Logo::Logo() {
    // Load the SVG from the binary data
    auto svgData = BinaryData::logo_svg;
    auto svgSize = BinaryData::logo_svgSize;

    // Create a Drawable from the SVG data
    drawable = juce::Drawable::createFromImageData(svgData, svgSize);
}

void Logo::paint(juce::Graphics& g) {
    if (drawable != nullptr) {
        // Get the area to draw within
        auto bounds = getLocalBounds().toFloat();

        // Draw the SVG
        drawable->drawWithin(g, bounds, juce::RectanglePlacement::xLeft, 1.0f);

        if (drawable != nullptr) {
            // Replace the color in the SVG (for example, replace black with red)
            drawable->replaceColour(juce::Colours::white, juce::Colours::lightgrey);
        }
    }
}
