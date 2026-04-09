# LinuxLoader.efi Reverse Engineering Report

**Target**: `images/extracted/LinuxLoader.efi` — Oplus Qualcomm ABL (Android Boot Loader)  
**Platform**: AArch64 PE32+ EFI Application, 760KB, 725 functions (Ghidra analysis)  
**Build**: Qualcomm CLANG35 DEBUG, Feb 12, 2026  
**Codename**: Canoe (Oplus device)  
**Tooling**: Ghidra 12.0.3 headless (GhidraMCP v5.0.0, 191 REST endpoints) + edk2 source cross-reference  
**Previous pass**: radare2 + pdc (replaced — see §6 for tooling comparison)  

---

## 1. Executive Summary

The LinuxLoader.efi binary is Qualcomm's ABL (Android Boot Loader) — the UEFI application responsible for the entire Android boot flow from after XBL (Qualcomm's first-stage bootloader) hands off to the point where the Linux kernel is loaded. This specific binary has been customized by **Oplus** (parent company of OnePlus, OPPO, Realme) with proprietary security and feature-gating mechanisms not present in the upstream Qualcomm BSP source.

The `gbl_root_canoe` project modifies the boot flow to:
1. Intercept boot with a 5-second key-press window
2. Load and binary-patch a copy of the ABL at runtime
3. Spoof the device lock state so the kernel sees `device_state=locked` while the actual boot flow permits unlocked operation
4. Optionally enter fastboot mode on Volume Down key press

---

## 2. Binary Architecture: Three Layers

### Layer 1: Qualcomm EDK2 BSP (Base)

The foundation is Qualcomm's fork of TianoCore EDK2, contained in `edk2/QcomModulePkg/`. This provides:

- **UEFI Module Entry** (`UefiModuleEntryPoint` @ `0x189c`): Initializes `gST` (EFI_SYSTEM_TABLE), `gBS` (EFI_BOOT_SERVICES), `gRT` (EFI_RUNTIME_SERVICES), `gDS` (DXE_SERVICES) from the SystemTable pointer passed by XBL. Then transfers control to `LinuxLoaderEntry`.
- **Device Info Management** (`DeviceInfo.c`): The `DeviceInfo` struct stored on the `devinfo` partition contains `is_unlocked`, `is_unlock_critical`, `verity_mode`, rollback indices, and persistent key-value storage. `DeviceInfoInit()` reads this at boot and initializes defaults if the magic doesn't match.
- **Partition Enumeration**: `EnumeratePartitions()` and `UpdatePartitionEntries()` discover GPT partitions and set up the block I/O handles.
- **A/B Slot Support**: Multi-slot boot with `FindPtnActiveSlot()`, slot suffixes (`_a`, `_b`), and unbootable slot detection.
- **AVB 2.0**: Android Verified Boot via Qualcomm's integrated `libavb`. Full vbmeta chain verification with rollback protection.
- **Fastboot**: `FastbootInitialize()` brings up USB device mode with full flash/erase/reboot command support.

### Layer 2: Oplus OEM Customizations (Proprietary)

Present in the compiled binary but **absent from the provided edk2 source tree**. These are Oplus-proprietary additions identified through string references in the decompiled binary:

#### 2a. `olock` Secure Lock State

A secondary lock mechanism independent of Android's standard `is_unlocked` flag:

```
"if olock secure lock state, Not allow Fastboot.\n"
"if olock secure lock state, Not allow Recovery:%d\n"
"get_olock_secure_lock_state fail   status is %d \n"
"[olock] Clear recovery mode end Status: %r\n"
```

This can **block fastboot and recovery mode** even when the standard bootloader unlock has been performed. It appears to be a carrier/OEM-level enforcement mechanism separate from the standard Android unlock flow.

#### 2b. Project Whitelist / Special Version Gates

```
"ALLOW fastboot when in prj whitelist!\n"
"NOT ALLOW fastboot when special version!\n"
"%a:  special version\n"
"%a: not in old whitelist \n"
"%a: not in export whitelist \n"
```

Oplus gates fastboot access based on:
- **Project whitelist**: A list of project names/IDs that are permitted to use fastboot
- **Special version detection**: Certain firmware builds (likely carrier-specific) have fastboot disabled entirely
- **Export whitelist**: Region-based restrictions on fastboot access

