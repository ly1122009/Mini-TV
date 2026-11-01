#!/usr/bin/env bash
# Edit on WSL, build (and optionally run) on the Pi, which is the build server.
# Usage: scripts/deploy.sh [build|run|test]
# Env:   PI_HOST (default ncl-buildserver), PRESET (default gcc-dev-Release), REMOTE_DIR
set -euo pipefail
PI_HOST="${PI_HOST:-ncl-buildserver}"
PRESET="${PRESET:-gcc-dev-Release}"
REMOTE_DIR="${REMOTE_DIR:-Mini-TV}"
cd "$(dirname "$0")/.."

rsync -az --delete --exclude .git --exclude build --exclude install ./ "$PI_HOST:$REMOTE_DIR/"
ssh "$PI_HOST" "cd $REMOTE_DIR && cmake --preset $PRESET >/dev/null && cmake --build --preset $PRESET"

case "${1:-build}" in
run) ssh -t "$PI_HOST" "cd $REMOTE_DIR && ./build/$PRESET/minitv conf/minitv.conf ${2:-}" ;;
test) ssh "$PI_HOST" "cd $REMOTE_DIR && ctest --preset test-$PRESET" ;;
esac
