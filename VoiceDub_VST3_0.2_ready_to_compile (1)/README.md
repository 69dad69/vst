# VoiceDub VST3 MVP

VoiceDub is an original voice-to-MIDI VST3 proof of concept. It is inspired by the general workflow of real-time vocal MIDI controllers, but it contains no Vochlea source code, models, assets, trademarks, or reverse-engineered components.

## Current state

Version 0.2 separates the real-time pitch/MIDI engine from the JUCE wrapper. The standalone core can be compiled and tested without JUCE or internet access. The VST3 wrapper uses JUCE 9.0.1 pinned to commit `e18f7f506c0b96f2c738a0bcd7fe6467a5005ad8`.

The plugin passes incoming audio through unchanged, analyses a mono sum, detects monophonic pitch using a lightweight YIN-style estimator, and emits MIDI note-on, note-off and pitch-bend messages. It includes a configurable input gate, vocal frequency range, confidence threshold, note stability, release tolerance, note hysteresis, bend range, transpose and MIDI channel.

The MIDI engine explicitly releases an active note on its original channel if the MIDI channel is changed while playing. It also centres pitch bend before starting a new note, reducing stuck-note and stale-bend failure modes.

## Offline core verification

This does not need JUCE:

    cmake --preset core-tests
    cmake --build --preset core-tests
    ctest --preset core-tests

The tests feed synthetic sine waves into the pitch detector and verify pitch accuracy, gate rejection, note lifecycle, release behaviour, pitch bend and MIDI-channel switching.

## VST3 build requirements

You need CMake 3.24+, a C++20 compiler and JUCE. By default CMake fetches the pinned JUCE revision from GitHub. If the build machine has no internet access, download JUCE separately and point VoiceDub at it:

    cmake -S . -B build -DVOICEDUB_JUCE_PATH=/path/to/JUCE -DCMAKE_BUILD_TYPE=Release
    cmake --build build --target VoiceDub_VST3 -j

You can also set the environment variable `JUCE_PATH` instead of passing `VOICEDUB_JUCE_PATH`.

JUCE licensing terms apply to any JUCE-based binary you distribute. Review the current JUCE licence before distribution.

## Windows

Open a Visual Studio 2022 Developer Command Prompt in this folder and run `build-windows.bat`, or run:

    cmake -S . -B build\windows-release -G "Visual Studio 17 2022" -A x64
    cmake --build build\windows-release --config Release --target VoiceDubCoreTests VoiceDub_VST3
    ctest --test-dir build\windows-release -C Release --output-on-failure

The VST3 bundle should appear at:

    build\windows-release\VoiceDub_artefacts\Release\VST3\VoiceDub.vst3

A common system-wide install directory is:

    C:\Program Files\Common Files\VST3

## macOS / Linux

Run `./build-unix.sh`, or:

    cmake --preset vst3-release
    cmake --build --preset vst3-release -j

On macOS the usual VST3 locations are `~/Library/Audio/Plug-Ins/VST3` and `/Library/Audio/Plug-Ins/VST3`. On Linux common locations include `~/.vst3` and `/usr/lib/vst3`.

Linux JUCE builds may additionally require the normal X11, font and audio development packages for the distribution.

## DAW routing

Place VoiceDub on an audio track receiving microphone input, then route the plugin's MIDI output to an instrument. Host support for MIDI output from an audio-effect VST3 differs by DAW.

Set the receiving instrument's pitch-bend range to the same number of semitones as VoiceDub's `Bend ±st` parameter. Otherwise continuous bends will be out of tune.

## Suggested starting values

Gate around -42 dB, Min 75 Hz, Max 1000 Hz, Confidence 0.68, Stability 2, Release 4, Hysteresis 0.12 semitones and Bend ±2 semitones are reasonable first values for clean monophonic voice. Raise Stability or Hysteresis if note boundaries chatter. Raise Confidence if octave errors occur. Narrow Min/Max Hz around the singer's range when possible.

## Known limitations

Pitch detection is monophonic and intentionally lightweight. Accuracy and latency depend on sample rate, buffer size, microphone signal-to-noise ratio, room reflections and articulation. Fast consonants can temporarily remove periodic pitch. The current decimator is an averaging filter rather than a production-grade resampler. There is no trained beatbox classifier, vowel-to-CC model, automatic key/scale detection, chord generation, microphone calibration model or post-record MIDI cleanup yet.

## License

The original VoiceDub source files in this package are released under the MIT License. JUCE is fetched or supplied separately and remains under its own licence.