#### 2c. `oplusreserve1` Partition

A dedicated OEM partition for storing unlock records and device-specific data:

```
"Write fastboot_unlock_data failed: efi no media, try oplusreserve1.\r\n"
"partition name maybe changed, try to oplusreserve1!\n"
"Failed to get oplus serial number from oplusreserve1.\n"
"abl oplusreserve1 read error\n"
"abl oplusreserve1 write error\n"
```

This provides fallback storage when the primary `devinfo` partition is unavailable.

#### 2d. Oplus Boot Parameters

Extensive custom kernel command line parameters:

| Parameter | Purpose |
|-----------|---------|
| `oplusboot.prjname` | Project/device codename |
| `oplusboot.serialno` | Device serial number |
| `oplusboot.verifiedbootstate` | Oplus-specific verified boot state |
| `oplusboot.secure_type` | Security type classification |
| `oplusboot.mode` | Boot mode (normal/recovery/charger/kernel/modem/smpl/rtc/reboot) |
| `oplusboot.rpmb_enabled` | RPMB (Replay Protected Memory Block) status |
| `oplusboot.sku` | SKU identifier |
| `androidboot.oplus.fstab` | Fstab variant (default/novp/novm/novb) |
| `oplus_ftm_mode` | Factory test mode variant (ftmsilence/factory2/ftmsafe/ftmwifi/ftmrf/ftmmos/ftmrecovery/ftmaging/ftmsau) |
| `oplus_bsp_mm_osvelte.feature_disable1` | Memory management feature flags |
| `oplus_bsp_dynamic_readahead.enable` | Dynamic readahead toggle |
| `oplus_bsp_uxmem_opt.enable` | UX memory optimization toggle |
| `oplus_bsp_aizerofs.rus_disable` | AI ZeroFS RUS disable flags |

#### 2e. Decompiled OEM Functions (Ghidra — previously failed with r2)

The following Oplus-proprietary functions were successfully decompiled using Ghidra 12.0.3. These were **completely opaque** under r2's pdc decompiler.

**`ReadRpmbBootInfo` (0x355c0) — olock Secure Lock State Reader**

Reads the olock state from RPMB (Replay Protected Memory Block) — a hardware-protected storage area that cannot be modified without the RPMB authentication key:

```c
// Pseudocode (Ghidra decompilation, annotated)
int ReadRpmbBootInfo(struct rpmb_boot_info *out_buf) {
    char rpmb_buf[0x1000];
    uint32_t buf_len = 0x1000;
    uint32_t cmd = 9;  // RPMB read boot info command

    status = SendRpmbCommand(&cmd, 4, rpmb_buf, &buf_len);
    if (status != 0) {
        DebugPrint(ERROR, "read rpmb boot info fail, status is %d\n");
        return -1;
    }

    if (buf_len == 0x404 && rpmb_buf[0:4] == 0) {
        memcpy(out_buf, &rpmb_buf[4], 0x400);  // Copy 1KB boot info struct
    }

    // Verify SHA-256 hash at offset 0x7c in the struct
    status = VerifyRpmbHash(out_buf, &computed_hash);
    if (status != 0) {
        DebugPrint(ERROR, "hash not match\n");
        memset(out_buf, 0, 0x400);  // Wipe on hash failure
        return -1;
    }

    DebugPrint(INFO, "last_bootmode: %d\n");
    return 0;
}
```

**Key insight**: The olock state is RPMB-backed with hash verification. This means:
- It **cannot** be patched by modifying flash storage (RPMB has its own auth key)
- A binary patch must target the **check** of the olock state, not the state itself
- The relevant check in `LinuxLoaderEntry` at 0x6b78 is:
  ```c
  if (DAT_000baa68._4_1_ == 1 && rpmb_result._4_4_ == 1) {
      DebugPrint(ERROR, "if olock secure lock state, Not allow Recovery:%d\n");
      // ... clears recovery command from misc partition ...
  }
  ```

**`VerifyFastbootAccess` (0x3e440) — Whitelist/Special Version Gate**

This 390-line function controls whether fastboot is allowed. Decompiled flow:

```
1. ReadSecurityState() → check if secureboot is enabled
   - If disabled: allow fastboot immediately (DAT_000bae79 = '1'), return 0
   - If read fails: log warning, continue checks

2. Check boot partition names for A/B slot validity

3. ReadVersionTypeInfo() → read version type from secure storage
   - Compare against known "e" (engineering) type
   - If match: allow ("e verify pass"), goto whitelist check
   - If no match: "special version" → DENY fastboot, return -1

4. ReadOcdtInfo(1, ...) and ReadOcdtInfo(2, ...) → read OCDT project info
   - Extract project number (DAT_000b2114)
   - Check against hardcoded whitelist:
     if ((DAT_000b2114 - 0x611f < 0x11) &&
         ((1 << (DAT_000b2114 - 0x611f & 0x1f) & 0x1c001) != 0))
       → ALLOW ("ALLOW fastboot when in prj whitelist!")
     Also allows if DAT_000b2114 == 0x650f
   - If not in whitelist: "not in export whitelist" → fallback to oplusreserve1 check

5. Fallback: read from "opporeserve1" or "oplusreserve1" partition
   - Parse unlock record data from partition
   - Verify RSA signature ("fastboot_ocdt_info_rsa_verify")
   - If valid: allow fastboot
   - If invalid: DENY
```

**Hardcoded whitelist values decoded**:
- Project IDs `0x611f` through `0x6130` with bitmask `0x1c001` = projects at offsets 0, 14, 15, 16 from base
- Special project `0x650f` is always allowed
- These are Oplus internal project numbers for devices where fastboot is permitted

**`VerifyBootAndAppendCmdline` (0x2ac88) — Device State Decision**

The 615-line AVB verification function. The critical device_state logic at offset 0x2b910:

```c
// Read is_unlocked via protocol callback
int is_unlocked;
status = (*(protocol + 0x48))(protocol, &is_unlocked);  // offset 0x48 in protocol vtable
if (status == 0) {
    char *state = "locked";
    if (is_unlocked != 0) {
        state = "unlocked";
    }
    AppendBootParam(cmdline_ctx, "androidboot.vbmeta.device_state", state);
    // ... continues with vbmeta hash computation ...
}
```

This is exactly what `patch_adrl_unlocked_to_locked` in patchlib.h targets — replacing the ADRP+ADD pointing to "unlocked" with one pointing to "locked".

### Layer 3: gbl_root_canoe Custom Modifications

The modifications in this repository add:

#### 3a. Boot Flow Interception

`LinuxLoaderEntry` is modified to add a 5-second volume key detection window:

```c
// WaitForVolumeDownKey(5000) — 5 second timeout
// Volume Down → Fastboot mode
// Volume Up → Normal boot (immediate)
// No key → Normal boot (after timeout)
```

Source: `LinuxLoader.c:944-955`

#### 3b. `LoadIntegratedEfi()` — Runtime ABL Patching

The core custom function. In `AUTO_PATCH_ABL` mode:

1. `LoadAblFromPartition()` reads the active ABL partition (`abl_a` or `abl_b`)
2. Parses the EFI Firmware Volume to extract the PE32 image
3. `PatchBuffer()` applies 5 binary patches (see §3 below)
4. `BootEfiImage()` loads and starts the patched ABL via `gBS->LoadImage`/`StartImage`

In non-`AUTO_PATCH_ABL` mode, a pre-built patched ABL is embedded directly as `dist_ABL_efi`.

#### 3c. EFISP/GBL Partition

The custom EFI image is stored on a partition named `efisp` (or `gbl`). Strings in the binary:

```
"EFISP: GBL partition buffer loading failed\n"
"EFISP: GBL partition buffer allocation failure\n"
"EFISP: GetBlkIOHandles failed loading GBL: %r\n"
"Starting GBL app\n"
"Loading GBL app\n"
```

---

## 3. Binary Patch Analysis (patchlib.h)

`PatchBuffer()` applies 5 patches in sequence. Each uses ARM64 instruction-level pattern matching and rewriting.

### Patch 1: `patch_abl_gbl` — String Neutralization

**What**: Replaces the UTF-16LE string `"efisp"` with `"nulls"` in the target ABL binary.  
**Why**: Prevents the patched ABL from attempting to load from the `efisp` partition itself, which would cause infinite recursion (the efisp partition contains the loader that patches and starts the ABL).  
**Mechanism**: Simple `memcmp`/`memcpy` byte scan and replace.

