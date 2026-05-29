/*
 * Copyright (c) 2011,2014-2015,2021 The Linux Foundation. All rights
 * reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 *  * Neither the name of The Linux Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/*
 * Changes from Qualcomm Innovation Center are provided under the following license:
 *
 * Copyright (c) 2022 - 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the
 * disclaimer below) provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *
 *     * Neither the name of Qualcomm Innovation Center, Inc. nor the names of
 *       its contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 * GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 * HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef _DEVINFO_H_
#define _DEVINFO_H_

#include <Protocol/EFIVerifiedBoot.h>
#define DEVICE_MAGIC "ANDROID-BOOT!"
#define DEVICE_MAGIC_SIZE 13
#define MAX_VERSION_LEN 64
#define MAX_VB_PARTITIONS 32
#define MAX_USER_KEY_SIZE 2048
#define MAX_NAME_SIZE      56
#define MAX_VALUE_SIZE     32
#define MAX_ENTRY_SIZE     8
#define MAX_AUDIO_FW_LENGTH 16
#define DICE_HIDDEN_SIZE 32

#ifdef HIBERNATION_SWAP_PARTITION_NAME
#define SWAP_PARTITION_NAME     HIBERNATION_SWAP_PARTITION_NAME
#else
#define SWAP_PARTITION_NAME     L"swap_a"
#endif

enum unlock_type {
  UNLOCK = 0,
  UNLOCK_CRITICAL,
};

typedef struct {
  UINT16  in_use;
  UINT16  name_size;
  UINT16  value_size;
  UINT8   name[MAX_NAME_SIZE];
  UINT8   value[MAX_VALUE_SIZE];
} persistent_value_type;

typedef struct device_info {
  CHAR8 magic[DEVICE_MAGIC_SIZE];
#ifdef AUTO_VIRT_ABL
  BOOLEAN IsResetDeviceState;
  CHAR8 Type; // 0 = UNLOCK, 1 = UNLOCK_CRITICAL
#endif
  BOOLEAN is_unlocked;
  BOOLEAN is_unlock_critical;
  BOOLEAN is_charger_screen_enabled;
  CHAR8 bootloader_version[MAX_VERSION_LEN];
  CHAR8 radio_version[MAX_VERSION_LEN];
  BOOLEAN verity_mode; // TRUE = enforcing, FALSE = logging
  UINT32 user_public_key_length;
  CHAR8 user_public_key[MAX_USER_KEY_SIZE];
  UINT64 rollback_index[MAX_VB_PARTITIONS];
  persistent_value_type  persistent_value[MAX_ENTRY_SIZE];
  UINTN GoldenSnapshot;
  CHAR8 AudioFramework[MAX_AUDIO_FW_LENGTH];
  UINT8 FdrFlag;
  UINT32 FrsSecLen; /*Holds length of FRS secret*/
  UINT8 FrsSec[DICE_HIDDEN_SIZE]; /*Holds plain secret*/
} DeviceInfo;

struct verified_boot_verity_mode {
  BOOLEAN verity_mode_enforcing;
  CHAR8 *name;
};

struct verified_boot_state_name {
  boot_state_t boot_state;
  CHAR8 *name;
};

BOOLEAN IsSnapshotGolden (VOID);
BOOLEAN IsUnlocked (VOID);
BOOLEAN IsUnlockCritical (VOID);
BOOLEAN IsEnforcing (VOID);
BOOLEAN IsChargingScreenEnable (VOID);
VOID GetBootloaderVersion (CHAR8 *BootloaderVersion, UINT32 Len);
VOID GetRadioVersion (CHAR8 *RadioVersion, UINT32 Len);
EFI_STATUS EnableChargingScreen (BOOLEAN IsEnabled);
EFI_STATUS EnableEnforcingMode (BOOLEAN IsEnabled);
EFI_STATUS SetDeviceUnlockValue (UINT32 Type, BOOLEAN State);
EFI_STATUS DeviceInfoInit (VOID);
EFI_STATUS ReadRollbackIndex (UINT32 Loc, UINT64 *RollbackIndex);
EFI_STATUS WriteRollbackIndex (UINT32 Loc, UINT64 RollbackIndex);
EFI_STATUS StoreUserKey (CHAR8 *UserKey, UINT32 UserKeySize);
EFI_STATUS GetUserKey (CHAR8 **UserKey, UINT32 *UserKeySize);
EFI_STATUS EraseUserKey (VOID);
EFI_STATUS ReadPersistentValue (CONST UINT8 *Name, UINTN NameSize,
                                UINT8 *Value, UINTN *ValueSize);
EFI_STATUS WritePersistentValue (CONST UINT8 *Name, UINTN NameSize,
                                 CONST UINT8 *Value, UINTN ValueSize);

EFI_STATUS
SetSnapshotGolden (UINTN Val);
EFI_STATUS
StoreAudioFrameWork (CONST CHAR8 *CmdLine, UINT32 CmdLineLen);
EFI_STATUS
ReadAudioFrameWork (CHAR8 **CmdLine, UINT32 *CmdLineLen);
EFI_STATUS
SetFDRFlag (VOID);
#endif
