/* Copyright (c) 2015-2018, 2020, The Linux Foundation. All rights reserved.
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
#define PAGE_SHIFT 12
#define PAGE_SIZE (_AC(1, UL) << PAGE_SHIFT)
#define DIV_ROUND_UP(n, d) (((n) + (d) - 1) / (d))
#define HIBERNATE_SIG           "S1SUSPEND"
#define __NEW_UTS_LEN 64
#define ALIGN32_BELOW(addr) ALIGN_POINTER (addr - 32, 32)
#define LOCAL_ROUND_TO_PAGE(x, y) (((x) + (y - 1)) & (~(y - 1)))
#define ROUND_TO_PAGE(x, y) ((ADD_OF ((x), (y))) & (~(y)))
#define ALIGN_PAGES(x, y) (((x) + (y - 1)) / (y))
#define DECOMPRESS_SIZE_FACTOR 8
#define ALIGNMENT_MASK_4KB 4096
#define MAX_NUMBER_OF_LOADED_IMAGES 32
#define PVMFW_CONFIG_MAX_BLOBS 2
#include "LinuxLoaderLib.h"
#include "AutoGen.h"
#include <FastbootLib/FastbootCmds.h>

/* Volume Label size 11 chars, round off to 16 */
#define VOLUME_LABEL_SIZE 16

/* List of all the filters that need device path protocol in the handle to
 * filter */
#define FILTERS_NEEDING_DEVICEPATH                                             \
  (BLK_IO_SEL_PARTITIONED_MBR | BLK_IO_SEL_PARTITIONED_GPT |                   \
   BLK_IO_SEL_MATCH_PARTITION_TYPE_GUID | BLK_IO_SEL_SELECT_ROOT_DEVICE_ONLY | \
   BLK_IO_SEL_MATCH_ROOT_DEVICE)

/* FileInfo-size = SIZE_OF_EFI_FILE_INFO + sizeof(name of directory entry)
   Since we don't know the sizeof(name of directory entry),
   we can set FileInfo-size = SIZE_OF_EFI_FILE_INFO + 256*/
#define FILE_INFO_SIZE (SIZE_OF_EFI_FILE_INFO + 256)

STATIC UINT32 TimerFreq, FactormS;
STATIC MemCardType RootDeviceType = UNKNOWN;

/* Returns 0 if the volume label matches otherwise non zero */
STATIC UINTN
CompareVolumeLabel (IN EFI_SIMPLE_FILE_SYSTEM_PROTOCOL*   Fs,
                    IN CHAR8*                             ReqVolumeName)
{
  INT32 CmpResult;
  UINT32 j;
  UINT16 VolumeLabel[VOLUME_LABEL_SIZE];
  EFI_FILE_PROTOCOL  *FsVolume = NULL;
  EFI_STATUS         Status;
  UINTN                               Size;
  EFI_FILE_SYSTEM_INFO                *FsInfo;

  // Get information about the volume
  Status = Fs->OpenVolume (Fs, &FsVolume);

  if (Status != EFI_SUCCESS) {
    return 1;
  }

  /* Get the Volume name */
  Size = 0;
  FsInfo = NULL;
  Status = FsVolume->GetInfo (FsVolume, &gEfiFileSystemInfoGuid, &Size, FsInfo);
  if (Status == EFI_BUFFER_TOO_SMALL) {
    FsInfo = AllocateZeroPool (Size);
    Status = FsVolume->GetInfo (FsVolume,
                                &gEfiFileSystemInfoGuid, &Size, FsInfo);
    if (Status != EFI_SUCCESS) {
      FreePool (FsInfo);
      return 1;
    }
  }

  if (FsInfo == NULL) {
    return 1;
  }

  /* Convert the passed in Volume name to Wide char and upper case */
  for (j = 0; (j < VOLUME_LABEL_SIZE - 1) && ReqVolumeName[j]; ++j) {
    VolumeLabel[j] = ReqVolumeName[j];

    if ((VolumeLabel[j] >= 'a') &&
        (VolumeLabel[j] <= 'z')) {
      VolumeLabel[j] -= ('a' - 'A');
    }
  }

  /* Null termination */
  VolumeLabel[j] = 0;

  /* Change any lower chars in volume name to upper
   * (ideally this is not needed) */
  for (j = 0; (j < VOLUME_LABEL_SIZE - 1) && FsInfo->VolumeLabel[j]; ++j) {
    if ((FsInfo->VolumeLabel[j] >= 'a') &&
          (FsInfo->VolumeLabel[j] <= 'z')) {
      FsInfo->VolumeLabel[j] -= ('a' - 'A');
    }
  }

  CmpResult = StrnCmp (FsInfo->VolumeLabel, VolumeLabel, VOLUME_LABEL_SIZE);

  FreePool (FsInfo);
  FsVolume->Close (FsVolume);

  return CmpResult;
}

