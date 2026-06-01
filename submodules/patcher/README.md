# PatchLib — ABL Binary Patcher

[中文版](#中文)

## Overview

PatchLib patches the compiled ABL (Android Bootloader) binary to achieve a fake-locked bootloader state. The entire patcher runs in **userspace** — on a Linux/Windows PC or on-device via a Magisk module — and produces a modified ABL that can be flashed to the `efisp` partition. No UEFI-level runtime logic is needed.

## Design Principles

### 1. Minimal Patch Set

The core functionality requires only **4 instruction-level rewrites** inside one function (`LoadImageAndAuthVB2` in `VerifiedBoot.c`). Every patch targets a specific compiled variable or data path; nothing is patched "just in case."

### 2. Userspace-First Architecture

All heavy lifting — ELF/FV/LZMA parsing (`extractfv`), ARM64 instruction analysis (`arm64_inst_decoder.h`), data-flow tracking (`patchlib.h`) — runs in userspace toolchains (Linux toolkit, Windows toolkit, Magisk module). The patcher itself is a standalone C program with **no UEFI dependencies**. The `types.h` abstraction layer allows the same code to compile for both userspace (`stdio.h`/`stdlib.h`) and UEFI (`UefiLib.h`) if needed, but the design intent is: **patch offline, flash once, boot clean**.

### 3. Instruction Analysis over Hardcoding

Instead of hardcoding byte offsets (which break on every ABL recompile), the patcher uses:

- **Anchor strings** — unique compiler-emitted strings (`"unlocked"`, `"locked"`, `"androidboot.vbmeta.device_state"`, `"Orange State\n"`) to locate code regions.
- **ARM64 instruction decoder** — a 20+ instruction-type decoder (`arm64_inst_decoder.h`) that understands ADRP+ADD pairs, LDRB/STRB/LDR/STR variants, MOV, CBZ, PACIASP boundaries, etc.
- **Data-flow tracking** — `find_ldrB_instructio_reverse` walks backward from an anchor point through register-to-stack spills and reloads (up to 8 bounces) to find the true source of a value. `track_forward_patch_strb` walks forward to find where a value is consumed. This tracks compiler register allocation changes across ABL versions.

The only hardcoded byte pattern is the boot-state anchor (`Original[]`) — an 8-instruction sequence whose structure is dictated by the `if/else` in `LoadImageAndAuthVB2:1887-1895`, which is stable across compiler versions because the control-flow shape is fixed.

### 4. Function Boundary Protection

All optional patches respect `PACIASP` as a function boundary marker. The backward tracker (`find_ldrB_instructio_reverse`) and forward tracker (`track_forward_patch_strb`) both stop at `PACIASP`, preventing cross-function corruption. A `MAX_BOUNCES` limit (8) prevents runaway tracking through pathological register-spill chains.

### 5. Graceful Degradation

Each patch has an explicit criticality level. Failures are handled per-patch, not globally:

- **Critical patches** → failure aborts the entire process (`return 0` / `return FALSE`).
- **Important but non-critical patches** → failure prints a warning, continues.
- **Optional patches** → failure is silently tolerated.

See the Patch Reference below for the classification of each patch.

## Patch Reference

All patches target the compiled output of `edk2/QcomModulePkg/Library/avb/VerifiedBoot.c` (function `LoadImageAndAuthVB2`) and related modules.

### Critical Patches — Failure Aborts

These target `LoadImageAndAuthVB2` internals. If any critical patch fails, the output ABL will either fail to boot or cause TEE to reject the data encryption key, bricking user data access. PatchBuffer returns FALSE on failure.

| ID | Function | Source Line | What It Does | Why Critical |
|----|----------|-------------|-------------|--------------|
| **Patch 3** | `patch_abl_bootstate` | VerifiedBoot.c:1887 | Locates the `if(AllowVerificationError) BootState=ORANGE; else BootState=GREEN` compiled pattern. Extracts the lock-state register number and anchor offset for subsequent patches. | This is the anchor for all data-flow patches. Without it, patches 4 and 5 cannot locate their targets. |
| **Patch 4** | `source_callback` | VerifiedBoot.c:1503 | Tracks backward from the anchor to find `LDRB Wn, [Xbase, #off]` (the compiled `AllowVerificationError = IsUnlocked()`) and rewrites it to `MOV Wn, #1`. | Forces `AllowVerificationError = TRUE`. Without this, AVB verification rejects modified partitions and ABL refuses to boot. Also prevents `UpdateRollbackIndex` from being set (VerifiedBoot.c:1742), which naturally blocks TZ soft-fuse (`TZ_BLOW_SW_FUSE_ID`) and rollback version updates (`TZ_UPDATE_ROLLBACK_VERSION`) — those calls only execute when `UpdateRollbackIndex == TRUE`. |
| **Patch 5** | `track_forward_patch_strb` | VerifiedBoot.c:1904 | Tracks forward from the anchor to find the STRB that writes `Data.IsUnlocked = AllowVerificationError` and replaces the source register with WZR (= 0). | Forces `Data.IsUnlocked = 0` in the `KMRotAndBootState` struct passed to `KeyMasterSetRotAndBootState()`. This struct feeds `SET_ROT` (0x201: `RotDigest = SHA256(PublicKey \|\| IsUnlocked)`) and `SET_BOOT_STATE` (0x208), then mirrors to SPU via `ShareKeyMintInfoWithSPU()`. If IsUnlocked=1 reaches KM/SPU, TEE refuses to release the FBE data encryption key → user data inaccessible. |

**Note on BootState=GREEN**: Patch 3's anchor locates the compiled `if/else` that assigns `Info->BootState`. With `AllowVerificationError = TRUE` (from patch 4), the original code would assign `ORANGE`. The forward-tracking STRB sink patch (patch 5) writes WZR to the BootState store, forcing `GREEN` (= 0). The `Data.Color` field at VerifiedBoot.c:1903 reads from `Info->BootState`, so both the kernel cmdline color and the KM `SET_BOOT_STATE` color become GREEN.

### Important but Non-Critical — Failure Continues

| ID | Function | Source Line | What It Does | Failure Impact |
|----|----------|-------------|-------------|----------------|
| **Patch 2** | `patch_adrl_unlocked_to_locked` | avb_cmdline.c:327 | Finds ADRP+ADD pairs loading `"unlocked"` and `"locked"` strings near `"androidboot.vbmeta.device_state"`, redirects the `"unlocked"` pointer to `"locked"`. | Kernel cmdline shows `device_state=unlocked`. Android framework detects unlocked state. Device **still boots and data is accessible**, but bootloader status is exposed to apps. Some integrity-checking apps (banking, etc.) may refuse to run. |

**Design note**: Patch 2 fails on some ABL versions where the compiler inlines or reorders the ADRP+ADD pairs. When it fails, PatchBuffer prints a warning and continues — the device is fully functional, only the cmdline leaks the true unlock state.

**Ambiguity guard**: If patch 2 matches more than one location, PatchBuffer aborts (`return FALSE`). A single match is expected; multiple matches indicate a misidentification that could corrupt unrelated code.

### Optional Patches — Failure Tolerated

| ID | Function | Target | What It Does | Failure Impact |
|----|----------|--------|-------------|----------------|
| **Patch 1** | `patch_abl_gbl` | ABL binary | Replaces UTF-16 `"efisp"` → `"nulls"` in ABL. | Prevents ABL from recursively loading efisp. |
| **Patch 6** | `patch_string_jump` | FastbootCmds.c | NOPs conditional branches that jump to `"is not allowed in Lock State"` error paths in fastboot command handlers (flash, erase, slot-change, snapshot-cancel). | Fastboot refuses flash/erase commands on a relocked device. Only matters for rescue scenarios where the user needs to reflash while in fake-locked state. |
| **Warning** | `patch_warning` | VerifiedBootMenu.c | Finds the CBZ that gates the "Orange State" / "Your device has been unlocked" warning screen, changes it to `CBZ WZR` (always skip). | Orange warning text appears on boot. Cosmetic only — **on OnePlus/Oppo devices** the OEM-specific warning check may not bypass this automatically when BootState=GREEN. That is different from public versions |

### Compile-Time Patch Control

Each patch can be individually disabled at compile time via preprocessor defines:

```c
-DDISABLE_PATCH_1   // skip efisp→nulls rename
-DDISABLE_PATCH_2   // skip cmdline unlocked→locked
-DDISABLE_PATCH_3   // skip boot-state anchor rewrite
-DDISABLE_PATCH_4   // skip AllowVerificationError MOV
-DDISABLE_PATCH_5   // skip STRB sink (Data.IsUnlocked)
-DDISABLE_PATCH_6   // skip fastboot lock-gate NOP
-DDISABLE_PRINT     // suppress all patcher output
```

## Architecture

```
                    ┌─────────────────────────────────┐
                    │         PatchBuffer()            │
                    │  (orchestrator, returns BOOL)    │
                    └──┬──────┬──────┬──────┬──────┬──┘
                       │      │      │      │      │
              ┌────────┘  ┌───┘  ┌───┘  ┌───┘  ┌───┘
              ▼           ▼      ▼      ▼      ▼
        patch_abl_gbl  patch2  patch6  patch3  patch_warning
        [optional]     [warn]  [opt]   [critical]  [optional]
                                         │
                                    ┌────┴────┐
                                    ▼         ▼
                              backward     forward
                              tracker      tracker
                            (patch 4)    (patch 5)
                            [critical]   [critical]

  ┌──────────────────────────────────────────────────────────┐
  │                  arm64_inst_decoder.h                     │
  │  20+ instruction decoders, encoder helpers,              │
  │  PACIASP boundary detection, LocSet data-flow tracker    │
  └──────────────────────────────────────────────────────────┘
```

## How It Avoids UEFI Complexity

The patcher operates on the ABL PE binary **as a flat byte buffer** in userspace. It does not:

- Hook UEFI protocol vtables at runtime
- Intercept SCM/QSEECOM/SPSS calls
- Require a chainloaded EFI application
- Need a logging filesystem or runtime manifest

---

<a name="中文"></a>

# PatchLib — ABL 二进制补丁库

## 概述

PatchLib 通过补丁编译后的 ABL（Android Bootloader）二进制来实现假锁（Fake Locked Bootloader）状态。整个补丁流程在**用户空间**运行 — PC 端 Linux/Windows 工具包或设备端 Magisk 模块 — 生成修改后的 ABL 刷入 `efisp` 分区。不需要 UEFI 层的运行时逻辑。

## 设计原则

### 1. 最小补丁集

核心功能只需要 **4 处指令级改写**，全部位于一个函数内（`VerifiedBoot.c` 的 `LoadImageAndAuthVB2`）。每处补丁都有明确的目标变量或数据路径，不做预防性补丁。

### 2. 用户空间优先架构

所有重计算 — ELF/FV/LZMA 解析（`extractfv`）、ARM64 指令分析（`arm64_inst_decoder.h`）、数据流追踪（`patchlib.h`）— 都在用户空间工具链中运行（Linux 工具包、Windows 工具包、Magisk 模块）。补丁程序是独立的 C 程序，**不依赖 UEFI**。`types.h` 抽象层允许同一份代码同时编译为用户空间（`stdio.h`/`stdlib.h`）和 UEFI（`UefiLib.h`）版本，但设计意图是：**离线补丁，一次刷写，干净启动**。

### 3. 指令分析取代硬编码

补丁不硬编码字节偏移（每次 ABL 重编译都会失效），而是使用：

- **锚点字符串** — 编译器保留的唯一字符串（`"unlocked"`、`"locked"`、`"androidboot.vbmeta.device_state"`、`"Orange State\n"`）定位代码区域。
- **ARM64 指令解码器** — 支持 20+ 种指令类型的解码器（`arm64_inst_decoder.h`），理解 ADRP+ADD 对、LDRB/STRB/LDR/STR 各变体、MOV、CBZ、PACIASP 边界等。
- **数据流追踪** — `find_ldrB_instructio_reverse` 从锚点向后遍历寄存器-栈溢出/重载链（最多 8 次跳转）找到值的真正来源。`track_forward_patch_strb` 向前遍历找到值的消费点。这种追踪适应编译器跨 ABL 版本的寄存器分配变化。

唯一的硬编码字节模式是启动状态锚点（`Original[]`）— 一个 8 条指令的序列，其结构由 `LoadImageAndAuthVB2:1887-1895` 的 `if/else` 决定。这段控制流形状在编译器版本间是稳定的。

### 4. 函数边界保护

所有可选补丁都以 `PACIASP` 作为函数边界标记。反向追踪器（`find_ldrB_instructio_reverse`）和正向追踪器（`track_forward_patch_strb`）都在遇到 `PACIASP` 时停止，防止跨函数破坏。`MAX_BOUNCES` 限制（8 次）防止在病态寄存器溢出链中失控追踪。

### 5. 优雅降级

每个补丁有明确的关键性级别，失败按补丁单独处理，而非全局中止：

- **关键补丁** → 失败中止整个流程（`return 0` / `return FALSE`）。
- **重要但非关键补丁** → 失败打印警告，继续执行。
- **可选补丁** → 失败静默容忍。

各补丁的分类见下方补丁参考表。

## 补丁参考

所有补丁目标为 `edk2/QcomModulePkg/Library/avb/VerifiedBoot.c`（函数 `LoadImageAndAuthVB2`）及相关模块的编译产物。

### 关键补丁 — 失败中止

这些补丁针对 `LoadImageAndAuthVB2` 内部。如果任何关键补丁失败，输出的 ABL 要么无法启动，要么导致 TEE 拒绝下发数据加密密钥，使用户数据无法访问。PatchBuffer 在失败时返回 FALSE。

| 编号 | 函数 | 源码行 | 补丁内容 | 为何关键 |
|------|------|--------|---------|---------|
| **补丁 3** | `patch_abl_bootstate` | VerifiedBoot.c:1887 | 定位编译后的 `if(AllowVerificationError) BootState=ORANGE; else BootState=GREEN` 模式。提取锁状态寄存器号和锚点偏移。 | 这是所有数据流补丁的锚点。没有它，补丁 4 和 5 无法定位目标。 |
| **补丁 4** | `source_callback` | VerifiedBoot.c:1503 | 从锚点反向追踪找到 `LDRB Wn, [Xbase, #off]`（编译后的 `AllowVerificationError = IsUnlocked()`），改写为 `MOV Wn, #1`。 | 强制 `AllowVerificationError = TRUE`。没有这个，AVB 验证会拒绝修改过的分区，ABL 拒绝启动。同时阻止 `UpdateRollbackIndex` 被置位（VerifiedBoot.c:1742），自然阻断了 TZ soft-fuse（`TZ_BLOW_SW_FUSE_ID`）和回滚版本更新（`TZ_UPDATE_ROLLBACK_VERSION`）— 这些调用只在 `UpdateRollbackIndex == TRUE` 时执行。 |
| **补丁 5** | `track_forward_patch_strb` | VerifiedBoot.c:1904 | 从锚点正向追踪找到写入 `Data.IsUnlocked = AllowVerificationError` 的 STRB，将源寄存器替换为 WZR（= 0）。 | 强制 `KMRotAndBootState` 结构体中 `Data.IsUnlocked = 0`。此结构体传入 `KeyMasterSetRotAndBootState()`，用于构造 `SET_ROT`（0x201: `RotDigest = SHA256(PublicKey \|\| IsUnlocked)`）和 `SET_BOOT_STATE`（0x208），并通过 `ShareKeyMintInfoWithSPU()` 镜像到 SPU。如果 IsUnlocked=1 到达 KM/SPU，TEE 拒绝释放 FBE 数据加密密钥 → 用户数据不可访问。 |

**关于 BootState=GREEN 的说明**：补丁 3 的锚点定位了赋值 `Info->BootState` 的编译后 `if/else`。由于补丁 4 让 `AllowVerificationError = TRUE`，原始代码会赋值 `ORANGE`。正向追踪的 STRB sink 补丁（补丁 5）将 WZR 写入 BootState 存储位置，强制为 `GREEN`（= 0）。`Data.Color` 字段（VerifiedBoot.c:1903）读取 `Info->BootState`，因此内核 cmdline 颜色和 KM `SET_BOOT_STATE` 颜色都变为 GREEN。

### 重要但非关键 — 失败继续

| 编号 | 函数 | 源码行 | 补丁内容 | 失败影响 |
|------|------|--------|---------|---------|
| **补丁 2** | `patch_adrl_unlocked_to_locked` | avb_cmdline.c:327 | 找到加载 `"unlocked"` 和 `"locked"` 字符串的 ADRP+ADD 对（在 `"androidboot.vbmeta.device_state"` 附近），将 `"unlocked"` 指针重定向到 `"locked"`。 | 内核 cmdline 显示 `device_state=unlocked`。Android 框架检测到解锁状态。设备**仍可启动，数据可访问**，但 bootloader 状态暴露给应用层。部分完整性检测应用（银行、支付等）可能拒绝运行。 |

**设计说明**：补丁 2 在某些 ABL 版本上会失败（编译器内联或重排了 ADRP+ADD 对）。失败时 PatchBuffer 打印警告并继续 — 设备功能完整，只是 cmdline 泄露了真实解锁状态。

**歧义保护**：如果补丁 2 匹配到多于一处，PatchBuffer 直接中止（`return FALSE`）。预期是唯一匹配；多重匹配意味着误识别，可能破坏无关代码。

### 可选补丁 — 失败容忍

| 编号 | 函数 | 目标 | 补丁内容 | 失败影响 |
|------|------|------|---------|---------|
| **补丁 1** | `patch_abl_gbl` | ABL 二进制 | 将 UTF-16 `"efisp"` 替换为 `"nulls"`。 | 防止 ABL 递归加载 efisp。 |
| **补丁 6** | `patch_string_jump` | FastbootCmds.c | NOP 掉跳转到 `"is not allowed in Lock State"` 错误路径的条件分支（flash、erase、slot-change、snapshot-cancel 命令处理器）。 | 真锁状态下 fastboot 拒绝 flash/erase 命令。仅影响需要在假锁状态下重刷的救砖场景。 |
| **警告** | `patch_warning` | VerifiedBootMenu.c | 找到控制 "Orange State" / "Your device has been unlocked" 警告屏的 CBZ，改为 `CBZ WZR`（永远跳过）。 | 开机显示橙色警告文字。纯外观问题 — **在一加/OPPO 设备上**，当 BootState=GREEN 时 OEM 定制的警告检查无法自动绕过。存在逻辑差异 |

### 编译时补丁控制

每个补丁可通过预处理宏单独禁用：

```c
-DDISABLE_PATCH_1   // 跳过 efisp→nulls 重命名
-DDISABLE_PATCH_2   // 跳过 cmdline unlocked→locked
-DDISABLE_PATCH_3   // 跳过启动状态锚点改写
-DDISABLE_PATCH_4   // 跳过 AllowVerificationError MOV
-DDISABLE_PATCH_5   // 跳过 STRB sink（Data.IsUnlocked）
-DDISABLE_PATCH_6   // 跳过 fastboot 锁状态门 NOP
-DDISABLE_PRINT     // 禁止所有补丁输出
```

## 架构

```
                    ┌─────────────────────────────────┐
                    │         PatchBuffer()            │
                    │    （编排器，返回 BOOL）           │
                    └──┬──────┬──────┬──────┬──────┬──┘
                       │      │      │      │      │
              ┌────────┘  ┌───┘  ┌───┘  ┌───┘  ┌───┘
              ▼           ▼      ▼      ▼      ▼
        patch_abl_gbl  补丁2  补丁6  补丁3  patch_warning
        [可选]         [警告]  [可选]  [关键]     [可选]
                                         │
                                    ┌────┴────┐
                                    ▼         ▼
                                反向追踪    正向追踪
                               (补丁 4)   (补丁 5)
                               [关键]     [关键]

  ┌──────────────────────────────────────────────────────────┐
  │                  arm64_inst_decoder.h                     │
  │  20+ 种指令解码器、编码辅助函数、                           │
  │  PACIASP 边界检测、LocSet 数据流追踪器                     │
  └──────────────────────────────────────────────────────────┘
```


