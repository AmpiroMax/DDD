#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd -- "$(dirname "$0")/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-"$ROOT/build"}"
BIN_NAME="${BIN_NAME:-DDDGame}"
BIN_PATH="$BUILD_DIR/$BIN_NAME"

if [ ! -x "$BIN_PATH" ]; then
  "$ROOT/scripts/build.sh"
fi

# Run from repo root so relative paths to config/ and resources/ work.
cd "$ROOT"
"$BIN_PATH" "$@"
