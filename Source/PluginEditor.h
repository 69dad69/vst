#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class VoiceDubAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                           private juce::Timer
{
public:
    explicit VoiceDubAudioProcessorEditor (VoiceDubAudioProcessor&);
    ~VoiceDubAudioProcessorEditor() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void setupSlider (juce::Slider& slider, juce::Label& label, const juce::String& text);

    VoiceDubAudioProcessor& processor;

    juce::Label title;
    juce::Label status;

    juce::Slider gate, minHz, maxHz, confidence, stability, releaseFrames,
                 hysteresis, bendRange, transpose, midiChannel;
    juce::Label gateLabel, minHzLabel, maxHzLabel, confidenceLabel, stabilityLabel,
                releaseFramesLabel, hysteresisLabel, bendRangeLabel, transposeLabel,
                midiChannelLabel;

    using Attachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<Attachment> gateAttachment, minHzAttachment, maxHzAttachment,
                                confidenceAttachment, stabilityAttachment, releaseFramesAttachment,
                                hysteresisAttachment, bendRangeAttachment, transposeAttachment,
                                midiChannelAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VoiceDubAudioProcessorEditor)
};
