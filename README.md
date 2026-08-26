# VoiceDub VST3

VoiceDub is an original monophonic voice-to-MIDI VST3 plugin. It analyses incoming microphone audio, tracks pitch in real time, emits MIDI Note On/Off and Pitch Bend messages, and passes the audio signal through unchanged.

The project is inspired only by the general workflow of real-time vocal MIDI controllers. It contains no Vochlea source code, models, assets, trademarks, or reverse-engineered components.

## Current implementation

VoiceDub 0.3.0 contains a JUCE VST3 wrapper and a standalone real-time core. The pitch tracker uses a lightweight YIN-style estimator and the MIDI engine adds note stability, release tolerance, hysteresis, velocity derived from input level, configurable pitch-bend range, transpose and MIDI channel handling. Active notes are explicitly released if the MIDI channel changes, and pitch bend is centred when notes are started or released to reduce stuck-note and stale-bend failure modes.

The editor exposes Gate, Min Hz, Max Hz, Confidence, Stability, Release, Hysteresis, Bend range, Transpose and MIDI channel. It also displays the detected note, frequency, confidence and input level.

## Standalone core tests

The DSP/MIDI core can be compiled without JUCE:

```sh
cmake --preset core-tests
cmake --build --preset core-tests
ctest --preset core-tests
```

The tests verify clean sine-wave pitch detection at 82.41, 110, 220, 440 and 880 Hz, input gate rejection, note lifecycle, release tolerance, pitch bend and safe MIDI-channel switching.

## Building the VST3

Requirements are CMake 3.24+, a C++20 compiler and JUCE. By default CMake fetches JUCE pinned to commit `e18f7f506c0b96f2c738a0bcd7fe6467a5005ad8`. You can instead provide a local JUCE checkout through `VOICEDUB_JUCE_PATH` or the `JUCE_PATH` environment variable.

On Windows with Visual Studio 2022, run:

```bat
build-windows.bat
```

or manually:

```bat
cmake -S . -B build\windows-release -G "Visual Studio 17 2022" -A x64 -DVOICEDUB_BUILD_PLUGIN=ON -DVOICEDUB_BUILD_TESTS=ON
cmake --build build\windows-release --config Release --target VoiceDubCoreTests VoiceDub_VST3
ctest --test-dir build\windows-release -C Release --output-on-failure
```

The expected bundle is:

```text
build\windows-release\VoiceDub_artefacts\Release\VST3\VoiceDub.vst3
```

On macOS or Linux, run `./build-unix.sh` or use the `vst3-release` CMake preset.

## DAW routing

Insert VoiceDub on an audio track receiving the microphone. Route the plugin MIDI output to an instrument track. Set the receiving instrument pitch-bend range to the same number of semitones as VoiceDub's bend-range parameter. Host support for MIDI output from an audio-effect VST3 varies between DAWs.

## CI

`.github/workflows/build-vst3.yml` runs standalone core tests on Linux and builds a Windows x64 VST3 on GitHub Actions. A successful Windows job uploads the complete `VoiceDub.vst3` bundle as the `VoiceDub-Windows-x64-VST3` artifact.

If the repository shows no workflow runs, enable GitHub Actions for the repository in Settings > Actions > General, then push any commit or run the workflow manually.

## Limitations

The current tracker is monophonic. Latency and accuracy depend on sample rate, buffer size, microphone signal-to-noise ratio, room reflections and articulation. The current decimator is intentionally lightweight rather than a production-grade resampler. There is no trained beatbox classifier, vowel-to-CC model, automatic scale lock, chord generation, microphone calibration model or post-record MIDI cleanup yet.

## Licensing

The VoiceDub source in this repository is MIT licensed. JUCE is fetched separately and remains under its own licence; review the current JUCE licensing terms before distributing binaries.
