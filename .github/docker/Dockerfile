# CI build image for MidiPlayer.
#
# Provides everything needed to build MidiPlayer for:
#   - x86_64 Linux   (native, since the self-hosted runner is amd64)
#   - aarch64 Linux  (cross-compiled via crossbuild-essential-arm64)
#   - x86_64 Windows (cross-compiled via mingw-w64, posix threads)
#
# Rebuilt/pushed only when this file changes (see
# .github/workflows/publish-ci-image.yml), then reused as-is by every CI run.
FROM debian:trixie-slim

ENV DEBIAN_FRONTEND=noninteractive

# --- Base build tooling ---
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    ninja-build \
    git \
    ca-certificates \
    pkg-config \
    wget \
    xvfb \
    xauth \
    && rm -rf /var/lib/apt/lists/*

# --- Native (amd64) dependencies: ALSA (rtaudio/portmidi) + OpenGL/X11 (glfw) ---
RUN apt-get update && apt-get install -y --no-install-recommends \
    libasound2-dev \
    libgl1-mesa-dev \
    libglu1-mesa-dev \
    libx11-dev \
    libxrandr-dev \
    libxinerama-dev \
    libxcursor-dev \
    libxi-dev \
    libxext-dev \
    && rm -rf /var/lib/apt/lists/*

# --- arm64 cross toolchain + arm64 dependencies (multiarch) ---
RUN dpkg --add-architecture arm64 \
    && apt-get update && apt-get install -y --no-install-recommends \
    crossbuild-essential-arm64 \
    libasound2-dev:arm64 \
    libgl1-mesa-dev:arm64 \
    libglu1-mesa-dev:arm64 \
    libx11-dev:arm64 \
    libxrandr-dev:arm64 \
    libxinerama-dev:arm64 \
    libxcursor-dev:arm64 \
    libxi-dev:arm64 \
    libxext-dev:arm64 \
    && rm -rf /var/lib/apt/lists/*

# --- Windows (mingw-w64, posix threads) cross toolchain ---
RUN apt-get update && apt-get install -y --no-install-recommends \
    g++-mingw-w64-x86-64 \
    gcc-mingw-w64-x86-64 \
    binutils-mingw-w64-x86-64 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace
