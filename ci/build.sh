#!/bin/bash
# Builds the web app, the LittleFS image and the firmware, in a node:*-bookworm image and not on
# the runner: the runner is a shared alpine container with no apt-get, sudo, python3 or pio.
# Usage: ci/build.sh NAME ; writes bin/light-saber-{firmware,littlefs}-NAME.bin
set -euo pipefail

name="$1"

# Debian's python3 refuses a system-wide pip install (PEP 668): PlatformIO goes in a venv.
# /root/.platformio is a docker volume, so the venv and the ESP32 toolchain survive between runs.
apt-get update -qq
apt-get install -y -qq python3-venv > /dev/null
venv=/root/.platformio/ci-venv
[ -x "$venv/bin/pio" ] || { python3 -m venv "$venv" && "$venv/bin/pip" install -q platformio; }
export PATH="$venv/bin:$PATH"

(cd app && npm ci && npm run build)
pio run --target buildfs --environment light-saber
pio run --environment light-saber

mkdir -p bin
cp .pio/build/light-saber/firmware.bin "bin/light-saber-firmware-${name}.bin"
cp .pio/build/light-saber/littlefs.bin "bin/light-saber-littlefs-${name}.bin"
(cd bin && sha256sum ./*.bin > "light-saber-${name}.sha256")
