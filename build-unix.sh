#!/usr/bin/env sh
set -eu

cmake --preset vst3-release
cmake --build --preset vst3-release -j

echo "Built plugin should be under build/vst3-release/VoiceDub_artefacts/Release/VST3/VoiceDub.vst3"
