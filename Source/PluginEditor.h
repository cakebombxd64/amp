#pragma once

#include "PluginProcessor.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>
#include <vector>

//==============================================================================
// Blackface-Optik: schwarzes Panel, Knöpfe mit Skala 1–10, rote "Jewel"-Lampen als Schalter
class AmpLookAndFeel : public juce::LookAndFeel_V4
{
public:
    AmpLookAndFeel();

    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPos, float startAngle, float endAngle,
                           juce::Slider&) override;

    void drawToggleButton (juce::Graphics&, juce::ToggleButton&,
                           bool shouldDrawButtonAsHighlighted,
                           bool shouldDrawButtonAsDown) override;
};

//==============================================================================
class CleanAmpAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit CleanAmpAudioProcessorEditor (CleanAmpAudioProcessor&);
    ~CleanAmpAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct Knob
    {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };

    struct Switch
    {
        juce::ToggleButton button;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> attachment;
    };

    using KnobList = std::vector<std::unique_ptr<Knob>>;

    std::unique_ptr<Knob> makeKnob (const char* paramId, const char* name,
                                    const char* suffix = "", bool noScale = false);
    std::unique_ptr<Switch> makeSwitch (const char* paramId, const char* name);
    static void layoutKnobs (juce::Rectangle<int> area, KnobList& list);

    CleanAmpAudioProcessor& audioProcessor;
    AmpLookAndFeel lnf;

    KnobList ampKnobs, tremKnobs, delayKnobs, reverbKnobs;
    std::unique_ptr<Switch> brightSwitch, cabSwitch, tremSwitch, delaySwitch, reverbSwitch;

    // für die Beschriftung in paint()
    juce::Rectangle<int> ampTitle, tremTitle, delayTitle, reverbTitle, effectsArea;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CleanAmpAudioProcessorEditor)
};
