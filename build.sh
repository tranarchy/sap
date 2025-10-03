#!/bin/sh

SOURCE_FILES="src/main.c"
RENDER_SOURCE_FILES="src/render/glad.c src/render/microui.c src/render/renderer.c"

SDL="$(pkg-config --cflags --libs sdl2,SDL2_mixer)"
STDFlAGS="$SDL -Iinclude -lm -Wall -O2"

OUTPUT="sap"

TARGET="$1"

if [ "$TARGET" = "" ]; then
  set -x

  cc $SOURCE_FILES $RENDER_SOURCE_FILES $STDFlAGS -o $OUTPUT
elif [ "$TARGET" = "install" ]; then
  set -x

  cp $OUTPUT /usr/local/bin
fi
