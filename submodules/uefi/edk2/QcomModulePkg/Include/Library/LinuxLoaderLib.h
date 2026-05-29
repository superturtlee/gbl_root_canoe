/* Copyright (c) 2015-2018, 2020 The Linux Foundation. All rights reserved.
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
 * Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All rights reserved.
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

#ifndef __LINUXLOADERLIB_H__
#define __LINUXLOADERLIB_H__

#include <Uefi.h>

#include "DeviceInfo.h"
#include <Guid/FileInfo.h>
#include <Guid/FileSystemInfo.h>
#include <Guid/Gpt.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/CacheMaintenanceLib.h>
#include <Library/DebugLib.h>
#include <Library/Debug.h>
#include <Library/DevicePathLib.h>
#include <Library/IoLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/PrintLib.h>
#include <Library/TimerLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/Board.h>
#include <PiDxe.h>
#include <Protocol/BlockIo.h>
#include <Protocol/DevicePath.h>
#include <Protocol/EFIEraseBlock.h>
#include <Protocol/EFIMdtp.h>
#include <Protocol/EFIVerifiedBoot.h>
#include <Protocol/EFINandPartiGuid.h>
#include <Protocol/FirmwareVolume2.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/SerialIo.h>
#include <Protocol/SimpleFileSystem.h>
/**
  gQcomTokenSpaceGuid GUID definition.
 */
#define QCOM_TOKEN_SPACE_GUID                                                  \
  {                                                                            \
    0x882f8c2b, 0x9646, 0x435f,                                                \
    {                                                                          \
      0x8d, 0xe5, 0xf2, 0x08, 0xff, 0x80, 0xc1, 0xbd                           \
    }                                                                          \
  }

/* Selection attributes for selecting the BlkIo handles */
#define BLK_IO_SEL_MEDIA_TYPE_REMOVABLE 0x0001
#define BLK_IO_SEL_MEDIA_TYPE_NON_REMOVABLE 0x0002
#define BLK_IO_SEL_PARTITIONED_GPT 0x0004
#define BLK_IO_SEL_PARTITIONED_MBR 0x0008
#define BLK_IO_SEL_MATCH_PARTITION_TYPE_GUID 0x0010
#define BLK_IO_SEL_SELECT_MOUNTED_FILESYSTEM 0x0020
#define BLK_IO_SEL_SELECT_BY_VOLUME_NAME 0x0040

/* Select only the root device handle indicated. Doesn't return
 * any partitions within.
 * Currently this filter applies only for eMMC device, not the external
 * device connected via USB */
#define BLK_IO_SEL_SELECT_ROOT_DEVICE_ONLY 0x0080
/* Select the handle that's on the indicated root device.
 * Currently this filter applies only for eMMC device, not the external
 * device connected via USB */
#define BLK_IO_SEL_MATCH_ROOT_DEVICE 0x0100

/* Select through partition name*/
#define BLK_IO_SEL_MATCH_PARTITION_LABEL 0x0200

/* Do case insensetive string comparisons */
#define BLK_IO_SEL_STRING_CASE_INSENSITIVE 0x0400

/* Partitioning scheme types for selecting the BlkIo handles */
#define PARTITIONED_TYPE_MBR 0x01
#define PARTITIONED_TYPE_GPT 0x02

#define ZERO 0

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) sizeof (a) / sizeof (*a)
#endif

#define MAX_HANDLE_INFO_LIST 128

/* Macro to avoid integer overflow */
#define ADD_OF(a, b) (MAX_UINT32 - b > a) ? (a + b) : ZERO

#define BOOT_DEV_MAX_LEN 32
#define BOOT_DEV_NAME_SIZE_MAX 10

/* Macros to check the boot type */
#define EFI_EMMC_NETWORK_FLASH_TYPE 14
#define EFI_MMC_FLASH_TYPE 5
#define EFI_PCIE_FLASH_TYPE 7
#define EFI_MAX_FLASH_TYPE  0x7FFFFFFF

/* Allocate unsafe stack size of 128KB to address worst case,
 * which is same as the  normal stack size */
