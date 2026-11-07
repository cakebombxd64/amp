#include "PluginEditor.h"

namespace AmpColours
{
    const juce::Colour tolex  { 0xff1d1b19 };
    const juce::Colour panel  { 0xff101010 };
    const juce::Colour chrome { 0xffc9ccd1 };
    const juce::Colour text   { 0xfff2f2f2 };
    const juce::Colour jewel  { 0xffff3b30 };
}

//==============================================================================
AmpLookAndFeel::AmpLookAndFeel()
{
    setColour (juce::BubbleComponent::backgroundColourId, AmpColours::panel.brighter (0.15f));
    setColour (juce::BubbleComponent::outlineColourId,    AmpColours::chrome);
    setColour (juce::TooltipWindow::textColourId,         AmpColours::text);
}

void AmpLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                       float sliderPos, float startAngle, float endAngle,
                                       juce::Slider& slider)
{
    const auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat().reduced (4.0f);
    const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto centre  = bounds.getCentre();

    // Skala 1–10 (nicht bei Reglern mit echten Einheiten wie dB oder ms)
    if (! (bool) slider.getProperties().getWithDefault ("noScale", false))
    {
        g.setColour (AmpColours::text.withAlpha (0.85f));
        g.setFont (juce::FontOptions (radius * 0.2f));

        for (int i = 1; i <= 10; ++i)
        {
            const float angle = startAngle + (float) (i - 1) / 9.0f * (endAngle - startAngle);
            const auto pos = centre.getPointOnCircumference (radius * 0.88f, angle);
            g.drawText (juce::String (i),
                        juce::Rectangle<float> (radius * 0.4f, radius * 0.25f).withCentre (pos),
                        juce::Justification::centred);
        }
    }

    // Schwarzer Knopf-Rock
    const float skirtR = radius * 0.62f;
    const auto skirt = juce::Rectangle<float> (skirtR * 2.0f, skirtR * 2.0f).withCentre (centre);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff3a3a3a), centre.x, centre.y - skirtR,
                                             juce::Colour (0xff050505), centre.x, centre.y + skirtR, false));
    g.fillEllipse (skirt);
    g.setColour (juce::Colours::black);
    g.drawEllipse (skirt, 1.5f);

    // Chrom-Kappe
    const float capR = skirtR * 0.6f;
    g.setGradientFill (juce::ColourGradient (juce::Colours::white, centre.x - capR * 0.4f, centre.y - capR * 0.6f,
                                             juce::Colour (0xff7d8086), centre.x + capR, centre.y + capR, true));
    g.fillEllipse (juce::Rectangle<float> (capR * 2.0f, capR * 2.0f).withCentre (centre));

    // Zeiger
    const float angle = startAngle + sliderPos * (endAngle - startAngle);
    const auto inner = centre.getPointOnCircumference (capR * 0.9f, angle);
    const auto outer = centre.getPointOnCircumference (skirtR * 0.95f, angle);
    g.setColour (AmpColours::text);
    g.drawLine ({ inner, outer }, juce::jmax (2.0f, radius * 0.06f));
}

void AmpLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& button,
                                       bool shouldDrawButtonAsHighlighted, bool)
{
    auto bounds = button.getLocalBounds().toFloat();
    const auto led = bounds.removeFromLeft (bounds.getHeight()).withSizeKeepingCentre (16.0f, 16.0f);
    const bool on = button.getToggleState();

    if (on)
    {
        g.setColour (AmpColours::jewel.withAlpha (0.35f));
        g.fillEllipse (led.expanded (5.0f));
    }

    g.setColour (on ? AmpColours::jewel : juce::Colour (0xff3a1414));
    g.fillEllipse (led);
    g.setColour (AmpColours::chrome);
    g.drawEllipse (led, 1.5f);

    g.setColour (AmpColours::text.withAlpha (shouldDrawButtonAsHighlighted ? 1.0f : 0.85f));
    g.setFont (juce::FontOptions (15.0f, juce::Font::bold));
    g.drawText (button.getButtonText(), bounds.withTrimmedLeft (6.0f), juce::Justification::centredLeft);
}

