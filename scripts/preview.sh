#!/usr/bin/env bash
# Headless preview: run minitv (sink=preview) on the Pi and serve frames over HTTP.
# Open http://<pi-ip>:8080/preview.html. Usage: scripts/preview.sh [file.mp4]
# Env: PI_HOST, PRESET, PORT (default 8080)
set -euo pipefail
PI_HOST="${PI_HOST:-ncl-buildserver}"
PRESET="${PRESET:-gcc-dev-Release}"
PORT="${PORT:-8080}"
cd "$(dirname "$0")/.."

scripts/deploy.sh build
ssh -t "$PI_HOST" "
  mkdir -p /tmp/minitv && cp ~/Mini-TV/conf/preview.html /tmp/minitv/
  (cd /tmp/minitv && python3 -m http.server $PORT >/dev/null 2>&1 & echo \$! > /tmp/minitv/http.pid)
  trap 'kill \$(cat /tmp/minitv/http.pid) 2>/dev/null' EXIT
  echo 'Preview: http://$PI_HOST:$PORT/preview.html  (q + Enter to quit)'
  ~/Mini-TV/build/$PRESET/minitv ~/Mini-TV/conf/minitv.conf ${1:-}
"
