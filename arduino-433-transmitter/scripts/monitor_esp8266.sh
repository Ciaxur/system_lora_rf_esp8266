#!/usr/bin/env bash

DEV="$1"

if [[ "$DEV" = "" ]]; then
  echo "Provide a device to monitor"
  exit 1
fi

arduino-cli monitor \
  -p "$DEV"  \
  -c baudrate=9600
