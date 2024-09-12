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
