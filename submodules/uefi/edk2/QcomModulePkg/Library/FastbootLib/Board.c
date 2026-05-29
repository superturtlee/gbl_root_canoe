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
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
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

#include "AutoGen.h"
#include <Board.h>
#include <Library/BootImage.h>
#include <Library/UpdateCmdLine.h>
#include <Library/UpdateDeviceTree.h>
#include <Protocol/EFICardInfo.h>
#include <Protocol/EFIPlatformInfoTypes.h>

#include <LinuxLoaderLib.h>

STATIC struct BoardInfo platform_board_info;

STATIC CONST CHAR8 *DeviceType[] = {
        [EMMC] = "EMMC",
        [UFS] = "UFS",
        [NAND] = "NAND",
        [NVME] = "NVME",
        [VBLK] = "VBLK",
        [UNKNOWN] = "Unknown",
};

STATIC CONST UINT32 DeviceTypeMedia[] = {
        [EMMC] = SIGNATURE_32 ('e', 'm', 'm', 'c'),
        [UFS] = SIGNATURE_32 ('u', 'f', 's', ' '),
        [NAND] = SIGNATURE_32 ('n', 'a', 'n', 'd'),
        [NVME] = SIGNATURE_32 ('n', 'v' , 'm', 'e'),
        [VBLK] = SIGNATURE_32 ('V', 'B', 'L', 'K'),
};

RamPartitionEntry *RamPartitionEntries = NULL;

STATIC EFI_STATUS
GetRamPartitions (RamPartitionEntry **RamPartitions, UINT32 *NumPartitions)
{

  EFI_STATUS Status = EFI_NOT_FOUND;
  EFI_RAMPARTITION_PROTOCOL *pRamPartProtocol = NULL;

  Status = gBS->LocateProtocol (&gEfiRamPartitionProtocolGuid, NULL,
                                (VOID **)&pRamPartProtocol);
  if (EFI_ERROR (Status) || (pRamPartProtocol == NULL)) {
    DEBUG ((EFI_D_ERROR,
            "Locate EFI_RAMPARTITION_Protocol failed, Status =  (0x%x)\r\n",
            Status));
    return EFI_NOT_FOUND;
  }
  Status = pRamPartProtocol->GetRamPartitions (pRamPartProtocol, NULL,
                                               NumPartitions);
  if (Status == EFI_BUFFER_TOO_SMALL) {
    *RamPartitions = AllocateZeroPool (
                         *NumPartitions * sizeof (RamPartitionEntry));
    if (*RamPartitions == NULL)
      return EFI_OUT_OF_RESOURCES;

    Status = pRamPartProtocol->GetRamPartitions (pRamPartProtocol,
                                                 *RamPartitions, NumPartitions);
    if (EFI_ERROR (Status) || (*NumPartitions < 1)) {
      DEBUG ((EFI_D_ERROR, "Failed to get RAM partitions"));
      FreePool (*RamPartitions);
      *RamPartitions = NULL;
      return EFI_NOT_FOUND;
    }
  } else {
    DEBUG ((EFI_D_ERROR, "Error Occured while populating RamPartitions\n"));
    return EFI_PROTOCOL_ERROR;
  }
  return Status;
}

EFI_STATUS
ReadRamPartitions (RamPartitionEntry **RamPartitions, UINT32 *NumPartitions)
{
  STATIC UINT32 NumPartitionEntries = 0;
  EFI_STATUS Status = EFI_SUCCESS;

  if (RamPartitionEntries == NULL) {
    NumPartitionEntries = 0;
    Status = GetRamPartitions (&RamPartitionEntries, &NumPartitionEntries);
    if (EFI_ERROR (Status)) {
      DEBUG ((EFI_D_ERROR, "Error returned from GetRamPartitions %r\n",
              Status));
      return Status;
    }
    if (!RamPartitionEntries) {
      DEBUG ((EFI_D_ERROR, "RamPartitions is NULL\n"));
      return EFI_NOT_FOUND;
    }
  }

  *RamPartitions = RamPartitionEntries;
  *NumPartitions = NumPartitionEntries;

  return Status;
}

