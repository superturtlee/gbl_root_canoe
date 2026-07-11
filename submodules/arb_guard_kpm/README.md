# arb_guard KPM

KernelSU Kernel Patch Module that suppresses the ColorOS ARB (Anti-Rollback) eFuse write.

## Background

ColorOS 16.0.8+ ships `qcom-dload-mode.ko` (vendor_boot ramdisk) which exposes
`/sys/kernel/dload/update_arb`. During boot, the Boot HAL writes `1` to this node,
causing `qcom_scm_update_rollback_version()` (from `qcom-scm.ko`) to issue an SMC
call (svc=0x01, cmd=0x1E) into TrustZone, which irreversibly burns QFPROM ARB eFuses.

## This KPM

Hooks `qcom_scm_update_rollback_version` at runtime via KPatch-Next and returns 0
immediately, preventing the SMC call and eFuse burn.

## Build Requirements

- OKI kernel built with `kpm_enable=true` (KPatch-Next applied to `Image`)
- KPatch-Next headers (`kpm_utils.h`) in include path
- `aarch64-linux-android` clang toolchain

```sh
# Build using the OKI workflow's clang toolchain
clang --target=aarch64-linux-android \
  -I<kpatch_next_headers> \
  -shared -fPIC -o arb_guard.kpm arb_guard.c
```

## Deployment

Place `arb_guard.kpm` in a KernelSU module:
```
/data/adb/modules/<module>/
    module.prop
    post-fs-data.sh   ← run: ksud kpm load /path/to/arb_guard.kpm
```

## Layered Defense

This KPM is the second layer. The first layer is `post-fs-data.sh` in the
`fake_bl_efisp` Magisk module, which `chmod 000`s the sysfs node before the
Boot HAL starts. The KPM is required if the sysfs approach is bypassed (e.g.,
if the HAL runs as a trusted process that ignores DAC).