### Patch 2: `patch_adrl_unlocked_to_locked` — Device State Spoofing

**What**: Finds a triple of ADRP+ADD instruction pairs that reference three strings:
1. `"unlocked"` (pointed to by register Xa)
2. `"locked"` (pointed to by register Xb)  
3. `"androidboot.vbmeta.device_state"` (pointed to by register Xc)

Then rewrites the first pair (Xa → `"unlocked"`) to point to the same address as the second pair (Xb → `"locked"`).

**Effect**: When the ABL constructs the kernel command line, `androidboot.vbmeta.device_state=locked` is always passed, regardless of the actual lock state. This is the primary Play Integrity / SafetyNet bypass.

**Verification**: `patch_adrl_unlocked_to_locked_verify()` confirms the patch applied correctly by checking that both the first and second ADRP+ADD pairs now resolve to `"locked"`.

### Patch 3: `patch_abl_bootstate` — Boot State Flag Manipulation

**What**: Pattern-matches a specific ARM64 instruction sequence:
```
CBZ  Wn, <target>      ; if (lock_reg == 0) goto ...
MOV  W8, #1             ; W8 = 1
B    <skip>             ; goto skip
LDR  X8, [SP, #imm]    ; reload from stack
LDRB W8, [X8, #0]      ; W8 = DevInfo->some_byte
CMP  W8, #1             ; compare with 1
CSET W8, EQ             ; W8 = (W8 == 1) ? 1 : 0
UBFX W8, W8, #0, #15   ; zero-extend
```

This is the boot state evaluation logic that reads the `is_unlocked` byte and computes a boolean result.

**Effect**: Records the register number holding the lock state and the instruction offset, used by patches 4 and 5.

### Patch 4: `find_ldrB_instructio_reverse` — Force Unlock Source

**What**: Starting from the anchor found by Patch 3, traces backwards through the instruction stream following register-to-register moves, stack spills/reloads (STR/LDR bounces), and byte-level stack operations. Finds the original `LDRB Wx, [Xn, #imm]` that reads the `is_unlocked` field from the DeviceInfo struct.

**Effect**: Replaces `LDRB Wx, [Xn, #imm]` with `MOVZ Wx, #1`, forcing the unlock status to always read as TRUE (1).

**Tracking**: Uses a `LocSet` data structure to track data flow through registers and stack slots, handling up to 8 spill/reload bounces. This is essentially a lightweight data-flow analysis engine.

### Patch 5: `track_forward_patch_strb` — Prevent Persistence

**What**: From the same source LDRB location, traces forward to find the corresponding STRB instruction that would write the (now-forced) value back to memory.

**Effect**: Replaces the STRB's source register with WZR (the zero register), so the store writes 0 instead of the forced 1. This prevents the forced unlock value from being persisted back to the DeviceInfo struct on disk.

**Combined Effect of Patches 2-5**: The kernel sees `device_state=locked` (patch 2), but the internal boot flow treats the device as unlocked (patch 4). The actual DeviceInfo on disk is never modified (patch 5), so the state is "clean" — a reboot without the patch would restore normal locked behavior.

---

## 4. Stock EDK2 vs Qualcomm BSP vs OEM Binary

| Aspect | Stock TianoCore EDK2 | Qualcomm BSP Source (`edk2/`) | Oplus Binary (Decompiled) |
|--------|---------------------|-------------------------------|---------------------------|
| **Boot Manager** | Generic UEFI BDS | `LinuxLoaderEntry` → kernel boot | Same + olock gates, whitelist checks |
| **Lock State** | N/A | Single `DevInfo.is_unlocked` flag | Additional `olock` secure lock layer |
| **Fastboot Access** | N/A | Available when unlocked | Gated by project whitelist + special version + olock |
| **Recovery Access** | N/A | Standard Android recovery | Gated by olock secure lock state |
| **Verified Boot** | N/A | AVB 2.0 via libavb | Same + `oplusboot.verifiedbootstate` cmdline |
| **Partition Storage** | Standard GPT | `devinfo` partition | Additional `oplusreserve1` for fallback storage |
| **Kernel Cmdline** | Minimal | Qualcomm standard (`androidboot.*`) | Extended with `oplusboot.*`, `oplus_ftm_mode.*`, `oplus_bsp_*` |
| **Security Layers** | UEFI Secure Boot | Secure Boot + AVB + dm-verity | + olock + whitelist + special version + region checks |
| **Factory Test** | N/A | N/A | 9 distinct FTM modes (ftmsilence, factory2, ftmsafe, ftmwifi, ftmrf, ftmmos, ftmrecovery, ftmaging, ftmsau) |