EFI_STATUS
GetGranuleSize (UINT32 *MinPasrGranuleSize)
{
  EFI_STATUS Status = EFI_NOT_FOUND;
  EFI_RAMPARTITION_PROTOCOL *pRamPartProtocol = NULL;

  Status = gBS->LocateProtocol (&gEfiRamPartitionProtocolGuid, NULL,
                                (VOID **)&pRamPartProtocol);
  if (EFI_ERROR (Status) ||
        (pRamPartProtocol == NULL)) {
    DEBUG ((EFI_D_ERROR,
            "Locate EFI_RAMPARTITION_Protocol failed, Status = %r \n",
            Status));
    return Status;
  }

  if (pRamPartProtocol->Revision < EFI_RAMPARTITION_PROTOCOL_REVISION) {
    DEBUG ((EFI_D_ERROR, "WARNING: Unsupported EFI_RAMPARTITION_PROTOCOL\n"));
    return EFI_UNSUPPORTED;
  }

  Status = pRamPartProtocol->GetMinPasrSize (pRamPartProtocol,
                                             MinPasrGranuleSize);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR,
            "Failed to get MinPasrSize, Status = %r\n",
            Status));
    return Status;
  }
  return Status;
}

EFI_STATUS
BaseMem (UINT64 *BaseMemory)
{
  EFI_STATUS Status = EFI_NOT_FOUND;
#ifdef AUTO_VIRT_ABL
  UINTN DataSize = 0;
  DataSize = sizeof (*BaseMemory);
  Status = gRT->GetVariable ((CHAR16 *)L"MemoryBase", &gQcomTokenSpaceGuid,
                          NULL, &DataSize, BaseMemory);
#else
  RamPartitionEntry *RamPartitions = NULL;
  UINT32 NumPartitions = 0;
  UINT64 SmallestBase;
  UINT32 i = 0;

  Status = ReadRamPartitions (&RamPartitions, &NumPartitions);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "Error returned from ReadRamPartitions %r\n", Status));
    return Status;
  }
  SmallestBase = RamPartitions[0].Base;
  for (i = 0; i < NumPartitions; i++) {
    if (SmallestBase > RamPartitions[i].Base)
      SmallestBase = RamPartitions[i].Base;
  }
  *BaseMemory = SmallestBase;
#endif
  DEBUG ((EFI_D_INFO, "Memory Base Address: 0x%x\n", *BaseMemory));

  return Status;
}

STATIC EFI_STATUS
GetChipInfo (struct BoardInfo *platform_board_info,
             EFIChipInfoModemType *ModemType)
{
  EFI_STATUS Status;
  EFI_CHIPINFO_PROTOCOL *pChipInfoProtocol;

  Status = gBS->LocateProtocol (&gEfiChipInfoProtocolGuid, NULL,
                                (VOID **)&pChipInfoProtocol);
  if (EFI_ERROR (Status))
    return Status;
  Status = pChipInfoProtocol->GetChipId (pChipInfoProtocol,
                                         &platform_board_info->RawChipId);
  if (EFI_ERROR (Status))
    return Status;
  Status = pChipInfoProtocol->GetChipVersion (
      pChipInfoProtocol, &platform_board_info->ChipVersion);
  if (EFI_ERROR (Status))
    return Status;
  Status = pChipInfoProtocol->GetFoundryId (pChipInfoProtocol,
                                            &platform_board_info->FoundryId);
  if (EFI_ERROR (Status))
    return Status;
  if (pChipInfoProtocol->Revision >= EFI_CHIPINFO_VERSION (1, 6)) {
    Status = pChipInfoProtocol->GetRawPackageType (
        pChipInfoProtocol, &platform_board_info->PackageId);
    if (EFI_ERROR (Status)) {
      platform_board_info->PackageId = 0;
    }
  } else {
    platform_board_info->PackageId = 0;
  }
  Status = pChipInfoProtocol->GetModemSupport (
      pChipInfoProtocol, ModemType);
  if (EFI_ERROR (Status))
    return Status;

  return Status;
}

