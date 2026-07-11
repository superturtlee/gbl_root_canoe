#!/system/bin/sh
# ARB eFuse write suppression.
#
# ColorOS 16.0.8+ android.hardware.boot HAL writes "1" to this sysfs node
# during boot, which causes qcom-dload-mode.ko to invoke
# qcom_scm_update_rollback_version() → SMC 0x01/0x1E → TrustZone → QFPROM.
# Removing write access before `class_start hal` prevents the blow.
#
# This script runs at post-fs-data, before any HAL service starts.
# qcom-dload-mode.ko is loaded in first-stage init (vendor_boot ramdisk),
# so the sysfs node already exists by the time we run.

ARB_SYSFS="/sys/kernel/dload/update_arb"

if [ -f "$ARB_SYSFS" ]; then
    chmod 000 "$ARB_SYSFS" 2>/dev/null
    echo "[arb_guard] blocked $ARB_SYSFS" > /dev/kmsg
else
    # Node absent — module not loaded or path changed. Log for debugging.
    echo "[arb_guard] WARNING: $ARB_SYSFS not found" > /dev/kmsg
fi
