#include "PluginEditor.h"

VoiceDubAudioProcessorEditor::VoiceDubAudioProcessorEditor (VoiceDubAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    title.setText ("VoiceDub — voice to MIDI", juce::dontSendNotification);
    title.setFont (title.getFont().withHeight (22.0f).boldened());
    title.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (title);

    status.setText ("Waiting for voice…", juce::dontSendNotification);
    status.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (status);

    setupSlider (gate, gateLabel, "Gate");
    setupSlider (minHz, minHzLabel, "Min Hz");
    setupSlider (maxHz, maxHzLabel, "Max Hz");
    setupSlider (confidence, confidenceLabel, "Confidence");
    setupSlider (stability, stabilityLabel, "Stability");
    setupSlider (releaseFrames, releaseFramesLabel, "Release");
    setupSlider (hysteresis, hysteresisLabel, "Hysteresis");
    setupSlider (bendRange, bendRangeLabel, "Bend ±st");
    setupSlider (transpose, transposeLabel, "Transpose");
    setupSlider (midiChannel, midiChannelLabel, "MIDI ch");

    gateAttachment = std::make_unique<Attachment> (processor.apvts, "gateDb", gate);
    minHzAttachment = std::make_unique<Attachment> (processor.apvts, "minHz", minHz);
    maxHzAttachment = std::make_unique<Attachment> (processor.apvts, "maxHz", maxHz);
    confidenceAttachment = std::make_unique<Attachment> (processor.apvts, "confidence", confidence);
    stabilityAttachment = std::make_unique<Attachment> (processor.apvts, "stability", stability);
    releaseFramesAttachment = std::make_unique<Attachment> (processor.apvts, "releaseFrames", releaseFrames);
    hysteresisAttachment = std::make_unique<Attachment> (processor.apvts, "hysteresis", hysteresis);
    bendRangeAttachment = std::make_unique<Attachment> (processor.apvts, "bendRange", bendRange);
    transposeAttachment = std::make_unique<Attachment> (processor.apvts, "transpose", transpose);
    midiChannelAttachment = std::make_unique<Attachment> (processor.apvts, "midiChannel", midiChannel);

    setSize (780, 410);
    startTimerHz (20);
}

void VoiceDubAudioProcessorEditor::setupSlider (juce::Slider& slider, juce::Label& label,
                                                 const juce::String& text)
{
    label.setText (text, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (label);

    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 82, 20);
    addAndMakeVisible (slider);
}

void VoiceDubAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (findColour (juce::ResizableWindow::backgroundColourId));

    auto r = getLocalBounds().reduced (16);
    r.removeFromTop (68);
    g.setColour (findColour (juce::Label::textColourId).withAlpha (0.12f));
    g.drawRoundedRectangle (r.toFloat(), 10.0f, 1.0f);
}

void VoiceDubAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (18);
    auto header = area.removeFromTop (60);
    title.setBounds (header.removeFromTop (30));
    status.setBounds (header);

    area.removeFromTop (12);

    constexpr int columns = 5;
    constexpr int rows = 2;
    const int cellW = area.getWidth() / columns;
    const int cellH = area.getHeight() / rows;

    juce::Slider* sliders[] = { &gate, &minHz, &maxHz, &confidence, &stability,
                                &releaseFrames, &hysteresis, &bendRange, &transpose, &midiChannel };
    juce::Label* labels[] = { &gateLabel, &minHzLabel, &maxHzLabel, &confidenceLabel, &stabilityLabel,
                              &releaseFramesLabel, &hysteresisLabel, &bendRangeLabel,
                              &transposeLabel, &midiChannelLabel };

    for (int i = 0; i < 10; ++i)
    {
        const int row = i / columns;
        const int col = i % columns;
        auto cell = juce::Rectangle<int> (area.getX() + col * cellW,
                                          area.getY() + row * cellH,
                                          cellW, cellH).reduced (8);
        labels[i]->setBounds (cell.removeFromTop (22));
        sliders[i]->setBounds (cell);
    }
}

void VoiceDubAudioProcessorEditor::timerCallback()
{
    const float hz = processor.displayHz.load();
    const float conf = processor.displayConfidence.load();
    const float rmsDb = processor.displayRmsDb.load();
    const int note = processor.displayMidiNote.load();

    if (hz <= 0.0f)
    {
        status.setText ("No stable pitch  |  Input " + juce::String (rmsDb, 1) + " dB",
                        juce::dontSendNotification);
        return;
    }

    const juce::String noteText = note >= 0
        ? juce::MidiMessage::getMidiNoteName (note, true, true, 3)
        : "—";

    status.setText (noteText + "  |  " + juce::String (hz, 1) + " Hz  |  confidence "
                    + juce::String (conf * 100.0f, 0) + "%  |  input "
                    + juce::String (rmsDb, 1) + " dB",
                    juce::dontSendNotification);
}

juce::AudioProcessorEditor* VoiceDubAudioProcessor::createEditor()
{
    return new VoiceDubAudioProcessorEditor (*this);
}
