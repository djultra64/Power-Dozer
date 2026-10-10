#!/usr/bin/env bash
set -euo pipefail
cd -- "$(dirname -- "${BASH_SOURCE[0]}")"
export SDL_VIDEODRIVER="${SDL_VIDEODRIVER:-x11}"
export SDL_AUDIODRIVER="${SDL_AUDIODRIVER:-pulseaudio}"
# Desktop beta: keep running, widescreen, RT64 interpolation enabled.
# Later arguments override --seconds, --scale and --refresh for diagnostics.
exec ./build/blastcorps-prototype --seconds 0 --scale 3 --wide --refresh 120 "$@"
