#include "VoiceDubCore.h"

#include <algorithm>
#include <cmath>

namespace voicedub
{
namespace
{
template <typename T>
T clampValue (T low, T high, T value)
{
    return std::min (high, std::max (low, value));
}
}

void PitchTracker::prepare (double sampleRate)
{
    decimationFactor = clampValue (1, 8, static_cast<int> (std::lround (sampleRate / 24000.0)));
    decimatedSampleRate = std::max (8000.0, sampleRate / static_cast<double> (decimationFactor));
    reset();
}

void PitchTracker::reset()
{
    ring.fill (0.0f);
    frame.fill (0.0f);
    yin.fill (1.0f);
    writeIndex = 0;
    samplesStored = 0;
    samplesSinceAnalysis = 0;
    decimationCounter = 0;
    decimationSum = 0.0f;
}

bool PitchTracker::pushSample (float monoSample, float minHz, float maxHz, float gateDb,
                               float minConfidence, PitchResult& result)
{
    decimationSum += monoSample;
    ++decimationCounter;

    if (decimationCounter < decimationFactor)
        return false;

    const float decimated = decimationSum / static_cast<float> (decimationFactor);
    decimationCounter = 0;
    decimationSum = 0.0f;

    ring[static_cast<std::size_t> (writeIndex)] = decimated;
    writeIndex = (writeIndex + 1) % ringSize;
    samplesStored = std::min (ringSize, samplesStored + 1);
    ++samplesSinceAnalysis;

    if (samplesStored < windowSize || samplesSinceAnalysis < hopSize)
        return false;

    samplesSinceAnalysis = 0;
    result = analyse (minHz, maxHz, gateDb, minConfidence);
    return true;
}

float PitchTracker::sampleAtFromEnd (int samplesBack) const
{
    int index = writeIndex - 1 - samplesBack;
    while (index < 0)
        index += ringSize;
    return ring[static_cast<std::size_t> (index % ringSize)];
}

PitchResult PitchTracker::analyse (float minHz, float maxHz, float gateDb, float minConfidence)
{
    PitchResult out;

    minHz = std::max (20.0f, minHz);
    maxHz = std::max (minHz + 1.0f, maxHz);
    minConfidence = clampValue (0.0f, 0.999f, minConfidence);

    for (int i = 0; i < windowSize; ++i)
        frame[static_cast<std::size_t> (windowSize - 1 - i)] = sampleAtFromEnd (i);

    double energy = 0.0;
    for (const auto x : frame)
        energy += static_cast<double> (x) * static_cast<double> (x);

    const float rms = static_cast<float> (std::sqrt (energy / static_cast<double> (windowSize)));
    out.rmsDb = rms > 1.0e-6f ? 20.0f * std::log10 (rms) : -120.0f;

    if (out.rmsDb < gateDb)
        return out;

    const int minLag = clampValue (2, maxYinLag - 2,
        static_cast<int> (std::floor (decimatedSampleRate / std::max (maxHz, 1.0f))));
    const int maxLag = clampValue (minLag + 2, maxYinLag - 2,
        static_cast<int> (std::ceil (decimatedSampleRate / std::max (minHz, 1.0f))));

    const int compareLength = windowSize - maxLag - 1;
    if (compareLength < 128)
        return out;

    yin.fill (1.0f);
    double runningSum = 0.0;

    for (int tau = 1; tau <= maxLag + 1; ++tau)
    {
        double diff = 0.0;
        for (int i = 0; i < compareLength; ++i)
        {
            const float d = frame[static_cast<std::size_t> (i)]
                          - frame[static_cast<std::size_t> (i + tau)];
            diff += static_cast<double> (d) * static_cast<double> (d);
        }

        runningSum += diff;
        yin[static_cast<std::size_t> (tau)] = runningSum > 1.0e-12
            ? static_cast<float> (diff * static_cast<double> (tau) / runningSum)
            : 1.0f;
    }

    const float yinThreshold = clampValue (0.03f, 0.70f, 1.0f - minConfidence);
    int bestTau = -1;

    for (int tau = minLag; tau <= maxLag; ++tau)
    {
        const float y = yin[static_cast<std::size_t> (tau)];
        if (y < yinThreshold
            && y <= yin[static_cast<std::size_t> (tau - 1)]
            && y <= yin[static_cast<std::size_t> (tau + 1)])
        {
            bestTau = tau;
            break;
        }
    }

    if (bestTau < 0)
    {
        float best = 1.0f;
        for (int tau = minLag; tau <= maxLag; ++tau)
        {
            const float y = yin[static_cast<std::size_t> (tau)];
            if (y < best)
            {
                best = y;
                bestTau = tau;
            }
        }
    }

    if (bestTau <= 1)
        return out;

    const float y0 = yin[static_cast<std::size_t> (bestTau - 1)];
    const float y1 = yin[static_cast<std::size_t> (bestTau)];
    const float y2 = yin[static_cast<std::size_t> (bestTau + 1)];
    const float denom = y0 - 2.0f * y1 + y2;
    float refinedTau = static_cast<float> (bestTau);

    if (std::abs (denom) > 1.0e-8f)
        refinedTau += 0.5f * (y0 - y2) / denom;

    const float confidence = clampValue (0.0f, 1.0f, 1.0f - y1);
    const float hz = static_cast<float> (decimatedSampleRate / static_cast<double> (refinedTau));

    if (! std::isfinite (hz) || hz < minHz || hz > maxHz || confidence < minConfidence)
        return out;

    out.valid = true;
    out.frequencyHz = hz;
    out.confidence = confidence;
    return out;
}

bool MidiEventBuffer::add (const MidiEvent& event) noexcept
{
    if (count >= events.size())
        return false;

    events[count++] = event;
    return true;
}

void VoiceMidiEngine::reset() noexcept
{
    activeNote = -1;
    activeChannel = 1;
    pendingNote = -1;
    stableCount = 0;
    dropoutCount = 0;
    lastPitchWheel = 8192;
    hasLastMidiPitch = false;
    lastMidiPitch = 0.0f;
    pitchMotion = 0.0f;
    settledFrames = 0;
}

float VoiceMidiEngine::frequencyToMidi (float frequencyHz) noexcept
{
    return 69.0f + 12.0f * std::log2 (frequencyHz / 440.0f);
}

float VoiceMidiEngine::velocityFromRms (float rmsDb, float gateDb) noexcept
{
    const float denominator = std::max (1.0f, -6.0f - gateDb);
    const float t = clampValue (0.0f, 1.0f, (rmsDb - gateDb) / denominator);
    return 0.18f + t * 0.82f;
}

void VoiceMidiEngine::emitNoteOffAndCentre (MidiEventBuffer& output) noexcept
{
    if (activeNote < 0)
        return;

    output.add ({ MidiEventType::noteOff, activeChannel, activeNote, 0, 0.0f });
    output.add ({ MidiEventType::pitchBend, activeChannel, 0, 8192, 0.0f });
    activeNote = -1;
    pendingNote = -1;
    stableCount = 0;
    lastPitchWheel = 8192;
    settledFrames = 0;
}

void VoiceMidiEngine::process (const PitchResult& result, const MidiEngineConfig& config,
                               MidiEventBuffer& output) noexcept
{
    output.clear();

    const int channel = clampValue (1, 16, config.midiChannel);
    const int stabilityNeeded = clampValue (1, 12, config.stabilityFrames);
    const int releaseNeeded = clampValue (1, 24, config.releaseFrames);
    const int bendRange = clampValue (1, 24, config.bendRangeSemitones);
    const int transpose = clampValue (-48, 48, config.transposeSemitones);
    const float hysteresis = clampValue (0.0f, 0.49f, config.noteHysteresisSemitones);

    if (activeNote >= 0 && channel != activeChannel)
        emitNoteOffAndCentre (output);

    if (! result.valid || result.frequencyHz <= 0.0f)
    {
        ++dropoutCount;
        pendingNote = -1;
        stableCount = 0;
        settledFrames = 0;

        if (dropoutCount >= releaseNeeded)
            emitNoteOffAndCentre (output);

        return;
    }

    dropoutCount = 0;

    const float midiFloat = frequencyToMidi (result.frequencyHz) + static_cast<float> (transpose);

    if (hasLastMidiPitch)
    {
        const float instantMotion = std::abs (midiFloat - lastMidiPitch);
        pitchMotion = 0.72f * pitchMotion + 0.28f * instantMotion;
    }
    else
    {
        pitchMotion = 0.0f;
        hasLastMidiPitch = true;
    }

    lastMidiPitch = midiFloat;

    int candidate = clampValue (0, 127, static_cast<int> (std::lround (midiFloat)));

    if (activeNote >= 0 && std::abs (midiFloat - static_cast<float> (activeNote)) < 0.5f + hysteresis)
        candidate = activeNote;

    if (activeNote < 0)
    {
        if (candidate == pendingNote)
            ++stableCount;
        else
        {
            pendingNote = candidate;
            stableCount = 1;
        }

        if (stableCount < stabilityNeeded)
            return;

        activeChannel = channel;
        activeNote = candidate;
        lastPitchWheel = 8192;
        settledFrames = 0;
        output.add ({ MidiEventType::pitchBend, activeChannel, 0, 8192, 0.0f });
        output.add ({ MidiEventType::noteOn, activeChannel, activeNote, 0,
                      velocityFromRms (result.rmsDb, config.gateDb) });
    }
    else if (candidate != activeNote)
    {
        // During a glide, keep the current MIDI note and express the continuous
        // movement as pitch bend. Only change the base MIDI note after the
        // detected pitch has actually settled on a new target. This prevents a
        // C->E vocal slide from generating C#, D and D# note-ons on the way.
        constexpr float settledMotionThreshold = 0.035f;
        const int settleNeeded = std::max (2, std::min (5, stabilityNeeded));

        if (pitchMotion <= settledMotionThreshold)
            ++settledFrames;
        else
            settledFrames = 0;

        if (candidate == pendingNote)
            ++stableCount;
        else
        {
            pendingNote = candidate;
            stableCount = 1;
        }

        if (settledFrames >= settleNeeded && stableCount >= stabilityNeeded)
        {
            output.add ({ MidiEventType::noteOff, activeChannel, activeNote, 0, 0.0f });
            output.add ({ MidiEventType::pitchBend, activeChannel, 0, 8192, 0.0f });
            activeChannel = channel;
            activeNote = candidate;
            lastPitchWheel = 8192;
            pendingNote = activeNote;
            stableCount = 0;
            settledFrames = 0;
            pitchMotion = 0.0f;
            output.add ({ MidiEventType::noteOn, activeChannel, activeNote, 0,
                          velocityFromRms (result.rmsDb, config.gateDb) });
        }
    }
    else
    {
        pendingNote = activeNote;
        stableCount = 0;
        settledFrames = 0;
    }

    if (activeNote < 0)
        return;

    const float semitoneOffset = midiFloat - static_cast<float> (activeNote);
    const float bendNorm = clampValue (-1.0f, 1.0f, semitoneOffset / static_cast<float> (bendRange));
    const int wheel = clampValue (0, 16383,
        static_cast<int> (std::lround (8192.0f + bendNorm * 8191.0f)));

    if (std::abs (wheel - lastPitchWheel) >= 8)
    {
        output.add ({ MidiEventType::pitchBend, activeChannel, 0, wheel, 0.0f });
        lastPitchWheel = wheel;
    }
}
}
