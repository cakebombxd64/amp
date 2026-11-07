#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    using Coeffs = juce::dsp::IIR::Coefficients<float>;

    // Vorstufen-Röhre: leicht asymmetrische Sättigung (wie eine 12AX7-Triode).
    // Bei kleinem Pegel nahezu linear -> clean; bei hohem Volume weicher Breakup.
    inline float tubeStage (float x, float drive) noexcept
    {
        constexpr float bias   = 0.18f;
        constexpr float offset = 0.178081f; // tanh(bias)
        return std::tanh (drive * x + bias) - offset;
    }

    // Endstufe: sanfte, symmetrische Kompression (Push-Pull)
    inline float powerStage (float x) noexcept
    {
        constexpr float k = 1.3f;
        return std::tanh (k * x) / k;
    }

    // Volume-Knopf (0..10) -> Vorstufen-Verstärkung (~0.6 bis ~9.5)
    inline float volumeToDrive (float volume) noexcept
    {
        return 0.6f * std::pow (10.0f, volume * 0.12f);
    }

    inline float knobTo (float knob, float lo, float hi) noexcept
    {
        return juce::jmap (knob, 0.0f, 10.0f, lo, hi);
    }

    inline float db (float decibels) noexcept
    {
        return juce::Decibels::decibelsToGain (decibels);
    }

    constexpr float maxDelaySeconds = 2.0f;
}

//==============================================================================
CleanAmpAudioProcessor::CleanAmpAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout CleanAmpAudioProcessor::createLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    auto addKnob = [&params] (const char* id, const char* name, float defaultValue)
    {
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { id, 1 }, name,
            juce::NormalisableRange<float> (0.0f, 10.0f, 0.01f), defaultValue));
    };

    auto addSwitch = [&params] (const char* id, const char* name, bool defaultValue)
    {
        params.push_back (std::make_unique<juce::AudioParameterBool> (
            juce::ParameterID { id, 1 }, name, defaultValue));
    };

    // --- Amp (Startwerte = typischer Fender-Clean) ---
    addKnob ("volume", "Volume", 4.0f);
    addKnob ("treble", "Treble", 6.0f);
    addKnob ("bass",   "Bass",   4.5f);
    addKnob ("mid",    "Middle", 5.0f);

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "master", 1 }, "Master",
        juce::NormalisableRange<float> (-30.0f, 6.0f, 0.1f), -6.0f,
        juce::AudioParameterFloatAttributes().withLabel ("dB")));

    addSwitch ("bright", "Bright", false);
    addSwitch ("cab", "Box-Sim", true);

    // --- Tremolo ---
    addSwitch ("trem_on", "Tremolo An", false);
    addKnob ("trem_speed", "Tremolo Speed", 4.0f);
    addKnob ("trem_depth", "Tremolo Intensity", 5.0f);

    // --- Delay ---
    addSwitch ("delay_on", "Delay An", false);
    {
        juce::NormalisableRange<float> timeRange (40.0f, 1200.0f, 1.0f);
        timeRange.setSkewForCentre (350.0f);
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "delay_time", 1 }, "Delay Time", timeRange, 380.0f,
            juce::AudioParameterFloatAttributes().withLabel ("ms")));
    }
    addKnob ("delay_fb",  "Delay Feedback", 3.0f);
    addKnob ("delay_mix", "Delay Mix", 3.0f);

    // --- Reverb ---
    addSwitch ("reverb_on", "Reverb An", true);
    addKnob ("reverb",      "Reverb Mix", 2.5f);
    addKnob ("reverb_size", "Reverb Decay", 5.0f);
    addKnob ("reverb_tone", "Reverb Tone", 5.0f);

    return { params.begin(), params.end() };
}

//==============================================================================
bool CleanAmpAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();

    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;

    return layouts.getMainInputChannelSet() == out;
}

void CleanAmpAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;
    const auto numCh = (juce::uint32) juce::jmax (1, getTotalNumOutputChannels());
    const juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) samplesPerBlock, numCh };

    for (auto* f : { &brightFilter, &bassFilter, &midFilter, &trebleFilter, &dcBlocker,
                     &cabHP, &cabLowRes, &cabPresence, &cabLP1, &cabLP2,
                     &reverbHP, &reverbLP })
    {
        f->prepare (spec);
        f->reset();
    }

    // Feste Filter
    *dcBlocker.state   = *Coeffs::makeHighPass (sr, 20.0f);

    // Box: offene 2x12 mit Jensen-artigem Charakter
    *cabHP.state       = *Coeffs::makeHighPass   (sr, 75.0f, 0.7f);
    *cabLowRes.state   = *Coeffs::makePeakFilter (sr, 110.0f, 1.2f, db (3.0f));
    *cabPresence.state = *Coeffs::makePeakFilter (sr, 2400.0f, 1.4f, db (3.5f));
    *cabLP1.state      = *Coeffs::makeLowPass    (sr, 5500.0f, 0.707f);
    *cabLP2.state      = *Coeffs::makeLowPass    (sr, 7500.0f, 0.6f);

    *reverbHP.state    = *Coeffs::makeHighPass (sr, 180.0f);

    // 2x Oversampling für die Sättigung
    oversampling = std::make_unique<juce::dsp::Oversampling<float>> (
        (size_t) numCh, 1, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, true);
    oversampling->initProcessing ((size_t) samplesPerBlock);
    setLatencySamples (juce::roundToInt (oversampling->getLatencyInSamples()));

    // Reverb
    reverb.prepare (spec);
    reverb.reset();
    reverbBuffer.setSize ((int) numCh, samplesPerBlock);

    // Delay
    delayLine.setMaximumDelayInSamples ((int) std::ceil (sampleRate * maxDelaySeconds) + 4);
    delayLine.prepare (spec);
    delayLine.reset();
    delayLpState.fill (0.0f);
    delayLpCoeff = (float) std::exp (-juce::MathConstants<double>::twoPi * 3500.0 / sampleRate);

    tremPhase = 0.0;

    // Glättung gegen Knackser beim Drehen
    driveSmoothed.reset (sampleRate * 2.0, 0.05);
    masterSmoothed.reset (sampleRate, 0.05);
    reverbMixSmoothed.reset (sampleRate, 0.05);
    delayTimeSmoothed.reset (sampleRate, 0.25); // langsam -> beim Drehen leichter Tape-Pitch-Effekt
    delayFbSmoothed.reset (sampleRate, 0.05);
    delayMixSmoothed.reset (sampleRate, 0.05);
    tremRateSmoothed.reset (sampleRate, 0.1);
    tremDepthSmoothed.reset (sampleRate, 0.05);

    driveSmoothed.setCurrentAndTargetValue (volumeToDrive (param ("volume")));
    masterSmoothed.setCurrentAndTargetValue (db (param ("master")));
    reverbMixSmoothed.setCurrentAndTargetValue (param ("reverb_on") > 0.5f ? param ("reverb") * 0.05f : 0.0f);
    delayTimeSmoothed.setCurrentAndTargetValue (param ("delay_time") * 0.001f * (float) sampleRate);
    delayFbSmoothed.setCurrentAndTargetValue (param ("delay_fb") * 0.092f);
    delayMixSmoothed.setCurrentAndTargetValue (param ("delay_mix") * 0.09f);
    tremRateSmoothed.setCurrentAndTargetValue (knobTo (param ("trem_speed"), 1.0f, 10.0f));
    tremDepthSmoothed.setCurrentAndTargetValue (param ("trem_on") > 0.5f ? param ("trem_depth") * 0.09f : 0.0f);

    updateFilters (true);
}

