#!/usr/bin/env bash
# build.sh - configure + build AutoMaster on macOS / Linux.
#
#   ./scripts/build.sh                       # Release, fetch JUCE
#   ./scripts/build.sh Debug
#   AUTOMASTER_JUCE_PATH=~/JUCE ./scripts/build.sh
#
# Requires: CMake >= 3.22 and a C++17 toolchain (Xcode CLT / gcc / clang).

set -euo pipefail
CONFIG="${1:-Release}"
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD="$ROOT/build"

CMAKE_ARGS=(-S "$ROOT" -B "$BUILD" -DCMAKE_BUILD_TYPE="$CONFIG")
if [[ -n "${AUTOMASTER_JUCE_PATH:-}" ]]; then
    CMAKE_ARGS+=(-DAUTOMASTER_JUCE_PATH="$AUTOMASTER_JUCE_PATH")
fi

echo "==> cmake ${CMAKE_ARGS[*]}"
cmake "${CMAKE_ARGS[@]}"

echo "==> cmake --build $BUILD --config $CONFIG"
cmake --build "$BUILD" --config "$CONFIG" --parallel

echo
echo "VST3 output:"
find "$BUILD" -name "AutoMaster.vst3" -maxdepth 6 -print
