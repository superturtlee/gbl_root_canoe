#!/bin/bash
docker run -it --rm \
  --user "$(id -u):$(id -g)" \
  -v "$(pwd)":/workspace \
  -w /workspace \
  ${NDK_PATH:+-v "$NDK_PATH":/opt/android-ndk:ro} \
  gbl_builder:latest /bin/bash