STATIC EFI_STATUS
GetPlatformInfo (struct BoardInfo *platform_board_info)
{
  EFI_STATUS eResult;
  EFI_PLATFORMINFO_PROTOCOL *hPlatformInfoProtocol;

  eResult = gBS->LocateProtocol (&gEfiPlatformInfoProtocolGuid, NULL,
                                 (VOID **)&hPlatformInfoProtocol);
  if (eResult != EFI_SUCCESS) {
    AsciiPrint ("Error: Failed to locate PlatformInfo protocol.\n");
    goto endtest;
  }

  eResult = hPlatformInfoProtocol->GetPlatformInfo (
      hPlatformInfoProtocol, &platform_board_info->PlatformInfo);
  if (eResult != EFI_SUCCESS) {
    AsciiPrint ("Error: GetPlatformInfo failed.\n");
    goto endtest;
  }

  if (platform_board_info->PlatformInfo.platform >=
      EFI_PLATFORMINFO_NUM_TYPES) {
    AsciiPrint ("Error: Unknown platform type (%d).\n",
                platform_board_info->PlatformInfo.platform);
    eResult = EFI_PROTOCOL_ERROR;
    goto endtest;
  }

  DEBUG ((EFI_D_VERBOSE, "Platform Info : 0x%x\n",
          platform_board_info->PlatformInfo.platform));
endtest:
  return eResult;
}

STATIC EFI_STATUS
GetPmicInfoExt (UINT32 PmicDeviceIndex,
                EFI_PM_DEVICE_INFO_EXT_TYPE *pmic_info_ext)
{
  EFI_STATUS Status;
  EFI_QCOM_PMIC_VERSION_PROTOCOL *pPmicVersionProtocol;

  Status = gBS->LocateProtocol (&gQcomPmicVersionProtocolGuid, NULL,
                                (VOID **)&pPmicVersionProtocol);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "Error locating pmic protocol: %r\n", Status));
    return Status;
  }

  Status =
      pPmicVersionProtocol->GetPmicInfoExt (PmicDeviceIndex, pmic_info_ext);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_VERBOSE, "Error getting pmic info ext: %r\n", Status));
    return Status;
  }
  return Status;
}

STATIC EFI_STATUS
GetPmicInfo (UINT32 PmicDeviceIndex,
             EFI_PM_DEVICE_INFO_TYPE *pmic_info,
             UINT64 *Revision)
{
  EFI_STATUS Status;
  EFI_QCOM_PMIC_VERSION_PROTOCOL *pPmicVersionProtocol;

  Status = gBS->LocateProtocol (&gQcomPmicVersionProtocolGuid, NULL,
                                (VOID **)&pPmicVersionProtocol);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "Error locating pmic protocol: %r\n", Status));
    return Status;
  }

  *Revision = pPmicVersionProtocol->Revision;

  Status = pPmicVersionProtocol->GetPmicInfo (PmicDeviceIndex, pmic_info);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "Error getting pmic info: %r\n", Status));
    return Status;
  }
  return Status;
}

/**
 Device Handler Info

 @param[out]     HndlInfo  : Pointer to array of HandleInfo structures
                               in which the output is returned.
 @param[in, out] MaxHandles  : On input, max number of handle structures
                               the buffer can hold, On output, the number
                               of handle structures returned.
 @param[in]      Type        : Device Type : UNKNOWN, UFS, EMMC, NAND, NVME,
                                             VBLK
 @retval         EFI_STATUS  : Return Success on getting Handler Info
 **/

STATIC EFI_STATUS
GetDeviceHandleInfo (VOID *HndlInfo, UINT32 MaxHandles, MemCardType Type)
{
  EFI_STATUS Status = EFI_INVALID_PARAMETER;
  UINT32 Attribs = 0;
  PartiSelectFilter HandleFilter;
  HandleInfo *HandleInfoList = HndlInfo;

  Attribs |= BLK_IO_SEL_MATCH_ROOT_DEVICE;
  HandleFilter.PartitionType = NULL;
  HandleFilter.VolumeName = NULL;

  HandleFilter.RootDeviceType = &gEfiUfsLU0Guid;

  Status =
     GetBlkIOHandles (Attribs, &HandleFilter, HandleInfoList, &MaxHandles);
  if (EFI_ERROR (Status) ||
     MaxHandles == 0) {
    DEBUG ((EFI_D_ERROR, "Get BlkIohandles failed\n"));
    return Status;
  }
  return Status;
}

/**
 Return a device type
 @retval         Device type : UNKNOWN | UFS | EMMC | NAND | NVME | VBLK
 **/