void CleanAmpAudioProcessor::updateFilters (bool force)
{
    const float volume  = param ("volume");
    const float bass    = param ("bass");
    const float mid     = param ("mid");
    const float treble  = param ("treble");
    const bool  bright  = param ("bright") > 0.5f;
    const float revSize = param ("reverb_size");
    const float revTone = param ("reverb_tone");

    if (force || volume != lastVolume || bass != lastBass || mid != lastMid
              || treble != lastTreble || bright != lastBright)
    {
        lastVolume = volume; lastBass = bass; lastMid = mid; lastTreble = treble; lastBright = bright;

        // Bright wie beim echten Amp: bei leisem Volume stark, bei hohem kaum noch
        const float brightDb = bright ? knobTo (volume, 9.0f, 1.5f) : 0.0f;
        *brightFilter.state = *Coeffs::makeHighShelf (sr, 2500.0f, 0.6f, db (brightDb));

        // Tonestack im Fender-Stil
        *bassFilter.state   = *Coeffs::makeLowShelf   (sr, 110.0f,  0.7f, db (knobTo (bass,   -12.0f, 8.0f)));
        *midFilter.state    = *Coeffs::makePeakFilter (sr, 500.0f,  0.8f, db (knobTo (mid,    -12.0f, 1.0f)));
        *trebleFilter.state = *Coeffs::makeHighShelf  (sr, 2200.0f, 0.7f, db (knobTo (treble, -12.0f, 8.0f)));
    }

    if (force || revTone != lastRevTone)
    {
        lastRevTone = revTone;
        const float cutoff = juce::jmin (2000.0f * std::pow (4.0f, revTone / 10.0f), (float) sr * 0.45f);
        *reverbLP.state = *Coeffs::makeLowPass (sr, cutoff);
    }

    if (force || revSize != lastRevSize)
    {
        lastRevSize = revSize;
        juce::Reverb::Parameters rp;
        rp.roomSize   = knobTo (revSize, 0.3f, 0.93f);
        rp.damping    = 0.5f;
        rp.wetLevel   = 1.0f;
        rp.dryLevel   = 0.0f;
        rp.width      = 1.0f;
        rp.freezeMode = 0.0f;
        reverb.setParameters (rp);
    }
}

void CleanAmpAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numCh = getTotalNumOutputChannels();
    const int n     = buffer.getNumSamples();

    for (int ch = getTotalNumInputChannels(); ch < numCh; ++ch)
        buffer.clear (ch, 0, n);

    if (n == 0 || numCh == 0)
        return;

    updateFilters (false);

    const bool cabOn    = param ("cab") > 0.5f;
    const bool delayOn  = param ("delay_on") > 0.5f;
    const bool tremOn   = param ("trem_on") > 0.5f;
    const bool reverbOn = param ("reverb_on") > 0.5f;

    driveSmoothed.setTargetValue (volumeToDrive (param ("volume")));
    masterSmoothed.setTargetValue (db (param ("master")));
    reverbMixSmoothed.setTargetValue (reverbOn ? param ("reverb") * 0.05f : 0.0f);           // 10 -> 50 %
    delayTimeSmoothed.setTargetValue (param ("delay_time") * 0.001f * (float) sr);
    delayFbSmoothed.setTargetValue (param ("delay_fb") * 0.092f);                            // 10 -> 92 %
    delayMixSmoothed.setTargetValue (param ("delay_mix") * 0.09f);                           // 10 -> 90 %
    tremRateSmoothed.setTargetValue (knobTo (param ("trem_speed"), 1.0f, 10.0f));            // 1..10 Hz
    tremDepthSmoothed.setTargetValue (tremOn ? param ("trem_depth") * 0.09f : 0.0f);

    juce::dsp::AudioBlock<float> block = juce::dsp::AudioBlock<float> (buffer)
                                             .getSubsetChannelBlock (0, (size_t) numCh);
    juce::dsp::ProcessContextReplacing<float> ctx (block);

    // 1) Bright
    brightFilter.process (ctx);

    // 2) Vorstufe, 2x oversampled
    {
        auto osBlock = oversampling->processSamplesUp (block);
        const auto osN  = osBlock.getNumSamples();
        const auto osCh = osBlock.getNumChannels();

        for (size_t i = 0; i < osN; ++i)
        {
            const float drive = driveSmoothed.getNextValue();
            const float comp  = 1.0f / std::sqrt (drive);

            for (size_t ch = 0; ch < osCh; ++ch)
            {
                auto* d = osBlock.getChannelPointer (ch);
                d[i] = tubeStage (d[i], drive) * comp;
            }
        }

        oversampling->processSamplesDown (block);
    }

    // 3) Tonestack
    bassFilter.process (ctx);
    midFilter.process (ctx);
    trebleFilter.process (ctx);

    // 4) Endstufe + 5) Tremolo (Lautstärke-Modulation wie beim Fender-Bias-Tremolo)
    {
        constexpr double twoPi = juce::MathConstants<double>::twoPi;
        constexpr float shapeNorm = 0.946806f; // tanh(1.8)

        for (int i = 0; i < n; ++i)
        {
            const float rate  = tremRateSmoothed.getNextValue();
            const float depth = tremDepthSmoothed.getNextValue();

            tremPhase += twoPi * rate / sr;
            if (tremPhase >= twoPi)
                tremPhase -= twoPi;

            // Sinus, leicht "eckiger" gemacht -> pulsiert wie das Original
            const float lfo   = std::tanh (1.8f * (float) std::sin (tremPhase)) / shapeNorm;
            const float tremG = 1.0f - depth * 0.5f * (1.0f + lfo);

            for (int ch = 0; ch < numCh; ++ch)
            {
                auto* d = buffer.getWritePointer (ch);
                d[i] = powerStage (d[i]) * tremG;
            }
        }
    }

    dcBlocker.process (ctx);

    // 6) Box-Simulation
    if (cabOn)
    {
        cabHP.process (ctx);
        cabLowRes.process (ctx);
        cabPresence.process (ctx);
        cabLP1.process (ctx);
        cabLP2.process (ctx);
    }

    // 7) Delay (Wiederholungen klingen nach dem Ausschalten natürlich aus)
    {
        const int maxDelay = delayLine.getMaximumDelayInSamples();

        for (int i = 0; i < n; ++i)
        {
            const float t   = juce::jlimit (1.0f, (float) maxDelay, delayTimeSmoothed.getNextValue());
            const float fb  = delayFbSmoothed.getNextValue();
            const float mix = delayMixSmoothed.getNextValue();

            for (int ch = 0; ch < juce::jmin (numCh, 2); ++ch)
            {
                auto* d = buffer.getWritePointer (ch);
                const float x = d[i];

                const float delayed = delayLine.popSample (ch, t);

                // Höhen pro Wiederholung abdämpfen (analoger Charakter)
                auto& lp = delayLpState[(size_t) ch];
                lp = delayed + delayLpCoeff * (lp - delayed);

                const float input = (delayOn ? x : 0.0f) + std::tanh (lp * fb);
                delayLine.pushSample (ch, input);

                d[i] = x + mix * delayed;
            }
        }
    }

    // 8) Reverb (parallel) + Master
    reverbBuffer.setSize (numCh, n, false, false, true);
    for (int ch = 0; ch < numCh; ++ch)
        reverbBuffer.copyFrom (ch, 0, buffer, ch, 0, n);

    juce::dsp::AudioBlock<float> revBlock (reverbBuffer);
    juce::dsp::ProcessContextReplacing<float> revCtx (revBlock);
    reverbHP.process (revCtx);
    reverbLP.process (revCtx);
    reverb.process (revCtx);

    for (int i = 0; i < n; ++i)
    {
        const float mix  = reverbMixSmoothed.getNextValue();
        const float gain = masterSmoothed.getNextValue();

        for (int ch = 0; ch < numCh; ++ch)
        {
            auto* d = buffer.getWritePointer (ch);
            const auto* r = reverbBuffer.getReadPointer (ch);
            d[i] = (d[i] + mix * r[i]) * gain;
        }
    }
}

//==============================================================================
juce::AudioProcessorEditor* CleanAmpAudioProcessor::createEditor()
{
    return new CleanAmpAudioProcessorEditor (*this);
}

void CleanAmpAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void CleanAmpAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));

    if (xml != nullptr && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new CleanAmpAudioProcessor();
}
