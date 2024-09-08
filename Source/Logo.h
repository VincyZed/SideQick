/*
  ==============================================================================

    Logo.h
    Created: 19 Aug 2024 12:28:43pm
    Author:  Vincent

  ==============================================================================
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