/**
 Return a device type
 @retval         Device type : UNKNOWN | UFS | EMMC | NAND | NVME | VBLK
                               default is UNKNOWN
 **/

MemCardType
CheckRootDeviceType (VOID)
{

  return UFS;
}

/**
 Get device type
 @param[out]  StrDeviceType  : Pointer to array of device type string.
 @param[in]   Len            : The size of the device type string
 **/
VOID
GetRootDeviceType (CHAR8 *StrDeviceType, UINT32 Len)
{
  UINT32 Type;

  Type = CheckRootDeviceType ();
  AsciiSPrint (StrDeviceType, Len, "%a", DeviceType[Type]);
}

/**
 Get device page size
 @param[out]  PageSize  : Pointer to the page size.
 **/
VOID
GetPageSize (UINT32 *PageSize)
{
  EFI_BLOCK_IO_PROTOCOL *BlkIo = NULL;
  HandleInfo HandleInfoList[HANDLE_MAX_INFO_LIST];
  UINT32 MaxHandles = ARRAY_SIZE (HandleInfoList);
  UINT32 Type;
  EFI_STATUS Status = EFI_INVALID_PARAMETER;

  *PageSize = BOOT_IMG_MAX_PAGE_SIZE;
  Type = CheckRootDeviceType ();
  Status = GetDeviceHandleInfo (HandleInfoList, MaxHandles, Type);
  if (Status == EFI_SUCCESS) {
    BlkIo = HandleInfoList[0].BlkIo;
    *PageSize = BlkIo->Media->BlockSize;
  }
}

UINT32
BoardPmicModel (UINT32 PmicDeviceIndex)
{
  EFI_STATUS Status;
  EFI_PM_DEVICE_INFO_TYPE pmic_info;
  UINT64 Revision;

  if ( BoardPlatformType () == EFI_PLATFORMINFO_TYPE_RUMI ) {
    return 0;
  }

  Status = GetPmicInfo (PmicDeviceIndex, &pmic_info, &Revision);
  if (Status != EFI_SUCCESS) {
    DEBUG ((EFI_D_ERROR, "Error getting pmic model info: %r\n", Status));
    ASSERT (0);
  }
  DEBUG ((EFI_D_VERBOSE, "PMIC Model 0x%x: 0x%x\n", PmicDeviceIndex,
          pmic_info.PmicModel));
  return pmic_info.PmicModel;
}

UINT32
BoardPmicTarget (UINT32 PmicDeviceIndex)
{
  UINT32 target;
  EFI_STATUS Status;
  UINT64 Revision;
  EFI_PM_DEVICE_INFO_TYPE pmic_info;
  EFI_PM_DEVICE_INFO_EXT_TYPE pmic_info_ext;

  if ( BoardPlatformType () == EFI_PLATFORMINFO_TYPE_RUMI ) {
    return 0;
  }

  Status = GetPmicInfo (PmicDeviceIndex, &pmic_info, &Revision);
  if (Status != EFI_SUCCESS) {
    DEBUG ((EFI_D_ERROR, "Error finding board pmic info: %r\n", Status));
    ASSERT (0);
  }

  if (Revision == PMIC_EXT_VERSION) {
    Status = GetPmicInfoExt (PmicDeviceIndex, &pmic_info_ext);
    if (Status != EFI_SUCCESS) {
      DEBUG ((EFI_D_VERBOSE, "Error finding board pmic info: %r\n", Status));
      return 0;
    }

    target = (pmic_info_ext.PmicVariantRevision << 24) |
             (pmic_info_ext.PmicAllLayerRevision << 16) |
             (pmic_info_ext.PmicMetalRevision << 8) | pmic_info_ext.PmicModel;
  } else {
    target = (pmic_info.PmicAllLayerRevision << 16) |
             (pmic_info.PmicMetalRevision << 8) | pmic_info.PmicModel;
  }

  DEBUG ((EFI_D_VERBOSE, "PMIC Target 0x%x: 0x%x\n", PmicDeviceIndex, target));
  return target;
}

