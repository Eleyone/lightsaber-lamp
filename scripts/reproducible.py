#!/usr/bin/env python3
"""
Pre script for PlatformIO: makes the firmware reproducible, so the CI image and
a local build of the same commit are byte for byte identical.
- __DATE__/__TIME__ come from the commit time (SOURCE_DATE_EPOCH) instead of the clock
- Absolute paths embedded by assert()/__FILE__ are rewritten to stable prefixes

The LittleFS image can't be: mklittlefs stores the clock as creation time and
the file mtimes, so ci/verify.sh compares its unpacked files instead.

Runs as "pre:" so the flags also reach the Arduino core and the libraries.
"""

import subprocess

# PlatformIO Import
Import("env")

project_dir = env["PROJECT_DIR"]

try:
    commit_time = subprocess.run(
        ["git", "log", "-1", "--format=%ct"],
        capture_output=True, text=True, cwd=project_dir, check=True
    ).stdout.strip()
    env["ENV"]["SOURCE_DATE_EPOCH"] = commit_time
except Exception as e:
    print(f"[Reproducible] Warning: Could not get the commit time: {e}")

env.Append(CCFLAGS=[
    f"-ffile-prefix-map={env.subst('$PROJECT_CORE_DIR')}=/pio",
    f"-ffile-prefix-map={project_dir}=.",
])