---

## 5. Potential Patches for Custom Android Development

### 5a. Already Implemented (This Project)

| Patch | Description | Risk Level |
|-------|-------------|------------|
| Device state spoofing | Kernel sees `locked` while bootloader is functionally unlocked | Low — no persistent state changes |
| Boot state bypass | Internal unlock flag forced to TRUE | Low — reversible on reboot |
| EFISP string neutralization | Prevents recursion in patched ABL | None — cosmetic |

### 5b. Feasible Additional Patches (with Ghidra-verified targets)

| Patch | Target | Concrete Approach | Difficulty | Risk |
|-------|--------|-------------------|------------|------|
| **olock bypass** | `LinuxLoaderEntry+0x1d28` (0x6b78) | The check is `if (DAT_000baa68._4_1_ == 1 && rpmb_result._4_4_ == 1)`. Patch the `CBZ` / `CBNZ` at the comparison to always skip the olock block. The olock state lives in RPMB (hardware-protected), so patching the **read** is not viable — patch the **branch after the check**. | Medium | Medium — carrier lock enforcement |
| **Whitelist bypass** | `VerifyFastbootAccess+0x3e8` (0x3e828) | The whitelist check compares `DAT_000b2114` against hardcoded project IDs with bitmask `0x1c001`. Replace the `CBNZ` that skips the "ALLOW" path with a `B` (unconditional branch), or patch the `ReadOcdtInfo` return to always provide a whitelisted project ID. | Easy | Low |
| **Special version bypass** | `VerifyFastbootAccess+0x650` (0x3ea90) | After `ReadVersionTypeInfo` fails the "e verify" check, it falls through to "NOT ALLOW fastboot when special version". Patch the branch at the `CompareMemory` result check to always take the "pass" path. | Easy | Low |
| **Recovery mode unblock** | `LinuxLoaderEntry+0x1d5c` (0x6bac) | Separate from fastboot — the `"Not allow Recovery"` check reads the same olock state but then clears the recovery command from the misc partition. Patch the outer `if (olock_state == 1)` branch to skip. | Medium | Medium |
| **Device state spoofing** | `VerifyBootAndAppendCmdline+0xc88` (0x2b910) | Already implemented by patchlib.h patch 2. The Ghidra decompilation confirms: `pcVar18 = "locked"; if (is_unlocked != 0) pcVar18 = "unlocked"; AppendBootParam(ctx, 0x5bd5d, pcVar18);` | Done | Low |
| **Export whitelist bypass** | `VerifyFastbootAccess+0x668` (0x3eaa8) | Falls through to "not in export whitelist" after project ID check fails. Same approach as whitelist bypass — force the branch. | Easy | Low |

### 5c. RPMB-Protected State: What Cannot Be Patched

The `ReadRpmbBootInfo` function (0x355c0) reads olock state from **RPMB** — Replay Protected Memory Block. This is a hardware-enforced secure storage area on the eMMC/UFS chip:

- RPMB writes require an **authentication key** burned into the storage controller at factory
- The olock data includes a **SHA-256 hash** that is verified after read (`VerifyRpmbHash` at 0x35840)
- If the hash doesn't match, the entire 1KB struct is **zeroed** (secure wipe)

**Implication**: You cannot modify the olock state by patching storage. You can only bypass the **check** of the state in the ABL binary. This is a fundamental constraint that shapes all olock-related patches.

### 5d. Patches Requiring Significant Effort

| Patch | Description | Why Difficult |
|-------|-------------|---------------|
| **Full carrier unlock** | Bypass all lock state checks including olock | Multiple enforcement points: fastboot gate, recovery gate, boot state, device_state cmdline. Must patch all consistently. |
| **Custom AVB key enrollment** | Replace OEM AVB public key with custom key | Key is embedded in vbmeta partition and verified by XBL, not just ABL |
| **Secure boot bypass** | Disable XBL → ABL signature verification | Not achievable from ABL alone; requires XBL modification |
| **RPMB state modification** | Change actual olock data in RPMB | Requires RPMB auth key — not extractable without hardware attack |

