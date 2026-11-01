#!/usr/bin/env bash
# Run ON the Pi: ssh ncl-buildserver 'bash -s' < scripts/setup-pi.sh
# Installs build + runtime deps so the Pi can act as the build server.
set -euo pipefail
sudo apt-get update
sudo apt-get install -y build-essential cmake ninja-build pkg-config rsync v4l-utils \
    libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev \
    gstreamer1.0-tools gstreamer1.0-plugins-base gstreamer1.0-plugins-base-apps \
    gstreamer1.0-plugins-good gstreamer1.0-plugins-bad gstreamer1.0-libav gstreamer1.0-alsa
