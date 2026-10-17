#!/usr/bin/env bash
# Fetch the source sample libraries (sparse clones) into ../ai-src, then run
#   python3 tools/prepare_samples.py ../ai-src Instruments
# Requires git, ffmpeg (libvorbis), python3 + numpy + soundfile.
set -euo pipefail
SRC="${1:-../ai-src}"; mkdir -p "$SRC"; cd "$SRC"
clone() { [ -d "$2" ] || git clone -q --depth 1 --filter=blob:none --sparse "https://github.com/$1" "$2"; }
clone sfzinstruments/SalamanderGrandPiano SalamanderGrandPiano
(cd SalamanderGrandPiano && git sparse-checkout set --no-cone '/LICENSE' '/README.md' '/Samples/*v4.flac' '/Samples/*v8.flac' '/Samples/*v12.flac' '/Samples/*v15.flac')
clone sfzinstruments/virtuosity_drums virtuosity_drums
(cd virtuosity_drums && git sparse-checkout set --no-cone '/LICENSE' '/README.md' '/Samples/mid/kick/*snon*' '/Samples/mid/snare/*center*' '/Samples/mid/snare/*rimshot*' '/Samples/mid/snare/*crossstick*' '/Samples/mid/hh/*' '/Samples/mid/htom/*center*' '/Samples/mid/ltom/*center*' '/Samples/mid/crash/*crash_crash*' '/Samples/mid/ride/*' '/Samples/mid/flatride/*crash*')
clone sgossner/VSCO-2-CE vsco
(cd vsco && git sparse-checkout set --no-cone '/LICENSE' '/README.md' '/Strings/Cello Section/susvib/' '/Strings/Viola Section/susvib/' '/Strings/Violin Section/susVib/')