#define BOOT_LOADER_MAX_UNSAFE_STACK_SIZE (1 << 17)

/* Any data specific to additional attributes can be added here. */
typedef struct {
  EFI_GUID *RootDeviceType; /* GUID Selecting the root device type */
  EFI_GUID *PartitionType;  /* Partition Type to match */
  CHAR8 *VolumeName;        /* Mounted filesystem volume name to match */
  CHAR16 *PartitionLabel;   /* Partition label to match */
} PartiSelectFilter;

/* Output data providing more information about the device handle */
typedef struct {
  /* Handle that has BlkIO protocol installed, returned for all type of filters
   */
  EFI_HANDLE *Handle;

  /* Block IO protocol interface is returned for all type of filters */
  EFI_BLOCK_IO_PROTOCOL *BlkIo;

  /* This HDD dev path is returned only if Matching Partition type is requested
   * It should be noted that the contents of this memory should NOT be changed
   */
  const HARDDRIVE_DEVICE_PATH *PartitionInfo;
} HandleInfo;

/* Load image from partition to buffer */
EFI_STATUS
LoadImageFromPartition (VOID *ImageBuffer, UINT32 *ImageSize, CHAR16 *Pname);

EFI_STATUS
ReadWriteDeviceInfo (vb_device_state_op_t Mode, void *DevInfo, UINT32 Sz);

EFI_STATUS
GetNandMiscPartiGuid (EFI_GUID *Ptype);
/**
  Returns a list of BlkIo handles based on required criteria
  SelectionAttrib : Bitmask representing the conditions that need
                    to be met for the handles returned. Based on the
                    selections filter members should have valid values.
  FilterData      : Instance of Partition Select Filter structure that
                    needs extended data for certain type flags. For example
                    Partition type and/or Volume name can be specified.
  HandleInfoPtr   : Pointer Handle info where the information can be returned
  MaxBlkIopCnt    : On input, max number of handles the buffer can hold,
                    On output, the number of handles returned.

  @retval EFI_SUCCESS if the operation was successful
 */
EFI_STATUS
EFIAPI
GetBlkIOHandles (IN UINT32 SelectionAttrib,
                 IN PartiSelectFilter *FilterData,
                 OUT HandleInfo *HandleInfoPtr,
                 IN OUT UINT32 *MaxBlkIopCnt);

VOID
ToLower (CHAR8 *Str);
UINT64 GetTimerCountms (VOID);
EFI_STATUS
WriteToPartition (EFI_GUID *Ptype, VOID *Msg, UINT32 MsgSize);
BOOLEAN IsSecureBootEnabled (VOID);
EFI_STATUS ResetDeviceState (VOID);
EFI_STATUS
ErasePartition (EFI_BLOCK_IO_PROTOCOL *BlockIo, EFI_HANDLE *Handle);
EFI_STATUS
GetBootDevice (CHAR8 *BootDevBuf, UINT32 Len);
UINT32 GetBootDeviceType ();

/* Returns whether MDTP is active or not,
 * or whether it should be considered active for
 * bootloader flows. */
EFI_STATUS
IsMdtpActive (BOOLEAN *IsActive);

EFI_STATUS
WriteBlockToPartitionNoFlush (EFI_BLOCK_IO_PROTOCOL *BlockIo,
                   IN EFI_HANDLE *Handle,
                   IN UINT64 Offset,
                   IN UINT64 Size,
                   IN VOID *Image);


EFI_STATUS
WriteBlockToPartition (EFI_BLOCK_IO_PROTOCOL *BlockIo,
                   IN EFI_HANDLE *Handle,
                   IN UINT64 Offset,
                   IN UINT64 Size,
                   IN VOID *Image);

VOID StoreRootDeviceType (VOID);

#ifdef AUDIO_FRAMEWORK
STATIC inline CHAR8* GetAudioFw (VOID)
{
 return AUDIO_FRAMEWORK;
}
#else
STATIC inline CHAR8* GetAudioFw (VOID)
{
 return "\0";
}
#endif

#endif