/**
  Returns a list of BlkIo handles based on required criteria
SelectionAttrib : Bitmask representing the conditions that need
to be met for the handles returned. Based on the
selections filter members should have valid values.
FilterData      : Instance of Partition Select Filter structure that
needs extended data for certain type flags. For example
Partition type and/or Volume name can be specified.
HandleInfoPtr   : Pointer to array of HandleInfo structures in which the
output is returned.
MaxBlkIopCnt    : On input, max number of handle structures the buffer can hold,
On output, the number of handle structures returned.

@retval EFI_SUCCESS if the operation was successful
*/
EFI_STATUS
EFIAPI
GetBlkIOHandles (IN UINT32 SelectionAttrib,
                 IN PartiSelectFilter *FilterData,
                 OUT HandleInfo *HandleInfoPtr,
                 IN OUT UINT32* MaxBlkIopCnt)
{
  EFI_BLOCK_IO_PROTOCOL *BlkIo;
  EFI_HANDLE *BlkIoHandles;
  UINTN BlkIoHandleCount;
  UINTN i;
  UINTN DevicePathDepth;
  HARDDRIVE_DEVICE_PATH *Partition, *PartitionOut;
  EFI_STATUS Status;
  EFI_DEVICE_PATH_PROTOCOL *DevPathInst;
  EFI_DEVICE_PATH_PROTOCOL *TempDevicePath;
  VENDOR_DEVICE_PATH *RootDevicePath;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL     *Fs;
  UINT32 BlkIoCnt = 0;
  EFI_PARTITION_ENTRY *PartEntry;

  if ((MaxBlkIopCnt == NULL) || (HandleInfoPtr == NULL))
    return EFI_INVALID_PARAMETER;

  /* Adjust some defaults first */
  if ((SelectionAttrib & (BLK_IO_SEL_MEDIA_TYPE_REMOVABLE |
                          BLK_IO_SEL_MEDIA_TYPE_NON_REMOVABLE)) == 0)
    SelectionAttrib |=
        (BLK_IO_SEL_MEDIA_TYPE_REMOVABLE | BLK_IO_SEL_MEDIA_TYPE_NON_REMOVABLE);

  if (((BLK_IO_SEL_PARTITIONED_GPT | BLK_IO_SEL_PARTITIONED_MBR) &
       SelectionAttrib) == 0)
    SelectionAttrib |=
        (BLK_IO_SEL_PARTITIONED_GPT | BLK_IO_SEL_PARTITIONED_MBR);

  /* If we need Filesystem handle then search based on that its narrower search
   * than BlkIo */
  if (SelectionAttrib & (BLK_IO_SEL_SELECT_MOUNTED_FILESYSTEM |
                         BLK_IO_SEL_SELECT_BY_VOLUME_NAME)) {
    Status =
        gBS->LocateHandleBuffer (ByProtocol, &gEfiSimpleFileSystemProtocolGuid,
                                 NULL, &BlkIoHandleCount, &BlkIoHandles);
  } else {
    Status = gBS->LocateHandleBuffer (ByProtocol, &gEfiBlockIoProtocolGuid,
                                      NULL, &BlkIoHandleCount, &BlkIoHandles);
  }

  if (Status != EFI_SUCCESS) {
    DEBUG (
        (EFI_D_ERROR, "Unable to get Filesystem Handle buffer %r\n", Status));
    return Status;
  }

  /* Loop through to search for the ones we are interested in. */
  for (i = 0; i < BlkIoHandleCount; i++) {

    Status = gBS->HandleProtocol (BlkIoHandles[i], &gEfiBlockIoProtocolGuid,
                                  (VOID **)&BlkIo);
    /* Fv volumes will not support Blk I/O protocol */
    if (Status == EFI_UNSUPPORTED) {
      continue;
    }

    if (Status != EFI_SUCCESS) {
      DEBUG ((EFI_D_ERROR, "Unable to get Filesystem Handle %r\n", Status));
      return Status;
    }

    /* Check if the media type criteria (for removable/not) satisfies */
    if (BlkIo->Media->RemovableMedia) {
      if ((SelectionAttrib & BLK_IO_SEL_MEDIA_TYPE_REMOVABLE) == 0)
        continue;
    } else {
      if ((SelectionAttrib & BLK_IO_SEL_MEDIA_TYPE_NON_REMOVABLE) == 0)
        continue;
    }

    /* Clear the pointer, we can get it if the filter is set */
    PartitionOut = NULL;

    /* Check if partition related criteria satisfies */
    if ((SelectionAttrib & FILTERS_NEEDING_DEVICEPATH) != 0) {
      Status = gBS->HandleProtocol (
          BlkIoHandles[i], &gEfiDevicePathProtocolGuid, (VOID **)&DevPathInst);

      /* If we didn't get the DevicePath Protocol then this handle
       * cannot be used */
      if (EFI_ERROR (Status))
        continue;

      DevicePathDepth = 0;

      /* Get the device path */
      TempDevicePath = DevPathInst;
      RootDevicePath = (VENDOR_DEVICE_PATH *)DevPathInst;
      Partition = (HARDDRIVE_DEVICE_PATH *)TempDevicePath;

      if ((SelectionAttrib & (BLK_IO_SEL_SELECT_ROOT_DEVICE_ONLY |
                              BLK_IO_SEL_MATCH_ROOT_DEVICE)) != 0) {
        if (!FilterData) {
          return EFI_INVALID_PARAMETER;
        }
        /* If this is not the root device that we are looking for, ignore this
         * handle */
        if (RootDevicePath->Header.Type != HARDWARE_DEVICE_PATH ||
            RootDevicePath->Header.SubType != HW_VENDOR_DP ||
            (RootDevicePath->Header.Length[0] |
             (RootDevicePath->Header.Length[1] << 8)) !=
#ifdef AUTO_VIRT_ABL
                (sizeof (VENDOR_DEVICE_PATH) + sizeof (UINT64)) ||
#else
                sizeof (VENDOR_DEVICE_PATH) ||
#endif
            ((FilterData->RootDeviceType != NULL) &&
             (CompareGuid (FilterData->RootDeviceType,
                           &RootDevicePath->Guid) == FALSE)))
          continue;
      }

      /* Locate the last Device Path Node */
      while (!IsDevicePathEnd (TempDevicePath)) {
        DevicePathDepth++;
        Partition = (HARDDRIVE_DEVICE_PATH *)TempDevicePath;
        TempDevicePath = NextDevicePathNode (TempDevicePath);
      }

      /* If we need the handle for root device only and if this is representing
       * a sub partition in the root device then ignore this handle */
      if (SelectionAttrib & BLK_IO_SEL_SELECT_ROOT_DEVICE_ONLY)
        if (DevicePathDepth > 1)
          continue;

      /* Check if the last node is Harddrive Device path that contains the
       * Partition information */
#ifndef AUTO_VIRT_ABL
      if (Partition->Header.Type == MEDIA_DEVICE_PATH &&
          Partition->Header.SubType == MEDIA_HARDDRIVE_DP &&
          (Partition->Header.Length[0] | (Partition->Header.Length[1] << 8)) ==
              sizeof (*Partition)) {
#else
      if (Partition->Header.Type == HARDWARE_DEVICE_PATH &&
          Partition->Header.SubType == HW_VENDOR_DP &&
          (Partition->Header.Length[0] | (Partition->Header.Length[1] << 8)) ==
              (sizeof (VENDOR_DEVICE_PATH) + sizeof (UINT64))) {
#endif

        PartitionOut = Partition;

        if ((SelectionAttrib & BLK_IO_SEL_PARTITIONED_GPT) == 0)
          if (Partition->MBRType == PARTITIONED_TYPE_GPT)
            continue;

        if ((SelectionAttrib & BLK_IO_SEL_PARTITIONED_MBR) == 0)
          if (Partition->MBRType == PARTITIONED_TYPE_MBR)
            continue;

        /* PartitionDxe implementation should return partition type also */
        if ((SelectionAttrib & BLK_IO_SEL_MATCH_PARTITION_TYPE_GUID) != 0) {
          GUID *PartiType;
          VOID *Interface;

          if (!FilterData ||
                FilterData->PartitionType == NULL) {
            return EFI_INVALID_PARAMETER;
          }

          Status = gBS->HandleProtocol (BlkIoHandles[i],
                                        FilterData->PartitionType,
                                        (VOID**)&Interface);
          if (EFI_ERROR (Status)) {
              Status = gBS->HandleProtocol (BlkIoHandles[i],
                              &gEfiPartitionTypeGuid,
                              (VOID **)&PartiType);
              if (EFI_ERROR (Status)) {
                continue;
              }

              if (CompareGuid (PartiType, FilterData->PartitionType) == FALSE) {
                continue;
              }
          }
        }
      }
      /* If we wanted a particular partition and didn't get the HDD DP,
         then this handle is probably not the interested ones */
      else if ((SelectionAttrib & BLK_IO_SEL_MATCH_PARTITION_TYPE_GUID) != 0)
          continue;
    }

    /* Check if the Filesystem related criteria satisfies */
    if ((SelectionAttrib & BLK_IO_SEL_SELECT_MOUNTED_FILESYSTEM) != 0) {
      Status = gBS->HandleProtocol (BlkIoHandles[i],
                               &gEfiSimpleFileSystemProtocolGuid, (VOID **)&Fs);
      if (EFI_ERROR (Status)) {
        continue;
      }

      if ((SelectionAttrib & BLK_IO_SEL_SELECT_BY_VOLUME_NAME) != 0) {
        if (!FilterData ||
             FilterData->VolumeName == NULL) {
          return EFI_INVALID_PARAMETER;
        }
        if (CompareVolumeLabel (Fs, FilterData->VolumeName) != 0) {
          continue;
        }
      }
    }

    /* Check if the Partition name related criteria satisfies */
    if ((SelectionAttrib & BLK_IO_SEL_MATCH_PARTITION_LABEL) != 0) {
      Status = gBS->HandleProtocol (BlkIoHandles[i], &gEfiPartitionRecordGuid,
                                    (VOID **)&PartEntry);
      if (Status != EFI_SUCCESS)
        continue;
      if (StrnCmp (PartEntry->PartitionName, FilterData->PartitionLabel,
                   MAX (StrLen (PartEntry->PartitionName),
                        StrLen (FilterData->PartitionLabel))))
        continue;
    }
    /* We came here means, this handle satisfies all the conditions needed,
     * Add it into the list */
    HandleInfoPtr[BlkIoCnt].Handle = BlkIoHandles[i];
    HandleInfoPtr[BlkIoCnt].BlkIo = BlkIo;
    HandleInfoPtr[BlkIoCnt].PartitionInfo = PartitionOut;
    BlkIoCnt++;
    if (BlkIoCnt >= *MaxBlkIopCnt)
      break;
  }

  *MaxBlkIopCnt = BlkIoCnt;

  /* Free the handle buffer */
  if (BlkIoHandles != NULL) {
    FreePool (BlkIoHandles);
    BlkIoHandles = NULL;
  }

  return EFI_SUCCESS;
}

VOID
ToLower (CHAR8 *Str)
{
  UINT32 Index;
  for (Index = 0; Str[Index] != '\0'; Index++) {
    if (Str[Index] >= 'A' && Str[Index] <= 'Z')
      Str[Index] += ('a' - 'A');
  }
}

/* Load image from partition to buffer */
EFI_STATUS
LoadImageFromPartition (VOID *ImageBuffer, UINT32 *ImageSize, CHAR16 *Pname)
{
  EFI_STATUS Status;
  EFI_BLOCK_IO_PROTOCOL *BlkIo;
  PartiSelectFilter HandleFilter;
  HandleInfo HandleInfoList[1];
  STATIC UINT32 MaxHandles;
  STATIC UINT32 BlkIOAttrib = 0;

  BlkIOAttrib = BLK_IO_SEL_PARTITIONED_MBR;
  BlkIOAttrib |= BLK_IO_SEL_PARTITIONED_GPT;
  BlkIOAttrib |= BLK_IO_SEL_MEDIA_TYPE_NON_REMOVABLE;
  BlkIOAttrib |= BLK_IO_SEL_MATCH_PARTITION_LABEL;

  HandleFilter.RootDeviceType = NULL;
  HandleFilter.PartitionLabel = Pname;
  HandleFilter.VolumeName = NULL;

  MaxHandles = sizeof (HandleInfoList) / sizeof (*HandleInfoList);

  Status =
      GetBlkIOHandles (BlkIOAttrib, &HandleFilter, HandleInfoList, &MaxHandles);

  if (Status == EFI_SUCCESS) {
    if (MaxHandles == 0)
      return EFI_NO_MEDIA;

    if (MaxHandles != 1) {
      // Unable to deterministically load from single partition
      DEBUG (
          (EFI_D_INFO, "ExecImgFromVolume(): multiple partitions found.\r\n"));
      return EFI_LOAD_ERROR;
    }
  } else {
    DEBUG ((EFI_D_ERROR,
            "%s: GetBlkIOHandles failed: %r\n", __func__, Status));
    return Status;
  }

  BlkIo = HandleInfoList[0].BlkIo;

  Status = BlkIo->ReadBlocks (
      BlkIo, BlkIo->Media->MediaId, 0,
      ROUND_TO_PAGE (*ImageSize, BlkIo->Media->BlockSize - 1), ImageBuffer);

  if (Status == EFI_SUCCESS) {
    DEBUG ((DEBUG_INFO, "Loading Image %s Done : "
          "%lu ms, Image size : %d Bytes\n",
           Pname, GetTimerCountms () - LoadImageStartTime, *ImageSize));
  }

  return Status;
}

UINT64 GetTimerCountms (VOID)
{
  UINT64 TempFreq, StartVal, EndVal;
  UINT64 TimerCount, Ms;

  if (!TimerFreq && !FactormS) {
    TempFreq = GetPerformanceCounterProperties (&StartVal, &EndVal);

    if (StartVal > EndVal) {
      DEBUG ((EFI_D_ERROR, "Error getting counter property\n"));
      return 0;
    }

    TimerFreq = (UINT32) (TempFreq & 0xFFFFFFFFULL);
    FactormS = TimerFreq / 1000;
  }

  TimerCount = GetPerformanceCounter ();
  Ms = TimerCount / FactormS;
  return Ms;
}

EFI_STATUS//sub_1842C
ReadWriteDeviceInfo (vb_device_state_op_t Mode, void *DevInfo, UINT32 Sz)
{
  EFI_STATUS Status = EFI_INVALID_PARAMETER;
  QCOM_VERIFIEDBOOT_PROTOCOL *VbIntf;

  Status = gBS->LocateProtocol (&gEfiQcomVerifiedBootProtocolGuid, NULL,
                                (VOID **)&VbIntf);
  if (Status != EFI_SUCCESS) {
    DEBUG ((EFI_D_ERROR, "Unable to locate VB protocol: %r\n", Status));
    return Status;
  }

  Status = VbIntf->VBRwDeviceState (VbIntf, Mode, DevInfo, Sz);
  if (Status != EFI_SUCCESS) {
    DEBUG ((EFI_D_ERROR, "VBRwDevice failed with: %r\n", Status));
    return Status;
  }

  return Status;
}

EFI_STATUS
GetNandMiscPartiGuid (EFI_GUID *Ptype)
{
  EFI_STATUS Status = EFI_INVALID_PARAMETER;
  EFI_NAND_PARTI_GUID_PROTOCOL *NandPartiGuid;

  Status = gBS->LocateProtocol (&gEfiNandPartiGuidProtocolGuid, NULL,
                                (VOID **)&NandPartiGuid);
  if (Status != EFI_SUCCESS) {
    DEBUG ((EFI_D_ERROR, "Unable to locate NandPartiGuid protocol: %r\n",
                                Status));
    return Status;
  }

  Status = NandPartiGuid->GenGuid (NandPartiGuid, (CONST CHAR16 *)L"misc",
                                StrLen ((CONST CHAR16 *)L"misc"), Ptype);
  if (Status != EFI_SUCCESS) {
    DEBUG ((EFI_D_ERROR, "NandPartiGuid GenGuid failed with: %r\n", Status));
    return Status;
  }

  return Status;
}

VOID StoreRootDeviceType (VOID)
{
    if (RootDeviceType == UNKNOWN) {
        RootDeviceType = CheckRootDeviceType ();
    }
}

EFI_STATUS
WriteBlockToPartitionNoFlush (EFI_BLOCK_IO_PROTOCOL *BlockIo,
                   IN EFI_HANDLE *Handle,
                   IN UINT64 Offset,
                   IN UINT64 Size,
                   IN VOID *Image)
{
  EFI_STATUS Status = EFI_SUCCESS;
  CHAR8 *ImageBuffer = NULL;
  UINT32 DivMsgBufSize;
  UINT32 WriteBlockSize;
  UINT64 WriteUnitSize = MAX_WRITE_SIZE;
  INT64 LeftSize = 0;
  UINT32 WriteSize = 0;

  if ((BlockIo == NULL) ||
    (Image == NULL)) {
    DEBUG ((EFI_D_ERROR, "NUll BlockIo or Image\n"));
    return EFI_INVALID_PARAMETER;
  }

  WriteBlockSize = BlockIo->Media->BlockSize;

  /* If the Size is not divisible by BlockSize.
    * Write the Image data to partition in twice.
    * First, write the divisible Image buffer size to partition
    * Second, malloc 1 BlockSize buffer for the rest Image data
    * and then write.
    *
    * NOTE: For NAND Targets, BlockSize would be EraseLengthGranularity
    * aligned which is available in EFI_ERASE_BLOCK_PROTOCOL.
    */
  if (RootDeviceType == NAND) {
    if (Handle == NULL) {
      DEBUG ((EFI_D_ERROR, "WriteBlockToPartition: Input Handle is Null.\n"));
      return EFI_INVALID_PARAMETER;
    }
    EFI_ERASE_BLOCK_PROTOCOL *EraseProt = NULL;
    Status = gBS->HandleProtocol (Handle, &gEfiEraseBlockProtocolGuid,
                                  (VOID **)&EraseProt);
    if (Status != EFI_SUCCESS) {
      DEBUG ((EFI_D_ERROR, "Unable to locate Erase block protocol handle:%r\n",
              Status));
      return Status;
    }

    WriteBlockSize = EraseProt->EraseLengthGranularity;
  }

  DivMsgBufSize = (Size / WriteBlockSize) * WriteBlockSize;
  WriteUnitSize = ROUND_TO_PAGE (WriteUnitSize, WriteBlockSize - 1);
  if (DivMsgBufSize) {
    /* The big image buffer may take a long flashing time which will block
       parallel usb image download. It will cause the fastboot  protocol host
       side timeout. So split the image into small writing units  to let usb
       have chance to champ in and doing work in parallel.
      */

    if (!IsFlashSplitNeeded ()) {
      WriteUnitSize = DivMsgBufSize;
    }

    LeftSize = DivMsgBufSize;
    while (LeftSize > 0) {
      WriteSize = LeftSize > WriteUnitSize? WriteUnitSize : LeftSize;
      Status = BlockIo->WriteBlocks (BlockIo,
                                     BlockIo->Media->MediaId,
                                     Offset,
                                     WriteSize,
                                     Image + DivMsgBufSize - LeftSize);

      if (Status != EFI_SUCCESS) {
        DEBUG ((EFI_D_ERROR, "Write the divisible Image failed :%r\n", Status));
        return Status;
      }
      Offset += WriteSize / BlockIo->Media->BlockSize;
      LeftSize -= WriteSize;
    }
  }

  if (Size - DivMsgBufSize > 0) {
    ImageBuffer = AllocateZeroPool (WriteBlockSize);
    if (ImageBuffer == NULL) {
      DEBUG ((EFI_D_ERROR, "Failed to allocate zero pool for ImageBuffer\n"));
      return EFI_OUT_OF_RESOURCES;
    }

    /* Read firstly to ensure the final write is not to change data
     * in this Block other than "Size - DivMsgBufSize"
     */
    Status = BlockIo->ReadBlocks (BlockIo,
                                 BlockIo->Media->MediaId,
                                 Offset,
                                 WriteBlockSize,
                                 ImageBuffer);

    if (Status != EFI_SUCCESS) {
      DEBUG ((EFI_D_ERROR, "Reading block failed :%r\n", Status));
      FreePool (ImageBuffer);
      ImageBuffer = NULL;
      return Status;
    }

    gBS->CopyMem (ImageBuffer, Image + DivMsgBufSize, Size - DivMsgBufSize);
    Status = BlockIo->WriteBlocks (BlockIo,
                                 BlockIo->Media->MediaId,
                                 Offset,
                                 WriteBlockSize,
                                 ImageBuffer);

    if (Status != EFI_SUCCESS) {
      DEBUG ((EFI_D_ERROR, "Writing single block failed :%r\n", Status));
    }

    FreePool (ImageBuffer);
    ImageBuffer = NULL;
  }

  return Status;
}

EFI_STATUS
WriteBlockToPartition (EFI_BLOCK_IO_PROTOCOL *BlockIo,
                   IN EFI_HANDLE *Handle,
                   IN UINT64 Offset,
                   IN UINT64 Size,
                   IN VOID *Image) {


  EFI_STATUS Status = EFI_SUCCESS;
  Status = WriteBlockToPartitionNoFlush (BlockIo, Handle, Offset, Size, Image);
  if (Status != EFI_SUCCESS) {
    return Status;
  }

  Status = BlockIo->FlushBlocks (BlockIo);
  if (Status != EFI_SUCCESS) {
    DEBUG ((EFI_D_ERROR, "Flushing single block failed :%r\n", Status));
  }
    return Status;
}


EFI_STATUS
WriteToPartition (EFI_GUID *Ptype, VOID *Msg, UINT32 MsgSize)
{
  EFI_STATUS Status;
  EFI_BLOCK_IO_PROTOCOL *BlkIo = NULL;
  PartiSelectFilter HandleFilter;
  HandleInfo HandleInfoList[1];
  UINT32 MaxHandles;
  UINT32 BlkIOAttrib = 0;
  EFI_HANDLE *Handle = NULL;

  if (Msg == NULL)
    return EFI_INVALID_PARAMETER;

  BlkIOAttrib = BLK_IO_SEL_PARTITIONED_GPT;
  BlkIOAttrib |= BLK_IO_SEL_MEDIA_TYPE_NON_REMOVABLE;
  BlkIOAttrib |= BLK_IO_SEL_MATCH_PARTITION_TYPE_GUID;

  HandleFilter.RootDeviceType = NULL;
  HandleFilter.PartitionType = Ptype;
  HandleFilter.VolumeName = NULL;

  MaxHandles = ARRAY_SIZE (HandleInfoList);
  Status =
      GetBlkIOHandles (BlkIOAttrib, &HandleFilter, HandleInfoList, &MaxHandles);

  if (Status == EFI_SUCCESS) {
    if (MaxHandles == 0)
      return EFI_NO_MEDIA;

    if (MaxHandles != 1) {
      // Unable to deterministically load from single partition
      DEBUG ((EFI_D_INFO, "%s: multiple partitions found.\r\n", __func__));
      return EFI_LOAD_ERROR;
    }
  } else {
    DEBUG ((EFI_D_ERROR,
            "%s: GetBlkIOHandles failed: %r\n", __func__, Status));
    return Status;
  }

  BlkIo = HandleInfoList[0].BlkIo;
  Handle = HandleInfoList[0].Handle;
  Status = WriteBlockToPartition (BlkIo, Handle, 0, MsgSize, Msg);

  if (Status != EFI_SUCCESS) {
    DEBUG ((EFI_D_ERROR,
          "Write the Msg failed :%r\n", Status));
  }

  return Status;
}

BOOLEAN IsSecureBootEnabled (VOID)
{
  EFI_STATUS Status = EFI_INVALID_PARAMETER;
  QCOM_VERIFIEDBOOT_PROTOCOL *VbIntf;
  BOOLEAN IsSecure = FALSE;

  // Initialize verified boot & Read Device Info
  Status = gBS->LocateProtocol (&gEfiQcomVerifiedBootProtocolGuid, NULL,
                                (VOID **)&VbIntf);
  if (Status != EFI_SUCCESS) {
    DEBUG ((EFI_D_ERROR, "Unable to locate VB protocol: %r\n", Status));
    return FALSE;
  }

  Status = VbIntf->VBIsDeviceSecure (VbIntf, &IsSecure);
  if (Status != EFI_SUCCESS) {
    DEBUG ((EFI_D_ERROR, "Error Reading the secure state: %r\n", Status));
    return FALSE;
  }

  return IsSecure;
}

EFI_STATUS
ResetDeviceState (VOID)
{

    return EFI_SUCCESS;

}

EFI_STATUS
ErasePartition (EFI_BLOCK_IO_PROTOCOL *BlockIo, EFI_HANDLE *Handle)
{
  EFI_STATUS Status;
  EFI_ERASE_BLOCK_TOKEN EraseToken;
  EFI_ERASE_BLOCK_PROTOCOL *EraseProt = NULL;
  UINTN PartitionSize;
  UINTN TokenIndex;

  PartitionSize = GetPartitionSize (BlockIo);
  if (!PartitionSize) {
    return EFI_BAD_BUFFER_SIZE;
  }

  Status = gBS->HandleProtocol (Handle, &gEfiEraseBlockProtocolGuid,
                                (VOID **)&EraseProt);
  if (Status != EFI_SUCCESS) {
    DEBUG ((EFI_D_ERROR, "Unable to locate Erase block protocol handle: %r\n",
            Status));
    return Status;
  }

  gBS->SetMem ((VOID *)&EraseToken, sizeof (EraseToken), 0);
  Status = EraseProt->EraseBlocks (BlockIo, BlockIo->Media->MediaId, 0,
                                   &EraseToken, PartitionSize);
  if (Status != EFI_SUCCESS) {
    DEBUG ((EFI_D_ERROR, "Unable to Erase Block: %r\n", Status));
    return Status;
  } else {
    /* handle the event */
    if (EraseToken.Event != NULL) {
      DEBUG ((EFI_D_INFO,
              "Waiting for the erase event to signal the completion\n"));
      gBS->WaitForEvent (1, &EraseToken.Event, &TokenIndex);
    }
  }

  return EFI_SUCCESS;
}

EFI_STATUS
GetBootDevice (CHAR8 *BootDevBuf, UINT32 Len)
{
  EFI_STATUS Status = EFI_SUCCESS;
  UINT64 BootDevAddr;
  UINTN DataSize = sizeof (BootDevAddr);
  CHAR8 BootDeviceType[BOOT_DEV_NAME_SIZE_MAX];

  Status =
      gRT->GetVariable ((CHAR16 *)L"BootDeviceBaseAddr", &gQcomTokenSpaceGuid,
                        NULL, &DataSize, &BootDevAddr);

  if (Status != EFI_SUCCESS) {
    DEBUG (
        (EFI_D_ERROR, "Failed to get Boot Device Base address, %r\n", Status));
    return Status;
  }

  GetRootDeviceType (BootDeviceType, BOOT_DEV_NAME_SIZE_MAX);

  if (!AsciiStrnCmp (BootDeviceType, "UFS", AsciiStrLen ("UFS"))) {
    AsciiSPrint (BootDevBuf, Len, "%x.ufshc", BootDevAddr);
  } else if (!AsciiStrnCmp (BootDeviceType, "EMMC", AsciiStrLen ("EMMC"))) {
    AsciiSPrint (BootDevBuf, Len, "%x.sdhci", BootDevAddr);
  } else if (!AsciiStrnCmp (BootDeviceType, "NVME", AsciiStrLen ("NVME"))) {
    AsciiSPrint (BootDevBuf, Len, "%x.pcie", BootDevAddr);
  } else if (!AsciiStrnCmp (BootDeviceType, "VBLK", AsciiStrLen ("VBLK"))) {
    DEBUG ((EFI_D_ERROR, "Virtio Block Device is not"
                         " supported as Boot Device\n"));
    return EFI_NOT_FOUND;
  } else {
    DEBUG ((EFI_D_ERROR, "Unknown Boot Device type detected \n"));
    return EFI_NOT_FOUND;
  }

  ToLower (BootDevBuf);

  return Status;
}

/* Returns whether MDTP is active or not,
 * or whether it should be considered active for
 * bootloader flows. */
EFI_STATUS
IsMdtpActive (BOOLEAN *MdtpActive)
{
  EFI_STATUS Status = EFI_SUCCESS;
  QCOM_MDTP_PROTOCOL *MdtpProtocol = NULL;
  MDTP_SYSTEM_STATE MdtpState = MDTP_STATE_ACTIVE;

  // Default value of MdtpActive is set to False
  *MdtpActive = FALSE;

  Status = gBS->LocateProtocol (&gQcomMdtpProtocolGuid, NULL,
                                (VOID **)&MdtpProtocol);

  if (EFI_ERROR (Status)) {
    DEBUG (
        (EFI_D_ERROR, "Failed to locate MDTP protocol, Status=%r\n", Status));
    return Status;
  }

  Status = MdtpProtocol->MdtpGetState (MdtpProtocol, &MdtpState);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "Failed to get mdtp state, Status=%r\n", Status));
    return Status;
  }

  *MdtpActive = ((MdtpState != MDTP_STATE_DISABLED) &&
                 (MdtpState != MDTP_STATE_INACTIVE));

  return Status;
}
