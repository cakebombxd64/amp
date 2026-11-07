#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <memory>
#include <vector>

//==============================================================================
// Glasklar Amp – cleaner Amp im Blackface-Stil mit Tremolo, Delay und Reverb
// Signalweg: Bright -> Vorstufe (2x oversampled) -> Tonestack -> Endstufe
//            -> Tremolo -> Box-Simulation -> Delay -> Reverb -> Master
//==============================================================================
class CleanAmpAudioProcessor : public juce::AudioProcessor
{
public:
    CleanAmpAudioProcessor();
    ~CleanAmpAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Glasklar Amp"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 8.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    juce::AudioProcessorValueTreeState apvts;

private:
    using Filter = juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>,
                                                  juce::dsp::IIR::Coefficients<float>>;

    float param (const char* id) const { return apvts.getRawParameterValue (id)->load(); }
    void updateFilters (bool force);

    double sr = 44100.0;

    // Klangregelung
    Filter brightFilter, bassFilter, midFilter, trebleFilter;
    // Gleichspannungsfilter nach der asymmetrischen Sättigung
    Filter dcBlocker;
    // Box-Simulation (offene 2x12-Box)
    Filter cabHP, cabLowRes, cabPresence, cabLP1, cabLP2;
    // Klangfilter für den Hall
    Filter reverbHP, reverbLP;

    std::unique_ptr<juce::dsp::Oversampling<float>> oversampling;

    // Reverb
    juce::dsp::Reverb reverb;
    juce::AudioBuffer<float> reverbBuffer;

    // Delay (analog-artig: Höhen werden mit jeder Wiederholung weicher)
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> delayLine;
    std::array<float, 2> delayLpState {};
    float delayLpCoeff = 0.0f;

    // Tremolo
    double tremPhase = 0.0;

    juce::SmoothedValue<float> driveSmoothed, masterSmoothed, reverbMixSmoothed;
    juce::SmoothedValue<float> delayTimeSmoothed, delayFbSmoothed, delayMixSmoothed;
    juce::SmoothedValue<float> tremRateSmoothed, tremDepthSmoothed;

    float lastVolume = -1.0f, lastBass = -1.0f, lastMid = -1.0f, lastTreble = -1.0f;
    float lastRevSize = -1.0f, lastRevTone = -1.0f;
    bool lastBright = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CleanAmpAudioProcessor)
};
