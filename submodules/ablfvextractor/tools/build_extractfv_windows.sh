#!/bin/bash
set -e

# ══════════════════════════════════════
# 配置
# ══════════════════════════════════════
TRIPLE="x86_64-w64-mingw32"
SRC="extractfv.c"
OUT_DIR="build"
XZ_VER="5.4.5"
XZ_URL="https://tukaani.org/xz/xz-${XZ_VER}.tar.gz"
XZ_SRC="third_party/xz"
LZMA_BUILD="build/lzma-mingw64"
LZMA_INSTALL="build/lzma-install-mingw64"

cd "$(dirname "$0")"/../

# ══════════════════════════════════════
# Step 1: 下载 xz 源码
# ══════════════════════════════════════
if [ ! -d "$XZ_SRC" ]; then
    echo "[1/3] Downloading xz-${XZ_VER}..."
    mkdir -p third_party
    curl -sL "$XZ_URL" | tar xz -C third_party
    mv "third_party/xz-${XZ_VER}" "$XZ_SRC"
else
    echo "[1/3] xz source already exists, skipping download"
fi

# ══════════════════════════════════════
# Step 2: 交叉编译 liblzma 静态库
# ══════════════════════════════════════
if [ -f "${LZMA_INSTALL}/lib/liblzma.a" ]; then
    echo "[2/3] liblzma already built, skipping"
else
    echo "[2/3] Building liblzma for mingw64..."
    mkdir -p "$LZMA_BUILD" "$LZMA_INSTALL"

    LZMA_INSTALL_ABS="$(cd "$LZMA_INSTALL" && pwd)"
    XZ_SRC_ABS="$(cd "$XZ_SRC" && pwd)"

    cd "$LZMA_BUILD"

    "${XZ_SRC_ABS}/configure" \
        --host="${TRIPLE}" \
        --prefix="${LZMA_INSTALL_ABS}" \
        --enable-static \
        --disable-shared \
        --disable-xz \
        --disable-xzdec \
        --disable-lzmadec \
        --disable-lzmainfo \
        --disable-scripts \
        --disable-doc \
        --disable-nls \
        CFLAGS="-O2"

    make -j$(nproc)
    make install

    cd ../..
    echo "[2/3] liblzma built -> ${LZMA_INSTALL}/lib/liblzma.a"
fi

# ══════════════════════════════════════
# Step 3: 编译 extractfv
# ══════════════════════════════════════
echo "[3/3] Compiling extractfv for Windows..."

mkdir -p "$OUT_DIR"

${TRIPLE}-gcc -O2 -std=c11 \
    -Wall -Wextra -Wno-unused-parameter \
    -I"${LZMA_INSTALL}/include" \
    -o "${OUT_DIR}/extractfv.exe" \
    "$SRC" \
    "${LZMA_INSTALL}/lib/liblzma.a"
