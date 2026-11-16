#!/bin/sh
set -u
GAME_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
export LD_LIBRARY_PATH="$GAME_DIR:/config/lib:/customer/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
export XDG_DATA_HOME="/mnt/SDCARD/Saves/Ports/GwentClassic"
export GWENT_ASSET_DIR="$GAME_DIR/assets/gwent-classic"
mkdir -p "$XDG_DATA_HOME" 2>/dev/null || true
cd "$GAME_DIR"
LOG_FILE="$XDG_DATA_HOME/last-run.log"
if ! : > "$LOG_FILE" 2>/dev/null; then
  LOG_FILE="$GAME_DIR/last-run.log"
fi
if : > "$LOG_FILE" 2>/dev/null; then
  exec >> "$LOG_FILE" 2>&1
  printf 'Gwent Classic launcher started: %s\n' "$GAME_DIR"
  printf 'Executable: %s/gwent\n' "$GAME_DIR"
else
  printf 'Could not write startup log: %s\n' "$LOG_FILE" >&2
fi
exec "$GAME_DIR/gwent" "$@"