//==============================================================================
std::unique_ptr<CleanAmpAudioProcessorEditor::Knob>
CleanAmpAudioProcessorEditor::makeKnob (const char* paramId, const char* name, const char* suffix, bool noScale)
{
    auto k = std::make_unique<Knob>();

    k->slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    k->slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    k->slider.setPopupDisplayEnabled (true, true, this);
    k->slider.setTextValueSuffix (suffix);
    k->slider.getProperties().set ("noScale", noScale);
    k->slider.setLookAndFeel (&lnf);

    if (auto* param = audioProcessor.apvts.getParameter (paramId))
        k->slider.setDoubleClickReturnValue (true, param->convertFrom0to1 (param->getDefaultValue()));

    k->attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.apvts, paramId, k->slider);
    addAndMakeVisible (k->slider);

    k->label.setText (name, juce::dontSendNotification);
    k->label.setJustificationType (juce::Justification::centred);
    k->label.setColour (juce::Label::textColourId, AmpColours::text);
    k->label.setFont (juce::Font (juce::FontOptions (13.0f, juce::Font::bold)));
    addAndMakeVisible (k->label);

    return k;
}

std::unique_ptr<CleanAmpAudioProcessorEditor::Switch>
CleanAmpAudioProcessorEditor::makeSwitch (const char* paramId, const char* name)
{
    auto s = std::make_unique<Switch>();
    s->button.setButtonText (name);
    s->button.setLookAndFeel (&lnf);
    s->attachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        audioProcessor.apvts, paramId, s->button);
    addAndMakeVisible (s->button);
    return s;
}

CleanAmpAudioProcessorEditor::CleanAmpAudioProcessorEditor (CleanAmpAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Amp
    ampKnobs.push_back (makeKnob ("volume", "VOLUME"));
    ampKnobs.push_back (makeKnob ("treble", "TREBLE"));
    ampKnobs.push_back (makeKnob ("bass",   "BASS"));
    ampKnobs.push_back (makeKnob ("mid",    "MIDDLE"));
    ampKnobs.push_back (makeKnob ("master", "MASTER", " dB", true));
    brightSwitch = makeSwitch ("bright", "Bright");
    cabSwitch    = makeSwitch ("cab", "Box-Sim");

    // Tremolo
    tremKnobs.push_back (makeKnob ("trem_speed", "SPEED"));
    tremKnobs.push_back (makeKnob ("trem_depth", "INTENSITY"));
    tremSwitch = makeSwitch ("trem_on", "An");

    // Delay
    delayKnobs.push_back (makeKnob ("delay_time", "TIME", " ms", true));
    delayKnobs.push_back (makeKnob ("delay_fb",   "FEEDBACK"));
    delayKnobs.push_back (makeKnob ("delay_mix",  "MIX"));
    delaySwitch = makeSwitch ("delay_on", "An");

    // Reverb
    reverbKnobs.push_back (makeKnob ("reverb",      "MIX"));
    reverbKnobs.push_back (makeKnob ("reverb_size", "DECAY"));
    reverbKnobs.push_back (makeKnob ("reverb_tone", "TONE"));
    reverbSwitch = makeSwitch ("reverb_on", "An");

    setSize (980, 580);
}

CleanAmpAudioProcessorEditor::~CleanAmpAudioProcessorEditor()
{
    for (auto* list : { &ampKnobs, &tremKnobs, &delayKnobs, &reverbKnobs })
        for (auto& k : *list)
            k->slider.setLookAndFeel (nullptr);

    for (auto* s : { brightSwitch.get(), cabSwitch.get(), tremSwitch.get(), delaySwitch.get(), reverbSwitch.get() })
        s->button.setLookAndFeel (nullptr);
}

void CleanAmpAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (AmpColours::tolex);

    // Tolex-Struktur
    g.setColour (juce::Colours::white.withAlpha (0.025f));
    for (int i = -getHeight(); i < getWidth(); i += 6)
        g.drawLine ((float) i, 0.0f, (float) (i + getHeight()), (float) getHeight(), 1.0f);

    // Frontplatte
    const auto panelArea = getLocalBounds().toFloat().reduced (14.0f);
    g.setColour (AmpColours::panel);
    g.fillRoundedRectangle (panelArea, 6.0f);

    // Chromleiste unten
    const auto strip = panelArea.withTop (panelArea.getBottom() - 10.0f).reduced (6.0f, 2.0f);
    g.setGradientFill (juce::ColourGradient (juce::Colours::white, 0.0f, strip.getY(),
                                             juce::Colour (0xff6d7076), 0.0f, strip.getBottom(), false));
    g.fillRect (strip);

    g.setColour (AmpColours::chrome);
    g.drawRoundedRectangle (panelArea, 6.0f, 2.0f);

    // Logo
    auto logoArea = panelArea.reduced (22.0f, 10.0f).removeFromTop (46.0f);
    g.setColour (AmpColours::text);
    g.setFont (juce::FontOptions (32.0f, juce::Font::bold | juce::Font::italic));
    g.drawText ("Glasklar", logoArea, juce::Justification::centredLeft);

    g.setColour (AmpColours::chrome.withAlpha (0.8f));
    g.setFont (juce::FontOptions (13.0f, juce::Font::bold));
    g.drawText ("CLEAN AMP  \xc2\xb7  NWTE", logoArea, juce::Justification::centredRight);

    // Sektions-Titel
    g.setColour (AmpColours::chrome);
    g.setFont (juce::FontOptions (14.0f, juce::Font::bold));
    g.drawText ("AMP",     ampTitle,    juce::Justification::centredLeft);
    g.drawText ("TREMOLO", tremTitle,   juce::Justification::centredLeft);
    g.drawText ("DELAY",   delayTitle,  juce::Justification::centredLeft);
    g.drawText ("REVERB",  reverbTitle, juce::Justification::centredLeft);

    // Trennlinien
    g.setColour (AmpColours::chrome.withAlpha (0.35f));
    g.drawHorizontalLine (effectsArea.getY() - 6, (float) effectsArea.getX(), (float) effectsArea.getRight());
    const int third = effectsArea.getWidth() / 3;
    for (int i = 1; i < 3; ++i)
        g.drawVerticalLine (effectsArea.getX() + i * third, (float) effectsArea.getY() + 4.0f,
                            (float) effectsArea.getBottom() - 4.0f);
}

void CleanAmpAudioProcessorEditor::layoutKnobs (juce::Rectangle<int> area, KnobList& list)
{
    if (list.empty())
        return;

    const int w = area.getWidth() / (int) list.size();

    for (auto& k : list)
    {
        auto col = area.removeFromLeft (w);
        k->label.setBounds (col.removeFromBottom (22));
        k->slider.setBounds (col.reduced (2));
    }
}

void CleanAmpAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (34);
    area.removeFromTop (50);   // Logo
    area.removeFromBottom (8); // Chromleiste

    // --- Amp-Reihe ---
    auto ampRow = area.removeFromTop (area.getHeight() / 2);
    ampTitle = ampRow.removeFromTop (24);

    auto switches = ampRow.removeFromLeft (130);
    brightSwitch->button.setBounds (switches.removeFromTop (switches.getHeight() / 2).withSizeKeepingCentre (120, 30));
    cabSwitch->button.setBounds (switches.withSizeKeepingCentre (120, 30));
    layoutKnobs (ampRow, ampKnobs);

    // --- Effekt-Reihe ---
    area.removeFromTop (12);
    effectsArea = area;

    const int third = area.getWidth() / 3;

    auto layoutSection = [] (juce::Rectangle<int> sec, juce::Rectangle<int>& title,
                             Switch& sw, KnobList& knobs)
    {
        sec = sec.reduced (10, 0);
        auto titleRow = sec.removeFromTop (30);
        sw.button.setBounds (titleRow.removeFromRight (70));
        title = titleRow;
        layoutKnobs (sec, knobs);
    };

    layoutSection (area.removeFromLeft (third), tremTitle,   *tremSwitch,   tremKnobs);
    layoutSection (area.removeFromLeft (third), delayTitle,  *delaySwitch,  delayKnobs);
    layoutSection (area,                        reverbTitle, *reverbSwitch, reverbKnobs);
}
