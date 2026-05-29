/* Copyright (c) 2016, 2019-2020 The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * * Redistributions of source code must retain the above copyright
 *  notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following
 * disclaimer in the documentation and/or other materials provided
 *  with the distribution.
 *   * Neither the name of The Linux Foundation nor the names of its
 * contributors may be used to endorse or promote products derived
 * from this software without specific prior written permission.
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
 * Copyright (c) 2023, 2025, Qualcomm Innovation Center, Inc. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted (subject to the limitations in the
 *  disclaimer below) provided that the following conditions are met:
 *
 *      * Redistributions of source code must retain the above copyright
 *        notice, this list of conditions and the following disclaimer.
 *
 *      * Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials provided
 *        with the distribution.
 *
 *      * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *        contributors may be used to endorse or promote products derived
 *        from this software without specific prior written permission.
 *
 *  NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 *  GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 *  HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 *   WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 *  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 *  ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 *  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 *  GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 *  IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 *  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 *  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _BOOTLOADER_RECOVERY_H
#define _BOOTLOADER_RECOVERY_H

#include "LinuxLoaderLib.h"
#include <FastbootLib/FastbootCmds.h>

#define FFBM_MODE_BUF_SIZE 8

#define RECOVERY_BOOT_RECOVERY "boot-recovery"
#define RECOVERY_BOOT_FASTBOOT "boot-fastboot"
#define RECOVERY_FACTORY_DATA_RESET \
  "recovery\n--wipe_data\n--reason=MainClearConfirm"

/* Recovery Message */
struct RecoveryMessage {
  CHAR8 Command[32];
  CHAR8 Status[32];
  CHAR8 Recovery[768];
#ifdef AUTO_VIRT_ABL
  CHAR8 Stage[32];
  CHAR8 Reserved[1184]; // AB slot metadata
#endif
};

#define MISC_VIRTUAL_AB_MESSAGE_VERSION 2
#define MISC_VIRTUAL_AB_MAGIC_HEADER 0x56740AB0

/** MISC Partition usage as per AOSP implementation.
  * 0   - 2K     For bootloader_message
  * 2K  - 16K    Used by Vendor's bootloader (the 2K - 4K range may be
  *              optionally used as bootloader_message_ab struct)
  * 16K - 32K    Used by uncrypt and recovery to store wipe_package
  *              for A/B devices
  * 32K - 64K    System space, used for miscellanious AOSP features.
  **/
#define MISC_VIRTUALAB_OFFSET (32 * 1024)

static CHAR8 *VabSnapshotMergeStatus[] = {
  "none",
  "unknown",
  "snapshotted",
  "merging",
  "cancelled"
};

typedef enum UINT8 {
  NONE_MERGE_STATUS,
  UNKNOWN_MERGE_STATUS,
  SNAPSHOTTED,
  MERGING,
  CANCELLED
} VirtualAbMergeStatus;

typedef struct {
  UINT8 Version;
  UINT32 Magic;
  UINT8 MergeStatus;  // IBootControl 1.1, MergeStatus enum.
  UINT8 SourceStatus;   // Slot number when merge_status was written.
  UINT8 Reserved[57];
} __attribute__ ((packed)) MiscVirtualABMessage;

EFI_STATUS
RecoveryInit (BOOLEAN *BootIntoRecovery);
EFI_STATUS
GetFfbmCommand (CHAR8 *FfbmMode, UINT32 Sz);
EFI_STATUS
WriteRecoveryMessage (CHAR8 *Command);
VirtualAbMergeStatus
GetSnapshotMergeStatus (VOID);
EFI_STATUS
SetSnapshotMergeStatus (VirtualAbMergeStatus MergeStatus);
EFI_STATUS
DetectFDR (BOOLEAN *FDRDetected);
EFI_STATUS
ReadFromPartition (EFI_GUID *Ptype, VOID **Msg, UINT32 Size);
#endif
