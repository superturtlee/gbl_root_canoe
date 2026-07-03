# Build Guide

## Prerequisites

Builds must run on Linux. Install `gcc` or `clang`, `make`, `zip`, `python3`, `liblzma-dev`, and MinGW-w64 for Windows packages. Set `NDK_PATH` to your Android NDK directory before building the Magisk module.

## Release Packages

Build the Linux PC toolkit:

```bash
make target_toolkit_linux
```

Build the Windows PC toolkit:

```bash
make target_toolkit_windows
```

Build the Magisk module:

```bash
NDK_PATH=/path/to/android-ndk make target_magisk_module
```

Build optional VBMeta fixer tools:

```bash
make tools_vbmetafixer_linux
make tools_vbmetafixer_windows
```

## Patching ABL

The toolkit build itself does not require an ABL image. To patch a device image, extract the toolkit, place the stock image at `images/abl.img` or `images/abl.elf`, and run `bash build.sh` on Linux or `build.bat` on Windows. The patched output is `ABL.efi`.