EFI_STATUS BoardInit (VOID)
{
  EFI_STATUS Status;
  EFIChipInfoModemType ModemType;
  UINT32 DdrType;
  UINT32 BootDeviceType;
  EFI_SOFT_SKU_ID SKUId;

  Status = GetChipInfo (&platform_board_info, &ModemType);
  if (EFI_ERROR (Status))
    return Status;

  Status = GetPlatformInfo (&platform_board_info);
  if (EFI_ERROR (Status))
    return Status;

  Status = BoardDdrType (&DdrType);
  if (EFI_ERROR (Status))
    return Status;

  BootDeviceType = CheckRootDeviceType ();

  platform_board_info.HlosSubType = (BootDeviceType << BOOT_DEVICE_SHIFT);
  platform_board_info.HlosSubType |= (DdrType << DDR_SHIFT);

  BoardSoftSku (&SKUId);
  platform_board_info.SoftSkuId = SKUId.eSoftSKUId;

  if (BoardPlatformFusion ()) {
    AsciiSPrint ((CHAR8 *)platform_board_info.ChipBaseBand,
                  CHIP_BASE_BAND_LEN, "%a", CHIP_BASE_BAND_MDM);
  } else if (ModemType == 0) {
    AsciiSPrint ((CHAR8 *)platform_board_info.ChipBaseBand,
                  CHIP_BASE_BAND_LEN, "%a", CHIP_BASE_BAND_APQ);
  } else {
    AsciiSPrint ((CHAR8 *)platform_board_info.ChipBaseBand,
                  CHIP_BASE_BAND_LEN, "%a", CHIP_BASE_BAND_MSM);
  }

  DEBUG ((EFI_D_VERBOSE, "Raw Chip Id   : 0x%x\n",
          platform_board_info.RawChipId));
  DEBUG ((EFI_D_VERBOSE, "Chip Version  : 0x%x\n",
          platform_board_info.ChipVersion));
  DEBUG ((EFI_D_VERBOSE, "Foundry Id    : 0x%x\n",
          platform_board_info.FoundryId));
  DEBUG ((EFI_D_VERBOSE, "Chip BaseBand    : %a\n",
          platform_board_info.ChipBaseBand));
  DEBUG ((EFI_D_VERBOSE, "Fusion Value    : %d\n",
          platform_board_info.PlatformInfo.fusion));
  DEBUG ((EFI_D_VERBOSE, "HLOS SubType    : 0x%x\n",
          platform_board_info.HlosSubType));
  DEBUG ((EFI_D_VERBOSE, "SoftSKUId    : 0x%x\n",
          platform_board_info.SoftSkuId));

  return Status;
}

EFI_STATUS
UfsGetSetBootLun (UINT32 *UfsBootlun, BOOLEAN IsGet)
{
  EFI_STATUS Status = EFI_INVALID_PARAMETER;
  EFI_MEM_CARDINFO_PROTOCOL *CardInfo;
  HandleInfo HandleInfoList[MAX_HANDLE_INFO_LIST];
  UINT32 Attribs = 0;
  UINT32 MaxHandles;
  PartiSelectFilter HandleFilter;

  Attribs |= BLK_IO_SEL_MATCH_ROOT_DEVICE;
  MaxHandles = ARRAY_SIZE (HandleInfoList);
  HandleFilter.PartitionType = NULL;
  HandleFilter.VolumeName = NULL;
  HandleFilter.RootDeviceType = &gEfiUfsLU0Guid;

  Status =
      GetBlkIOHandles (Attribs, &HandleFilter, HandleInfoList, &MaxHandles);
  if (EFI_ERROR (Status))
    return EFI_NOT_FOUND;

  Status =
      gBS->HandleProtocol (HandleInfoList[0].Handle,
                           &gEfiMemCardInfoProtocolGuid, (VOID **)&CardInfo);

  if (Status != EFI_SUCCESS) {
    DEBUG ((EFI_D_ERROR, "Error locating MemCardInfoProtocol:%x\n", Status));
    return Status;
  }

  if (CardInfo->Revision < EFI_MEM_CARD_INFO_PROTOCOL_REVISION_3) {
    DEBUG ((EFI_D_ERROR, "This API not supported in Revision =%u\n",
            CardInfo->Revision));
    return EFI_NOT_FOUND;
  }

  if (IsGet == TRUE) {
    if (CardInfo->GetBootLU (CardInfo, UfsBootlun) == EFI_SUCCESS)
      DEBUG ((EFI_D_VERBOSE, "Get BootLun =%u\n", *UfsBootlun));
  } else {
    if (CardInfo->SetBootLU (CardInfo, *UfsBootlun) == EFI_SUCCESS)
      DEBUG ((EFI_D_VERBOSE, "SetBootLun =%u\n", *UfsBootlun));
  }
  return Status;
}

