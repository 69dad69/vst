#pragma once

#include <array>
#include <cstddef>

namespace voicedub
{
struct PitchResult
{
    bool valid = false;
    float frequencyHz = 0.0f;
    float confidence = 0.0f;
    float rmsDb = -120.0f;
};

class PitchTracker
{
public:
    void prepare (double sampleRate);
    bool pushSample (float monoSample, float minHz, float maxHz, float gateDb,
                     float minConfidence, PitchResult& result);
    void reset();

private:
    PitchResult analyse (float minHz, float maxHz, float gateDb, float minConfidence);
    float sampleAtFromEnd (int samplesBack) const;

    static constexpr int ringSize = 4096;
    static constexpr int windowSize = 1024;
    static constexpr int hopSize = 128;
    static constexpr int maxYinLag = 768;

    std::array<float, ringSize> ring {};
    std::array<float, windowSize> frame {};
    std::array<float, maxYinLag + 2> yin {};

    int writeIndex = 0;
    int samplesStored = 0;
    int samplesSinceAnalysis = 0;
    int decimationCounter = 0;
    int decimationFactor = 2;
    float decimationSum = 0.0f;
    double decimatedSampleRate = 24000.0;
};

enum class MidiEventType
{
    noteOn,
    noteOff,
    pitchBend
};

struct MidiEvent
{
    MidiEventType type = MidiEventType::noteOff;
    int channel = 1;
    int note = 0;
    int value = 0;
    float velocity = 0.0f;
};

class MidiEventBuffer
{
public:
    void clear() noexcept { count = 0; }
    bool add (const MidiEvent& event) noexcept;
    std::size_t size() const noexcept { return count; }
    const MidiEvent& operator[] (std::size_t index) const noexcept { return events[index]; }

private:
    static constexpr std::size_t capacity = 8;
    std::array<MidiEvent, capacity> events {};
    std::size_t count = 0;
};

struct MidiEngineConfig
{
    int midiChannel = 1;
    int stabilityFrames = 2;
    int releaseFrames = 4;
    int bendRangeSemitones = 2;
    int transposeSemitones = 0;
    float gateDb = -42.0f;
    float noteHysteresisSemitones = 0.12f;
};

class VoiceMidiEngine
{
public:
    void reset() noexcept;
    void process (const PitchResult& result, const MidiEngineConfig& config,
                  MidiEventBuffer& output) noexcept;

    int getActiveNote() const noexcept { return activeNote; }
    int getActiveChannel() const noexcept { return activeChannel; }
    int getLastPitchWheel() const noexcept { return lastPitchWheel; }

private:
    static float frequencyToMidi (float frequencyHz) noexcept;
    static float velocityFromRms (float rmsDb, float gateDb) noexcept;
    void emitNoteOffAndCentre (MidiEventBuffer& output) noexcept;

    int activeNote = -1;
    int activeChannel = 1;
    int pendingNote = -1;
    int stableCount = 0;
    int dropoutCount = 0;
    int lastPitchWheel = 8192;
};
}
