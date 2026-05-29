/* Copyright (c) 2015-2018, 2020-2021, The Linux Foundation. All rights
 * reserved.
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
 *
 * Changes from Qualcomm Innovation Center are provided under the following license:
 *
 * Copyright (c) 2022, 2024 Qualcomm Innovation Center, Inc. All rights reserved.
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

#ifndef __BOARD_H__
#define __BOARD_H__

#include <Uefi.h>
#include <Library/DebugLib.h>
#include <Library/Debug.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Protocol/EFIChipInfo.h>
#include <Protocol/EFIPlatformInfo.h>
#include <Protocol/EFIPmicVersion.h>
#include <Protocol/EFIRamPartition.h>
#include <Protocol/EFISoftSkuInfo.h>

#define HANDLE_MAX_INFO_LIST 128
#define CHIP_BASE_BAND_LEN 4
#define CHIP_BASE_BAND_MSM "msm"
#define CHIP_BASE_BAND_APQ "apq"
#define CHIP_BASE_BAND_MDM "mdm"

#define BIT(x) (1ULL << x)

extern RamPartitionEntry *RamPartitionEntries;

typedef enum {
  EMMC = 0,
  UFS = 1,
  NAND = 2,
  NVME = 3,
  VBLK = 4,
  UNKNOWN,
} MemCardType;

#define BOOT_DEVICE_SHIFT      16
#define DDR_SHIFT              8

#define MB             (1024 * 1024UL)
#define DDR_128MB      (128 * MB)
#define DDR_256MB      (256 * MB)
#define DDR_512MB      (512 * MB)
#define DDR_1024MB     (1024 * MB)
#define DDR_2048MB     (2048 * MB)
#define DDR_3072MB     (3072 * MB)
#define DDR_4096MB     (4096 * MB)

typedef enum {
  DDRTYPE_256MB = 1,
  DDRTYPE_512MB,
  DDRTYPE_1024MB,
  DDRTYPE_2048MB,
  DDRTYPE_3072MB,
  DDRTYPE_4096MB,
  DDRTYPE_128MB,
} DdrType;

struct BoardInfo {
  EFI_PLATFORMINFO_PLATFORM_INFO_TYPE PlatformInfo;
  UINT32 RawChipId;
  CHAR8 ChipBaseBand[EFICHIPINFO_MAX_ID_LENGTH];
  EFIChipInfoVersionType ChipVersion;
  EFIChipInfoFoundryIdType FoundryId;
  UINT32 PackageId;
  UINT32 HlosSubType;
  UINT32 SoftSkuId;
};

EFI_STATUS
BaseMem (UINT64 *BaseMemory);

UINT32
BoardPmicModel (UINT32 PmicDeviceIndex);

UINT32
BoardPmicTarget (UINT32 PmicDeviceIndex);

EFI_STATUS BoardInit (VOID);

EFI_STATUS
BoardSerialNum (CHAR8 *StrSerialNum, UINT32 Len);
UINT32 BoardPlatformRawChipId (VOID);
CHAR8 *BoardPlatformChipBaseBand (VOID);
EFIChipInfoVersionType BoardPlatformChipVersion (VOID);
EFIChipInfoFoundryIdType BoardPlatformFoundryId (VOID);
UINT32 BoardPlatformPackageId (VOID);
EFI_PLATFORMINFO_PLATFORM_TYPE BoardPlatformType (VOID);
UINT32 BoardPlatformVersion (VOID);
UINT32 BoardPlatformSubType (VOID);
UINT32 BoardOEMVariantId (VOID);
UINT32 BoardTargetId (VOID);
VOID
GetRootDeviceType (CHAR8 *StrDeviceType, UINT32 Len);
MemCardType
CheckRootDeviceType (VOID);
VOID
BoardHwPlatformName (CHAR8 *StrHwPlatform, UINT32 Len);
EFI_STATUS
UfsGetSetBootLun (UINT32 *UfsBootlun, BOOLEAN IsGet);
BOOLEAN BoardPlatformFusion (VOID);
UINT32 BoardPlatformRawChipId (VOID);
EFI_STATUS ReadRamPartitions (RamPartitionEntry **RamPartitions,
                  UINT32 *NumPartitions);
EFI_STATUS GetGranuleSize (UINT32 *MinPasrGranuleSize);
VOID GetPageSize (UINT32 *PageSize);
EFI_STATUS GetDdrSize (UINT64 *DdrSize);
EFI_STATUS BoardDdrType (UINT32 *Type);
UINT32 BoardPlatformHlosSubType (VOID);
VOID BoardSoftSku (EFI_SOFT_SKU_ID *SkuId);
UINT32 BoardSoftSkuId (VOID);
#endif
