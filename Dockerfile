FROM ubuntu:22.04
LABEL org.opencontainers.image.title="gbl_builder"
ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    python3 python3-pip python-is-python3 \
    build-essential make ninja-build git \
    wget lsb-release software-properties-common gnupg \
    gcc-aarch64-linux-gnu g++-aarch64-linux-gnu \
    zip vim xxd uuid-dev \
    nasm acpica-tools libssl-dev bc bison flex \
    device-tree-compiler liblzma-dev\
    && rm -rf /var/lib/apt/lists/*

RUN wget https://apt.llvm.org/llvm.sh && \
    chmod +x llvm.sh && \
    ./llvm.sh 20 all && \
    rm llvm.sh

RUN ln -s /usr/bin/clang-20 /usr/bin/clang && \
    ln -s /usr/bin/clang++-20 /usr/bin/clang++ && \
    ln -s /usr/bin/lld-20 /usr/bin/lld && \
    ln -s /usr/bin/llvm-ar-20 /usr/bin/llvm-ar && \
    ln -s /usr/bin/lld-20 /usr/bin/ld.lld && \
    ln -sf /usr/bin/llvm-objcopy-20 /usr/bin/llvm-objcopy && \
    ln -sf /usr/bin/llvm-strip-20 /usr/bin/llvm-strip

WORKDIR /workspace
