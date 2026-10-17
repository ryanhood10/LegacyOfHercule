#!/bin/bash
# Build the wide-view mGBA (SDL frontend, Lua scripting) inside WSL Ubuntu.
# usage (from Windows): wsl -d Ubuntu -u root -- bash /mnt/f/MeltyCreations/log2/tools/mgba-wide/build-wsl.sh
# Source: the mGBA 0.10 clone at tools/downloads/mgba with tools/mgba-wide/apply.py applied; built in
# /root/mgba-build (the WSL filesystem is much faster than /mnt/f). Output: /root/mgba-build/sdl/mgba.
set -e
SRC=/mnt/f/MeltyCreations/tools/downloads/mgba
PATCH=/mnt/f/MeltyCreations/log2/tools/mgba-wide
rm -rf /root/mgba-src
cp -rp "$SRC" /root/mgba-src
python3 "$PATCH/apply.py" /root/mgba-src
mkdir -p /root/mgba-build
cd /root/mgba-build
[ -f build.ninja ] || cmake -G Ninja /root/mgba-src -DCMAKE_BUILD_TYPE=Release -DBUILD_QT=OFF -DBUILD_SDL=ON -DBUILD_GL=OFF -DBUILD_GLES2=OFF -DBUILD_GLES3=OFF \
  -DENABLE_SCRIPTING=ON -DUSE_LUA=ON -DUSE_FFMPEG=OFF -DUSE_LIBZIP=OFF -DUSE_DISCORD_RPC=OFF -DBUILD_TEST=OFF -DBUILD_SUITE=OFF > /root/mgba-cmake.log 2>&1 \
  || { tail -30 /root/mgba-cmake.log; exit 1; }
ninja -j 8 2>&1 | grep -E "error|FAILED|warning: unused|^\[" | tail -20
ls -la /root/mgba-build/sdl/mgba
