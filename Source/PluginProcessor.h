#pragma once

#include <JuceHeader.h>
#include "VoiceDubCore.h"
#include <atomic>

class VoiceDubAudioProcessor final : public juce::AudioProcessor
{
public:
    VoiceDubAudioProcessor();
    ~VoiceDubAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    std::atomic<float> displayHz { 0.0f };
    std::atomic<float> displayConfidence { 0.0f };
    std::atomic<float> displayRmsDb { -120.0f };
    std::atomic<int> displayMidiNote { -1 };

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

private:
    void emitMidiEvents (const voicedub::MidiEventBuffer& events, juce::MidiBuffer& midi,
                         int sampleOffset, int blockSize);

    voicedub::PitchTracker pitchTracker;
    voicedub::VoiceMidiEngine midiEngine;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VoiceDubAudioProcessor)
};