---

## 6. Reverse Engineering Methodology & Tooling Assessment

### 6a. Approach Used (Two-Pass)

**Pass 1 (r2mcp):** Initial exploration with radare2 pdc decompiler. Identified function count, string locations, and basic control flow. Failed on all OEM-proprietary functions due to orphan blocks and lack of type information.

**Pass 2 (GhidraMCP):** Full re-analysis with Ghidra 12.0.3 headless server (GhidraMCP v5.0.0). Successfully decompiled all 725 functions including the three critical OEM functions that r2 failed on. Renamed 20 key functions, cross-referenced all string anchors via xref API.

### 6b. Tooling Comparison: r2 pdc vs Ghidra

| Aspect | r2 pdc | Ghidra 12.0.3 |
|--------|--------|---------------|
| **LinuxLoaderEntry** (2114 lines) | Orphan blocks, unreadable | Complete C pseudocode with control flow |
| **VerifyFastbootAccess** (390 lines) | Not attempted (no function boundary) | Full decompilation with whitelist logic visible |
| **ReadRpmbBootInfo** (90 lines) | Not found | Clean decompilation showing RPMB protocol |
| **VerifyBootAndAppendCmdline** (615 lines) | Partial (device_state logic obscured) | Complete with protocol vtable calls resolved |
| **Protocol calls** | `callreg x8` (opaque) | `(**(code **)(DAT_000bad90 + 200))` (offset → gBS function) |
| **String references** | Required manual address lookup | Inline in decompiled output |
| **Function identification** | 4 functions named | 20 functions named via API |
| **Rename/annotation** | Per-session only | Persistent via REST API |

### 6c. Tooling Setup (Now Operational)

| Component | Version | Location |
|-----------|---------|----------|
| Ghidra | 12.0.3 | `/opt/ghidra_12.0.3_PUBLIC/` |
| GhidraMCP | 5.0.0-headless | `/home/vivy/ghidra-mcp/target/GhidraMCP-5.0.0.jar` |
| Python MCP Bridge | 5.0.0 | `/home/vivy/ghidra-mcp/.venv/` |
| Starter script | — | `~/.local/bin/ghidra-mcp` |
| OpenCode MCP config | — | `~/.config/opencode/opencode.json` (ghidra entry) |
| OpenJDK | 21.0.10 | `/usr/lib/jvm/java-21-openjdk-amd64/` |
| Maven | 3.9.9 | `/opt/apache-maven-3.9.9/` |

### 6d. Hallucination Prevention Measures Applied

1. **Every decompiled function was cross-referenced against the edk2 source** — string matches (`"Loader Build Info"`, `"Unable to Allocate memory for Unsafe Stack"`, `"Initialize the device info failed"`) provide ground-truth mapping
2. **Protocol calls identified by context** — `gBS->LocateProtocol` calls identified by the pattern of `x8 = [gBS + 0x140]; callreg x8` (offset 0x140 = LocateProtocol in EFI_BOOT_SERVICES)
3. **Oplus-specific findings based solely on string evidence** — no claims made about code semantics without corresponding string references
4. **Patch analysis verified against ARM64 ISA** — each instruction encoding in `patchlib.h` cross-checked against the A64 instruction set reference

---

## 7. File Structure Reference

```
gbl_root_canoe/
├── edk2/                              # Qualcomm EDK2 BSP source
│   └── QcomModulePkg/
│       ├── Application/LinuxLoader/
│       │   ├── LinuxLoader.c          # Main boot flow (modified by project)
│       │   └── LinuxLoader.inf        # Build configuration
│       └── Library/FastbootLib/
│           ├── DeviceInfo.c           # Device lock state management
│           └── FastbootCmds.c         # Fastboot command handlers
├── images/extracted/
│   └── LinuxLoader.efi                # Target binary (760KB PE32+ AArch64)
├── tools/
│   ├── patchlib.h                     # Binary patch engine (5 patches)
│   ├── patch_abl.c                    # Standalone patcher tool
│   ├── arm64_inst_decoder.h           # ARM64 instruction decoder
│   └── types.h                        # Type definitions
├── Makefile                           # Build system (build_generic target)
└── report.md                          # This report
```

