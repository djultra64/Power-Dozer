#!/usr/bin/env bash
set -euo pipefail
cd -- "$(dirname -- "${BASH_SOURCE[0]}")"
export SDL_VIDEODRIVER="${SDL_VIDEODRIVER:-x11}"
export SDL_AUDIODRIVER="${SDL_AUDIODRIVER:-pulseaudio}"
# Saved preferences apply first; later arguments override this launch.
exec ./build/blastcorps-prototype --seconds 0 "$@"
