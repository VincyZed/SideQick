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

class Logo : public juce::Component {
public:
    Logo();
    void paint(juce::Graphics& g) override;

private:
    std::unique_ptr<juce::Drawable> drawable;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Logo)
};
