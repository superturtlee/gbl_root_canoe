# Release Download & Flashing Guide

## 1. Download the Release

Current releases provide device-specific patched ABL files:

### Device-Specific Version

The filename includes your **device model**, **codename**, and **corresponding ABL version**. It contains the following files:

| Filename | Description |
|----------|-------------|
| `ABL.efi` | Patched ABL |
| `ABL_original.efi` | Original backup — **Do NOT flash** |


## 2. Select Operation Mode

| Mode | Applicable Scenario |
|------|---------------------|
| **Re-lock (True Re-lock)** | Devices with official unlock support (e.g., Twoplus), or international Dami devices that passed official unlock review |
| **Fake Re-lock** | Devices with force-unlocked bootloader, or devices requiring official fastboot as fallback |


## 3. Fake Re-lock Procedure

### 3.1 Currently in a **Locked + Rooted** State

Follow these steps in order:

1. Root the device via **KernelSU (KSU)**
2. Flash the `efisp` partition using the `dd` command
3. Use KSU to **persistently install the patch to the partition**
4. Enter **Super Fastboot** and perform the unlock operation
   - ✅ **Dami devices**: Data will **NOT be wiped** at this step
   - ⚠️ **OnePlus devices**: Must perform a **deep test unlock**; data will be lost, but Root access is retained

### 3.2 Currently in an **Unlocked** State

Follow these steps in order:

1. Obtain Root access normally
2. Enter **official fastboot** and flash the `efisp` partition
3. Reboot into Recovery and perform a format/wipe> ⚠️ The first reboot may crash — simply retry


## 4. True Re-lock Procedure

### 4.1 Currently in a **Locked** State

Follow **Section 3.1**, but **skip the unlock step**.

### 4.2 Currently in an **Unlocked** State

Follow **Section 3.2**, then additionally perform the **re-lock operation**.

> ✅ **It is recommended to use Super Fastboot for re-locking. Dami devices will not lose data during re-lock.**


## ⚠️ Important Warnings

> **Before performing any operation, make sure to verify the following:**

- 📌 Check whether any partition **other than those containing `boot`** has been modified — restore them if so
- 📌 Partitions verified by `init`: **AVB still enabled** must not be modified
- 📌 The `dtbo` partition verified by ABL **must not be modified** in true re-lock mode
- ❌ **Do NOT install TWRP** — doing so will result in **data corruption**
