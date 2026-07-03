# ABL 修改细节

本文档记录当前 `patch_abl` 实际会对 ABL/EFI 做的修改，依据是 `submodules/patcher/src/patchs/` 中的实现。

## Patch 流程

PC 工具包会先从官方 ABL 镜像中提取最大的 EFI/PE payload，保存为 `ABL_original.efi`，然后执行：

```bash
patch_abl ABL_original.efi ABL.efi
```

`patch_abl` 会在内存中直接修改文件内容，再写出 `ABL.efi`。它不会改变文件大小，不会重建 EFI 分区结构，也不会重新签名。

## 已启用修改

| 区域 | 修改内容 | 目的 | 失败行为 |
|------|----------|------|----------|
| GBL 分区字符串 | 将 UTF-16LE `efisp` 替换为 UTF-16LE `nulls` | 避免漏洞路径继续使用原始 `efisp` 名称 | 非致命 warning：`Failed to patch ABL GBL` |
| `androidboot.vbmeta.device_state` | 将指向 `unlocked` 的 ADRP+ADD 改为指向 `locked` | 上报 `androidboot.vbmeta.device_state=locked` | 找不到时非致命；匹配到多处并修改时中止 |
| Boot-state 判断模式 | 在匹配到的 ARM64 指令模式中，将 `MOV W8, #1` 的立即数改成 `MOV W8, #0` | 将匹配到的 boot-state 分支逻辑推向 locked 行为 | 找不到该模式时中止 |
| Lock-state 源加载 | 将数据流追踪到的 `LDRB` 源指令替换为 `MOV Wn, #1` | 强制后续代码使用的寄存器来源值 | 找不到数据流链时非致命 warning |
| Lock-state 写回点 | 将 boot-state anchor 之后追踪到的第一个 `STRB` 写回点改为写入 `WZR` | 阻止派生出的 unlock 状态写回到被追踪的字节变量 | 找不到 sink 时非致命 warning |
| Orange State 警告 | 在确认同源 lock-state 变量后，将警告字符串附近的 `CBZ Wn` 改为 `CBZ WZR` | 抑制解锁开机警告文字 | 找不到时非致命 warning |

## GBL 字符串修改

patcher 会搜索以下 UTF-16LE 字节序列：

```text
```

然后将首次匹配替换为：

```text
n\0 u\0 l\0 l\0 s\0
```

新旧字符串长度一致，因此不需要调整文件布局。

## Device State 修改

patcher 会扫描 ARM64 代码，寻找连续的两组 ADRP+ADD 地址计算。它们需要分别指向 `unlocked` 和 `locked` 字符串，同时附近还必须存在指向 `androidboot.vbmeta.device_state` 的 ADRP+ADD。

找到后，第一组 ADRP+ADD 会被改写为指向 `locked`。这会改变被选择的字符串，但不会直接修改字符串内容。

如果没有找到这组三元匹配，patch 会继续执行。如果匹配并修改了多处，`patch_abl` 会中止，因为这通常说明匹配结果存在歧义。

## Boot State 与 Lock 变量修改

patcher 会搜索一个 32 字节的 ARM64 指令模式，用于定位 boot-state 相关逻辑。部分字节是通配符，因为不同 ABL 构建中的寄存器分配可能不同。

匹配成功后：

1. 从匹配到的条件分支指令中记录 lock-state 寄存器。
2. 将匹配模式中的 `MOV W8, #1` 编码改为 `MOV W8, #0`。
3. 从 anchor 位置向前追踪，找到从全局结构中加载 lock-state 字节的 `LDRB`。
4. 将这个源加载改为 `MOV Wn, #1`。
5. 从源加载位置向后追踪，找到相关的 `STRB` 写回点，并将写回寄存器改为 `WZR`，使其写入 0。

这些修改组合起来，会把 patched ABL 使用到的状态路径和写回值推向项目所需的 locked 行为。

## 警告文字抑制

对于 OPlus 风格的 ABL，patcher 会查找以下两个字符串引用：

```text
Orange State
Your device has been unlocked and can't be trusted
```

然后在同一函数中向前查找 `CBZ Wn` 分支。只有当反向数据流追踪确认 `Wn` 来自前面识别出的同一个 lock-state 全局变量时，才会修改这条分支。

修改方式是将其改为 `CBZ WZR`。由于 `WZR` 恒为 0，这会强制分支进入用于跳过或关闭警告路径的方向。

## 测试用补丁

代码中存在一个 fastboot 相关补丁，位于 `patchs/oplus/forceenablefastboot.c`。它会查找 `fastboot_unlock_verify error and reboot.`，并可以将附近的 `CBZ` / `CBZ X` 改为无条件跳转。

该补丁受 `ENABLE_TESTING_PATCHS` 控制，当前 Makefile 构建默认没有启用。

## 不会修改的内容

`patch_abl` 不会执行以下操作：

1. 不改变文件大小。
2. 不重建 PE/EFI section。
3. 不重新签名输出文件。
4. 不保证覆盖所有厂商的警告实现。
5. 当模式缺失或匹配存在歧义时，不保证继续生成可用结果。

运行工具包后请检查 `patch_log.txt`。如果出现 `Failed to patch ABL GBL`、`ADRL triple not found` 或 `patch_warning failed` 等提示，说明对应修改没有完整应用。
