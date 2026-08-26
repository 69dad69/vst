#include "VoiceDubCore.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>

namespace
{
int failures = 0;

void expect (bool condition, const std::string& message)
{
    if (! condition)
    {
        ++failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

float midiToHz (float midi)
{
    return 440.0f * std::pow (2.0f, (midi - 69.0f) / 12.0f);
}

voicedub::PitchResult detectSine (double sampleRate, float hz, float amplitude,
                                  float minHz = 55.0f, float maxHz = 1200.0f,
                                  float gateDb = -60.0f, float confidence = 0.60f)
{
    voicedub::PitchTracker tracker;
    tracker.prepare (sampleRate);

    voicedub::PitchResult last;
    constexpr double pi = 3.14159265358979323846;
    const int samples = static_cast<int> (sampleRate * 0.45);

    for (int i = 0; i < samples; ++i)
    {
        const float x = amplitude * static_cast<float> (std::sin (2.0 * pi * hz * i / sampleRate));
        voicedub::PitchResult current;
        if (tracker.pushSample (x, minHz, maxHz, gateDb, confidence, current) && current.valid)
            last = current;
    }

    return last;
}

void testPitchTracker()
{
    for (const float frequency : { 82.41f, 110.0f, 220.0f, 440.0f, 880.0f })
    {
        const auto result = detectSine (48000.0, frequency, 0.5f);
        expect (result.valid, "pitch should be valid for " + std::to_string (frequency) + " Hz");
        expect (std::abs (result.frequencyHz - frequency) < 1.5f,
                "pitch error should stay below 1.5 Hz for " + std::to_string (frequency) + " Hz");
        expect (result.confidence > 0.75f,
                "confidence should be high for clean sine " + std::to_string (frequency) + " Hz");
    }

    const auto quiet = detectSine (48000.0, 220.0f, 0.0001f, 55.0f, 1200.0f, -42.0f, 0.6f);
    expect (! quiet.valid, "gate should reject a very quiet signal");
}

void testMidiEngineLifecycle()
{
    voicedub::VoiceMidiEngine engine;
    voicedub::MidiEngineConfig config;
    config.stabilityFrames = 2;
    config.releaseFrames = 3;
    config.bendRangeSemitones = 2;

    voicedub::PitchResult a4;
    a4.valid = true;
    a4.frequencyHz = 440.0f;
    a4.confidence = 0.95f;
    a4.rmsDb = -18.0f;

    voicedub::MidiEventBuffer events;
    engine.process (a4, config, events);
    expect (engine.getActiveNote() < 0, "first frame should not trigger before stability threshold");

    engine.process (a4, config, events);
    expect (engine.getActiveNote() == 69, "A4 should map to MIDI note 69");
    expect (events.size() >= 2, "note start should emit pitch centre and note-on");

    voicedub::PitchResult sharp = a4;
    sharp.frequencyHz = 445.0f;
    engine.process (sharp, config, events);
    expect (engine.getLastPitchWheel() > 8192, "sharp pitch should generate positive pitch bend");

    voicedub::PitchResult invalid;
    engine.process (invalid, config, events);
    engine.process (invalid, config, events);
    expect (engine.getActiveNote() == 69, "release should tolerate short dropouts");
    engine.process (invalid, config, events);
    expect (engine.getActiveNote() < 0, "release threshold should send note off");
}

void testGlideDoesNotCreateChromaticSteps()
{
    voicedub::VoiceMidiEngine engine;
    voicedub::MidiEngineConfig config;
    config.stabilityFrames = 3;
    config.bendRangeSemitones = 8;
    config.noteHysteresisSemitones = 0.18f;

    voicedub::PitchResult pitch;
    pitch.valid = true;
    pitch.confidence = 0.95f;
    pitch.rmsDb = -18.0f;

    voicedub::MidiEventBuffer events;

    pitch.frequencyHz = midiToHz (60.0f);
    for (int i = 0; i < 4; ++i)
        engine.process (pitch, config, events);

    expect (engine.getActiveNote() == 60, "glide test should start on MIDI note 60");

    int intermediateNoteOns = 0;
    for (float midi = 60.15f; midi <= 64.0f; midi += 0.15f)
    {
        pitch.frequencyHz = midiToHz (midi);
        engine.process (pitch, config, events);

        for (std::size_t i = 0; i < events.size(); ++i)
            if (events[i].type == voicedub::MidiEventType::noteOn
                && events[i].note != 64)
                ++intermediateNoteOns;
    }

    expect (intermediateNoteOns == 0,
            "continuous C-to-E glide must not create intermediate chromatic note-ons");

    pitch.frequencyHz = midiToHz (64.0f);
    for (int i = 0; i < 24 && engine.getActiveNote() != 64; ++i)
        engine.process (pitch, config, events);

    expect (engine.getActiveNote() == 64,
            "after the glide settles, the engine should switch directly to the final E note");
}

void testChannelChangeSafety()
{
    voicedub::VoiceMidiEngine engine;
    voicedub::MidiEngineConfig config;
    config.stabilityFrames = 1;

    voicedub::PitchResult pitch;
    pitch.valid = true;
    pitch.frequencyHz = 220.0f;
    pitch.confidence = 0.95f;
    pitch.rmsDb = -18.0f;

    voicedub::MidiEventBuffer events;
    engine.process (pitch, config, events);
    expect (engine.getActiveChannel() == 1, "note should start on channel 1");

    config.midiChannel = 2;
    engine.process (pitch, config, events);

    bool sawOldChannelOff = false;
    for (std::size_t i = 0; i < events.size(); ++i)
        if (events[i].type == voicedub::MidiEventType::noteOff && events[i].channel == 1)
            sawOldChannelOff = true;

    expect (sawOldChannelOff, "changing MIDI channel must release the note on the old channel");
}
}

int main()
{
    testPitchTracker();
    testMidiEngineLifecycle();
    testGlideDoesNotCreateChromaticSteps();
    testChannelChangeSafety();

    if (failures != 0)
    {
        std::cerr << failures << " test(s) failed\n";
        return EXIT_FAILURE;
    }

    std::cout << "All VoiceDub core tests passed\n";
    return EXIT_SUCCESS;
}
