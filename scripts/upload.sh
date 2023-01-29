#!/usr/bin/env bash

DEV="$1"

if [[ "$DEV" = "" ]]; then
  echo "Provide a device to upload to"
  exit 1
fi

arduino-cli upload \
  --fqbn esp8266:esp8266:nodemcuv2 \
  --port "$DEV"
