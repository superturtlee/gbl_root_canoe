# GBL Root Canoe

[English](README.md)

`gbl_root_canoe` 是一个基于 EDK2 的工作区，主要用于修补高通 ABL 内的efi程序。该项目利用 GBL (Generic Bootloader Loader) 漏洞注入自定义efi，其主要目的是在骁龙 8 Gen 5 / 8 Elite (Gen 5) 设备上实现**假回锁**（绕过 Bootloader 的解锁状态检测）。修补后的efi通常会被直接注入并刷入手机的 `efisp` 分区中。

---

## 开发者构建指南 (Builder Guide)

本节适用于希望从源码编译工具包的开发者。

### 编译依赖
构建各种发布包必须在 **Linux** 环境下进行：
- `gcc` / `clang`, `lld`, `make`, `zip`, `python3`
- `liblzma-dev` (用于编译 `extractfv` 解包工具)
- **Android NDK**（用于 `make target_magisk_module` 构建 Android 平台的修补工具；需要将 `NDK_PATH` 指向 NDK 目录）
- **MinGW-w64**

### 核心构建目标

**注意**：在仅编译工具包或 Magisk 模块时，**不需要**事先提供 `abl.img`。
- **`make target_toolkit_linux`**
  构建 Linux PC 工具包，包含 `extractfv` 和 `patch_abl`。输出文件为 `targets/toolkit_linux/build/toolkit_linux.zip`。

- **`make target_toolkit_windows`**
  构建 Windows PC 工具包，使用 MinGW-w64 将修补工具交叉编译为 `.exe` 文件。输出文件为 `targets/toolkit_windows/build/toolkit_windows.zip`。

- **`make target_magisk_module`**
  使用 NDK 将修补工具交叉编译至 Android 原生平台架构，并封装为一个标准的 Magisk 模块。输出文件为 `targets/magisk_module/build/module_android.zip`。

- **`make tools_vbmetafixer_linux`** / **`make tools_vbmetafixer_windows`**
  构建可选的 VBMeta fixer 工具包。

### Docker 构建

你可以使用项目自带的 Docker 镜像构建 Linux 和 Windows PC 工具包：

```bash
bash docker_build.sh
bash run_docker.sh
make clean
make target_toolkit_linux
make target_toolkit_windows
```

如需在 Docker 中构建 Magisk 模块，请先在宿主机安装 Android NDK，并通过 `NDK_PATH` 传入：

```bash
NDK_PATH=/path/to/android-ndk bash run_docker.sh
make target_magisk_module
```

---

## 普通用户使用指南 (User Guide)

本节适用于获取了编译产物位于 `release/` 目录中工具包或模块的普通用户。

### 1. 使用 Magisk 模块版本（手机端热修补）

Magisk 版本可直接通过 Root 管理器在有 Root 权限的手机上刷入运行。

**设备要求：**
- 必须是骁龙 8 Gen 5 / 8 Elite (Gen 5) 芯片设备。
- 设备 BL 锁已经解锁。
- 内核**没有** Baseband Guard 拦截。

**刷入及使用流程：**
在使用修补模块（如 KernelSU/Magisk/APatch 等）刷入该压缩包时，脚本会通过音量键与您交互：
- **按音量上键 (首次全新安装)：** 安装脚本会自动提取手机当前活动的 `.abl` 镜像，为其打补丁，并将修补后的文件刷写入 `/dev/block/by-name/efisp` 分区。完事后，请重启手机进入 Recovery 模式**格式化Data**。开机后，请再次刷入本模块（第二次刷入时应按音量下键）以走完完整安装流程。
- **按音量下键 (非首次安装的日常 OTA 保留)：** 用作 OTA 更新后保留 BL 版本的修补选项，OTA前使用模块自动降级abl后重启系统即可。

### 2. 使用 PC 工具包 (Linux / Windows)

如果你下载的是 `target_toolkit_loader` 或 `target_toolkit_windows` 的发布压缩包：
1. 请先解压该 zip 并进入套件文件夹。
2. 提取出你所用机型的官方 ABL 镜像，并以 `abl.img` 或 `abl.elf` 文件名放入套件中的 `images/`（或 `images\`）目录下。
3. **Linux平台：** 开启终端执行 `bash build.sh`。 **Windows平台：** 双击运行 `build.bat`。
4. 等待脚本自动为你提取固件并打补丁。结束后同目录下的 `ABL.efi` 即为修补后的文件，同时会生成 `ABL_original.efi` 和 `patch_log.txt`。（注意查看反馈日志，若显示 "Warning: Failed to patch ABL GBL" 则说明你的设备没有该 GBL 漏洞，需要降级abl）。

### 3. 使用预先补丁efi
下载包含文件名中含有手机型号或代号的特定发布版本。使用其中的 `ABL.efi` 通过 `fastboot` 命令进行引导或刷写 (比如 `fastboot flash efisp ABL.efi`)。

### 4. 使用 generic efi（弃用）
当前 Makefile 不再包含 generic EFI 构建目标。请优先使用机型对应的 `ABL.efi`。

### 5. OTA升级
使用模块在重启前刷写保留旧版本abl，如果跨版本更新，建议勾选“更新efisp”，否则卡一屏。

### 6. superfastboot使用方法
开启 OEM 解锁且开机出现小白字时，必须按 **音量减**（Volume Down）键才能进入 Superfastboot 模式。常用命令包括：
- **临时启动EFI文件（无需刷入）**：`fastboot boot xxx.efi`
- **锁定与解锁 (BL 锁相关)**：
  - 锁定 BL，触发数据清除：`fastboot flashing lock`
  - 解锁 BL，不触发数据清除：`fastboot flashing unlock` 或 `fastboot flashing unlock_critical`
  - 注意：如果遇到 TEE 状态不一致的情况，设备会拒绝下发 data key 导致数据无法访问。
- **刷写与擦除**：
  - `fastboot flash <partition> <file.img>`
  - `fastboot erase <partition>`
- **重启设备**：
  - `fastboot reboot bootloader` （下一次正常启动进入官方 Fastboot）
  - `fastboot reboot recovery`
  - `fastboot reboot`

### 7. 不同变体说明
1. `ABL.efi`:修补后的abl
2. `ABL_original.efi`:开发者IDA分析用，用于故障反馈，**不能刷入**
3. `patch_log.txt`: 工具包生成的补丁日志