EFI_STATUS
GetMemCardInfo (VOID **MemCardInfo)
{
  EFI_STATUS Status = EFI_INVALID_PARAMETER;
  EFI_MEM_CARDINFO_PROTOCOL *CardInfo = NULL;
  EFI_HANDLE *Handles = NULL;
  UINTN NumHandles = 0;
  UINT8 Index = 0;
  UINT8 Count = 0;
  EFI_BLOCK_IO_PROTOCOL *BlkIo = NULL;
  BOOLEAN CardFound = FALSE;
  UINT32 MediaSignature = 0;
  BOOLEAN RemovableMedia = FALSE;

  // Locate all instances of MemCardInfoProtocol
  Status = gBS->LocateHandleBuffer (ByProtocol,
    &gEfiMemCardInfoProtocolGuid, NULL, &NumHandles, &Handles);
  if (EFI_ERROR (Status) ||
     !Handles) {
    DEBUG ((EFI_D_ERROR,
      "LocateHandleBuffer(gEfiMemCardInfoProtocolGuid) failed: %r\n",
      Status));
    return Status;
  }

  for (Index = 0; Index < NumHandles; Index++) {
    Status = gBS->HandleProtocol (Handles[Index],
                 &gEfiMemCardInfoProtocolGuid, (VOID **)&CardInfo);
    if (EFI_ERROR (Status)) {
      continue;
    }

    if (CardInfo->Revision >= EFI_MEM_CARD_INFO_PROTOCOL_REVISION_4 &&
        CardInfo->Media != NULL) {
      RemovableMedia = CardInfo->Media->RemovableMedia;
      MediaSignature = CardInfo->Media->MediaId;
    } else {
      Status = gBS->HandleProtocol (Handles[Index],
               &gEfiBlockIoProtocolGuid, (VOID **) &BlkIo);
      if (EFI_ERROR (Status)) {
        continue;
      }
      RemovableMedia = BlkIo->Media->RemovableMedia;
      MediaSignature = BlkIo->Media->MediaId;
    }

    if (RemovableMedia == FALSE) {
      for (Count = 0; Count < ARRAY_SIZE (DeviceTypeMedia); Count++) {
        if (DeviceTypeMedia[Count] == MediaSignature) {
          CardFound = TRUE;
          break;
        }
      }
    }

    if (CardFound) {
      *MemCardInfo = (VOID *) CardInfo;
      break;
    }
  }

  // free the buffer for the handles
  gBS->FreePool (Handles);
  return Status;
}
const CHAR8 * hardcodesn= "0000000000000000";
EFI_STATUS
BoardSerialNum (CHAR8 *StrSerialNum, UINT32 Len)
{
  for (UINT32 i = 0; i < Len - 1 && hardcodesn[i] != '\0'; i++) {
    StrSerialNum[i] = hardcodesn[i];
  }
  StrSerialNum[Len - 1] = '\0';
  return EFI_SUCCESS;
}

/* Helper APIs for device tree selection */
UINT32 BoardPlatformRawChipId (VOID)
{
  return platform_board_info.RawChipId;
}

EFIChipInfoVersionType BoardPlatformChipVersion (VOID)
{
  return platform_board_info.ChipVersion;
}

EFIChipInfoFoundryIdType BoardPlatformFoundryId (VOID)
{
  return platform_board_info.FoundryId;
}

UINT32 BoardPlatformPackageId (VOID)
{
  return platform_board_info.PackageId;
}

CHAR8 *BoardPlatformChipBaseBand (VOID)
{
  return platform_board_info.ChipBaseBand;
}

EFI_PLATFORMINFO_PLATFORM_TYPE BoardPlatformType (VOID)
{
  return platform_board_info.PlatformInfo.platform;
}

UINT32 BoardPlatformVersion (VOID)
{
  return platform_board_info.PlatformInfo.version;
}

