#!/usr/bin/env bash

# Change directory into root.
CUR_DIR=$(dirname "$0")
ROOT_DIR=$(realpath "$CUR_DIR/..")
cd "$ROOT_DIR" || exit 1
mkdir -p build

# Build for arm64.
export GOOS="linux"
export GOARCH="arm64"
go build -o build/ingestor