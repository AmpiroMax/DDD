#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd -- "$(dirname "$0")/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-"$ROOT/build"}"
BUILD_TYPE="${CMAKE_BUILD_TYPE:-Debug}"

# Detect CPU cores for parallel build unless user overrides
if [ -z "${CMAKE_BUILD_PARALLEL_LEVEL:-}" ]; then
  if command -v sysctl >/dev/null 2>&1; then
    export CMAKE_BUILD_PARALLEL_LEVEL="$(sysctl -n hw.logicalcpu)"
  else
    export CMAKE_BUILD_PARALLEL_LEVEL="$(nproc 2>/dev/null || echo 4)"
  fi
fi

cmake -S "$ROOT" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE="$BUILD_TYPE"
cmake --build "$BUILD_DIR" "$@"
