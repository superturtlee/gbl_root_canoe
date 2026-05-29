#!/bin/bash
set -e

# ══════════════════════════════════════
# 配置
# ══════════════════════════════════════
NDK=$NDK_PATH
API=31
TRIPLE="aarch64-linux-android"
SRC="extractfv.c"
OUT_DIR="build"
XZ_VER="5.4.5"
XZ_URL="https://tukaani.org/xz/xz-${XZ_VER}.tar.gz"
XZ_SRC="third_party/xz"
LZMA_BUILD="build/lzma-aarch64"
LZMA_INSTALL="build/lzma-install-aarch64"

cd "$(dirname "$0")"/../

# ══════════════════════════════════════
# 检查 NDK
# ══════════════════════════════════════
TOOLCHAIN="$NDK/toolchains/llvm/prebuilt/linux-x86_64"
[ ! -d "$TOOLCHAIN" ] && TOOLCHAIN="$NDK/toolchains/llvm/prebuilt/darwin-x86_64"

if [ ! -d "$TOOLCHAIN" ]; then
    echo "Error: NDK toolchain not found."
    echo "Set NDK_PATH to your NDK path."
    exit 1
fi

CC="${TOOLCHAIN}/bin/${TRIPLE}${API}-clang"

if [ ! -f "$CC" ]; then
    echo "Error: Compiler not found: $CC"
    exit 1
fi

echo "NDK Compiler: $CC"

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
    echo "[2/3] Building liblzma for arm64..."
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
        CC="$CC" \
        CFLAGS="-O2 -fPIC"

    make -j$(nproc)
    make install

    cd ../..
    echo "[2/3] liblzma built -> ${LZMA_INSTALL}/lib/liblzma.a"
fi

# ══════════════════════════════════════
# Step 3: 编译 extractfv
# ══════════════════════════════════════
echo "[3/3] Compiling extractfv for arm64-v8a..."

mkdir -p "$OUT_DIR"

$CC -O2 -std=c11 \
    -Wall -Wextra -Wno-unused-parameter \
    -D__ANDROID__ \
    -I"${LZMA_INSTALL}/include" \
    -o "${OUT_DIR}/extractfv_android" \
    "$SRC" \
    -L"${LZMA_INSTALL}/lib" \
    -llzma \
    -static