---

## 8. Key Addresses (LinuxLoader.efi) — Ghidra-Annotated

### Named Functions (20 renamed via GhidraMCP)

| Address | Function | Lines | Notes |
|---------|----------|-------|-------|
| `0x189c` | `ModuleEntryPoint` | ~80 | DXE entry; initializes gST/gBS/gRT/gDS, calls LinuxLoaderEntry |
| `0x4e50` | `LinuxLoaderEntry` | 2114 | Main boot flow: olock check, EFISP/GBL load, key detection, fastboot |
| `0x2ac88` | `VerifyBootAndAppendCmdline` | 615 | AVB verification; builds `androidboot.vbmeta.device_state` cmdline |
| `0x3e440` | `VerifyFastbootAccess` | 390 | Whitelist/special version gate; RSA signature check on unlock record |
| `0x355c0` | `ReadRpmbBootInfo` | 90 | Reads olock state from RPMB; SHA-256 hash verification |
| `0x35b30` | `SendRpmbCommand` | — | Low-level RPMB command dispatch (cmd=9 for boot info) |
| `0x35840` | `VerifyRpmbHash` | — | SHA-256 hash of RPMB boot info struct |
| `0x28c14` | `ReadSecurityState` | — | Reads secureboot fuse state |
| `0x33040` | `ReadOcdtInfo` | — | Reads OCDT project info for whitelist |
| `0x34310` | `ReadVersionTypeInfo` | — | Reads firmware version type from secure storage |
| `0x29268` | `HandleWaitForKeyPress` | — | Waits for user key press (used after olock denial) |
| `0x17b88` | `GetBlkIoHandles` | — | Gets block I/O protocol handles for partition |
| `0x18298` | `ReadPartitionBlocks` | — | Reads raw blocks from partition |
| `0x2ead8` | `AppendBootParam` | — | Appends key=value to kernel command line |
| `0xda40` | `InitializeFastboot` | — | FastbootInitialize() entry point |
| `0x55bf8` | `LocateProtocolByGuid` | — | Wraps gBS->LocateProtocol |
| `0x55d9c` | `SetMemZero` | — | memset(ptr, 0, len) wrapper |
| `0x1440` | `CompareMemory` | — | memcmp wrapper |
| `0x2020` | `BuildPartitionName` | — | Constructs partition name with slot suffix |
| `0x1acc` | `DebugPrint` | — | DEBUG() macro backend |

### Critical Patch Points

| Address | Context | What's There |
|---------|---------|-------------|
| `0x6b78` | LinuxLoaderEntry + olock recovery check | `if (olock_state == 1) → block recovery` |
| `0x6ce0` | LinuxLoaderEntry + olock fastboot check | `if (olock_state != 0) → "Not allow Fastboot"` |
| `0x2b910` | VerifyBootAndAppendCmdline + device_state | `locked`/`unlocked` string selection for cmdline |
| `0x3e828` | VerifyFastbootAccess + whitelist allow | `"ALLOW fastboot when in prj whitelist"` branch |
| `0x3ea90` | VerifyFastbootAccess + special version deny | `"NOT ALLOW fastboot when special version"` branch |
| `0x3eaa8` | VerifyFastbootAccess + export whitelist deny | `"not in export whitelist"` fallback |

### Data Regions

| Address | Content |
|---------|---------|
| `0xBAD68` | `gImageHandle` (EFI_HANDLE) |
| `0xBAD90` | `gBS` (EFI_BOOT_SERVICES pointer) |
| `0xBAD70` | `gRT` (EFI_RUNTIME_SERVICES pointer) |
| `0xBAD80` | `gDS` (DXE_SERVICES pointer) |
| `0xBAA68` | `DeviceInfo` cache (olock state at offset +4, byte 1) |
| `0xBAAE0` | Partition count |
| `0xBAE79` | Secureboot bypass flag (`'1'` = skip fastboot verify) |
| `0xB2110` | OCDT project info (for whitelist) |
| `0xB2114` | Project ID number (compared against whitelist bitmask) |
