#!/usr/bin/env bash

# Ensure in the arduino project root directory.
SCRIPT_DIR="$(realpath "$(dirname "$0")")"
cd "$SCRIPT_DIR/.." || exit 1

# HACK: add a symlink to the top level includes directory to be included
# in the compilation.
INCLUDE=$(realpath ../include)
ln -s "$INCLUDE" .

arduino-cli compile \
  arduino-433-transmitter.ino \
  --fqbn esp8266:esp8266:nodemcuv2:baud=115200

# Remove the symlink when done.
rm ./include
