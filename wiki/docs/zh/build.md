# 构建指南

## 依赖

构建需要在 Linux 环境下进行。请安装 `gcc` 或 `clang`、`make`、`zip`、`python3`、`liblzma-dev`。如需构建 Windows 工具包，还需要 MinGW-w64；如需构建 Magisk 模块，需要安装 Android NDK 并设置 `NDK_PATH`。

## 发布包构建

构建 Linux PC 工具包：

```bash
make target_toolkit_linux
```

构建 Windows PC 工具包：

```bash
make target_toolkit_windows
```

构建 Magisk 模块：

```bash
NDK_PATH=/path/to/android-ndk make target_magisk_module
```

构建可选的 VBMeta fixer 工具：

```bash
make tools_vbmetafixer_linux
make tools_vbmetafixer_windows
```

## 修补 ABL

构建工具包本身不需要 ABL 镜像。如需修补设备镜像，请解压工具包，将官方镜像以 `images/abl.img` 或 `images/abl.elf` 文件名放入，然后在 Linux 执行 `bash build.sh`，或在 Windows 执行 `build.bat`。修补后的输出文件为 `ABL.efi`。
