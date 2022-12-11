#!/usr/bin/env bash

arduino-cli compile \
  arduino-433-transmitter.ino \
  -v \
  --fqbn esp8266:esp8266:nodemcuv2:baud=115200

