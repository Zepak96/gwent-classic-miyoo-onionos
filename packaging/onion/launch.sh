#!/bin/sh
set -eu
GAME_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
export LD_LIBRARY_PATH="$GAME_DIR:/config/lib:/customer/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
export XDG_DATA_HOME="/mnt/SDCARD/Saves/Ports/GwentClassic"
export GWENT_ASSET_DIR="$GAME_DIR/assets/gwent-classic"
mkdir -p "$XDG_DATA_HOME"
cd "$GAME_DIR"
exec "$GAME_DIR/gwent" "$@"
