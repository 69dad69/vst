#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace ParamIDs
{
    static constexpr auto gateDb = "gateDb";
    static constexpr auto minHz = "minHz";
    static constexpr auto maxHz = "maxHz";
    static constexpr auto confidence = "confidence";
    static constexpr auto stability = "stability";
    static constexpr auto releaseFrames = "releaseFrames";
    static constexpr auto hysteresis = "hysteresis";
    static constexpr auto bendRange = "bendRange";
    static constexpr auto transpose = "transpose";
    static constexpr auto midiChannel = "midiChannel";
}

VoiceDubAudioProcessor::VoiceDubAudioProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMETERS", createParameterLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout VoiceDubAudioProcessor::createParameterLayout()
{
    using APF = juce::AudioParameterFloat;
    using API = juce::AudioParameterInt;
    using Range = juce::NormalisableRange<float>;

    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<APF> (juce::ParameterID { ParamIDs::gateDb, 1 },
                                             "Gate", Range { -72.0f, -12.0f, 0.1f }, -42.0f));
    params.push_back (std::make_unique<APF> (juce::ParameterID { ParamIDs::minHz, 1 },
                                             "Min frequency", Range { 55.0f, 300.0f, 1.0f }, 75.0f));
    params.push_back (std::make_unique<APF> (juce::ParameterID { ParamIDs::maxHz, 1 },
                                             "Max frequency", Range { 300.0f, 1600.0f, 1.0f }, 1000.0f));
    params.push_back (std::make_unique<APF> (juce::ParameterID { ParamIDs::confidence, 1 },
                                             "Confidence", Range { 0.30f, 0.95f, 0.01f }, 0.68f));
    params.push_back (std::make_unique<API> (juce::ParameterID { ParamIDs::stability, 1 },
                                             "Stability", 1, 12, 2));
    params.push_back (std::make_unique<API> (juce::ParameterID { ParamIDs::releaseFrames, 1 },
                                             "Release frames", 1, 24, 4));
    params.push_back (std::make_unique<APF> (juce::ParameterID { ParamIDs::hysteresis, 1 },
                                             "Note hysteresis", Range { 0.0f, 0.49f, 0.01f }, 0.12f));
    params.push_back (std::make_unique<API> (juce::ParameterID { ParamIDs::bendRange, 1 },
                                             "Pitch bend range", 1, 24, 2));
    params.push_back (std::make_unique<API> (juce::ParameterID { ParamIDs::transpose, 1 },
                                             "Transpose", -48, 48, 0));
    params.push_back (std::make_unique<API> (juce::ParameterID { ParamIDs::midiChannel, 1 },
                                             "MIDI channel", 1, 16, 1));

    return { params.begin(), params.end() };
}

void VoiceDubAudioProcessor::prepareToPlay (double sampleRate, int)
{
    pitchTracker.prepare (sampleRate);
    midiEngine.reset();
    displayHz.store (0.0f);
    displayConfidence.store (0.0f);
    displayRmsDb.store (-120.0f);
    displayMidiNote.store (-1);
}

void VoiceDubAudioProcessor::releaseResources()
{
    pitchTracker.reset();
    midiEngine.reset();
}

bool VoiceDubAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto in = layouts.getMainInputChannelSet();
    const auto out = layouts.getMainOutputChannelSet();

    if (in != out)
        return false;

    return in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo();
}

void VoiceDubAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    if (numSamples <= 0 || numChannels <= 0)
        return;

    const float minHz = apvts.getRawParameterValue (ParamIDs::minHz)->load();
    const float maxHz = juce::jmax (minHz + 10.0f,
                                    apvts.getRawParameterValue (ParamIDs::maxHz)->load());
    const float gateDb = apvts.getRawParameterValue (ParamIDs::gateDb)->load();
    const float minConfidence = apvts.getRawParameterValue (ParamIDs::confidence)->load();

    voicedub::MidiEngineConfig midiConfig;
    midiConfig.midiChannel = static_cast<int> (apvts.getRawParameterValue (ParamIDs::midiChannel)->load());
    midiConfig.stabilityFrames = static_cast<int> (apvts.getRawParameterValue (ParamIDs::stability)->load());
    midiConfig.releaseFrames = static_cast<int> (apvts.getRawParameterValue (ParamIDs::releaseFrames)->load());
    midiConfig.bendRangeSemitones = static_cast<int> (apvts.getRawParameterValue (ParamIDs::bendRange)->load());
    midiConfig.transposeSemitones = static_cast<int> (apvts.getRawParameterValue (ParamIDs::transpose)->load());
    midiConfig.gateDb = gateDb;
    midiConfig.noteHysteresisSemitones = apvts.getRawParameterValue (ParamIDs::hysteresis)->load();

    for (int i = 0; i < numSamples; ++i)
    {
        float mono = 0.0f;
        for (int ch = 0; ch < numChannels; ++ch)
            mono += buffer.getReadPointer (ch)[i];
        mono /= static_cast<float> (numChannels);

        voicedub::PitchResult result;
        if (! pitchTracker.pushSample (mono, minHz, maxHz, gateDb, minConfidence, result))
            continue;

        displayRmsDb.store (result.rmsDb);
        displayConfidence.store (result.confidence);
        displayHz.store (result.valid ? result.frequencyHz : 0.0f);

        voicedub::MidiEventBuffer events;
        midiEngine.process (result, midiConfig, events);
        emitMidiEvents (events, midi, i, numSamples);
        displayMidiNote.store (midiEngine.getActiveNote());
    }
}

void VoiceDubAudioProcessor::emitMidiEvents (const voicedub::MidiEventBuffer& events,
                                              juce::MidiBuffer& midi,
                                              int sampleOffset, int blockSize)
{
    const int safeOffset = juce::jlimit (0, juce::jmax (0, blockSize - 1), sampleOffset);

    for (std::size_t i = 0; i < events.size(); ++i)
    {
        const auto& event = events[i];
        switch (event.type)
        {
            case voicedub::MidiEventType::noteOn:
                midi.addEvent (juce::MidiMessage::noteOn (event.channel, event.note, event.velocity), safeOffset);
                break;
            case voicedub::MidiEventType::noteOff:
                midi.addEvent (juce::MidiMessage::noteOff (event.channel, event.note), safeOffset);
                break;
            case voicedub::MidiEventType::pitchBend:
                midi.addEvent (juce::MidiMessage::pitchWheel (event.channel, event.value), safeOffset);
                break;
        }
    }
}

void VoiceDubAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void VoiceDubAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VoiceDubAudioProcessor();
}
