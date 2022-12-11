#!/usr/bin/env bash

arduino-cli upload \
  --fqbn esp8266:esp8266:nodemcuv2 \
  --port /dev/ttyUSB0
