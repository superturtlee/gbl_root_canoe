FROM ubuntu:22.04
LABEL org.opencontainers.image.title="gbl_builder"
ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    python3 python3-pip python-is-python3 \
    build-essential make ninja-build git clang lld llvm \
    wget curl ca-certificates lsb-release software-properties-common gnupg \
    gcc-aarch64-linux-gnu g++-aarch64-linux-gnu \
    zip vim xxd uuid-dev \
    nasm acpica-tools libssl-dev bc bison flex \
    device-tree-compiler liblzma-dev \
    mingw-w64 \
    && rm -rf /var/lib/apt/lists/*

ENV NDK_PATH=/opt/android-ndk

WORKDIR /workspace
