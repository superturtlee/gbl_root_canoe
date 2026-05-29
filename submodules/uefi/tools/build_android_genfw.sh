#!/bin/bash
set -e

NDK=$NDK_PATH
API=31
TRIPLE="aarch64-linux-android"
OUT_DIR="build"

cd "$(dirname "$0")"/../

TOOLCHAIN="$NDK/toolchains/llvm/prebuilt/linux-x86_64"

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

BASETOOLS="edk2/BaseTools/Source/C"
INCLUDE="-I${BASETOOLS} -I${BASETOOLS}/Include -I${BASETOOLS}/Include/Common -I${BASETOOLS}/Include/IndustryStandard -I${BASETOOLS}/Include/AArch64 -I${BASETOOLS}/Common -I${BASETOOLS}/GenFw"

COMMON_SRCS="
    ${BASETOOLS}/Common/BasePeCoff.c
    ${BASETOOLS}/Common/BinderFuncs.c
    ${BASETOOLS}/Common/CommonLib.c
    ${BASETOOLS}/Common/Crc32.c
    ${BASETOOLS}/Common/Decompress.c
    ${BASETOOLS}/Common/EfiCompress.c
    ${BASETOOLS}/Common/EfiUtilityMsgs.c
    ${BASETOOLS}/Common/FirmwareVolumeBuffer.c
    ${BASETOOLS}/Common/FvLib.c
    ${BASETOOLS}/Common/MemoryFile.c
    ${BASETOOLS}/Common/MyAlloc.c
    ${BASETOOLS}/Common/OsPath.c
    ${BASETOOLS}/Common/ParseGuidedSectionTools.c
    ${BASETOOLS}/Common/ParseInf.c
    ${BASETOOLS}/Common/PeCoffLoaderEx.c
    ${BASETOOLS}/Common/SimpleFileParsing.c
    ${BASETOOLS}/Common/StringFuncs.c
    ${BASETOOLS}/Common/TianoCompress.c
"

GENFW_SRCS="
    ${BASETOOLS}/GenFw/GenFw.c
    ${BASETOOLS}/GenFw/ElfConvert.c
    ${BASETOOLS}/GenFw/Elf32Convert.c
    ${BASETOOLS}/GenFw/Elf64Convert.c
"

CFLAGS="-O2 -fshort-wchar -fno-strict-aliasing -fwrapv -Wall -Wno-deprecated-declarations -Wno-unused-result -D__ANDROID__"

mkdir -p "$OUT_DIR"

echo "Compiling GenFw for arm64..."
$CC $CFLAGS $INCLUDE \
    -o "${OUT_DIR}/GenFw_android" \
    $GENFW_SRCS $COMMON_SRCS \
    -static