UINT32 BoardPlatformSubType (VOID)
{
  return platform_board_info.PlatformInfo.subtype;
}

UINT32 BoardOEMVariantId (VOID)
{
  return platform_board_info.PlatformInfo.OEMVariantID;
}

BOOLEAN BoardPlatformFusion (VOID)
{
  return platform_board_info.PlatformInfo.fusion;
}

UINT32 BoardTargetId (VOID)
{
  UINT32 Target;

  Target = (((platform_board_info.PlatformInfo.subtype & 0xff) << 24) |
            (((platform_board_info.PlatformInfo.version >> 16) & 0xff) << 16) |
            ((platform_board_info.PlatformInfo.version & 0xff) << 8) |
            (platform_board_info.PlatformInfo.platform & 0xff));

  return Target;
}

VOID
BoardHwPlatformName (CHAR8 *StrHwPlatform, UINT32 Len)
{
  EFI_STATUS Status;
  EFI_CHIPINFO_PROTOCOL *pChipInfoProtocol;
  UINT32 ChipIdValidLen = 4;

  if (StrHwPlatform == NULL) {
    DEBUG ((EFI_D_ERROR, "Error: HW Platform string is NULL\n"));
    return;
  }

  Status = gBS->LocateProtocol (&gEfiChipInfoProtocolGuid, NULL,
                                (VOID **)&pChipInfoProtocol);
  if (EFI_ERROR (Status)) {
    DEBUG (
        (EFI_D_ERROR, "Locate Protocol failed for gEfiChipInfoProtocolGuid\n"));
    return;
  }

  Status = pChipInfoProtocol->GetChipIdString (
      pChipInfoProtocol, StrHwPlatform,
      EFICHIPINFO_MAX_ID_LENGTH > Len ? Len : EFICHIPINFO_MAX_ID_LENGTH);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "Failed to Get the ChipIdString\n"));
    return;
  }

  if (Len < ChipIdValidLen)
    StrHwPlatform[Len - 1] = '\0';
  else
    StrHwPlatform[ChipIdValidLen - 1] = '\0';
}

EFI_STATUS GetDdrSize (UINT64 *DdrSizeRet)
{
  EFI_STATUS Status;
  RamPartitionEntry *RamPartitions = NULL;
  UINT32 i = 0;
  STATIC UINT64 DdrSize;
  UINT32 NumPartitions = 0;

  if (DdrSizeRet == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (DdrSize) {
    *DdrSizeRet = DdrSize;
    return EFI_SUCCESS;
  }

  Status = ReadRamPartitions (&RamPartitions, &NumPartitions);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "Error returned from GetRamPartitions %r\n", Status));
    return Status;
  }

  *DdrSizeRet = 0;

  for (i = 0; i < NumPartitions; i++) {
    *DdrSizeRet += RamPartitions[i].AvailableLength;
  }

  DdrSize = *DdrSizeRet;

  return Status;
}

EFI_STATUS BoardDdrType (UINT32 *Type)
{
  EFI_STATUS Status;
  UINT64 DdrSize = 0;

  Status = GetDdrSize (&DdrSize);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "Error getting DDR size %r\n", Status));
    return Status;
  }

  DEBUG ((EFI_D_INFO, "Total DDR Size: 0x%016lx \n", DdrSize));

  *Type = 0;
  if (DdrSize <= DDR_128MB) {
    *Type = DDRTYPE_128MB;
  } else if (DdrSize <= DDR_256MB) {
    *Type = DDRTYPE_256MB;
  } else if (DdrSize <= DDR_512MB) {
    *Type = DDRTYPE_512MB;
  } else if (DdrSize <= DDR_1024MB) {
    *Type = DDRTYPE_1024MB;
  } else if (DdrSize <= DDR_2048MB) {
    *Type = DDRTYPE_2048MB;
  } else if (DdrSize <= DDR_3072MB) {
    *Type = DDRTYPE_3072MB;
  } else if (DdrSize <= DDR_4096MB) {
    *Type = DDRTYPE_4096MB;
  }

  return Status;
}

UINT32 BoardPlatformHlosSubType (VOID)
{
  return platform_board_info.HlosSubType;
}

UINT32 BoardSoftSkuId (VOID)
{
  return platform_board_info.SoftSkuId;
}
