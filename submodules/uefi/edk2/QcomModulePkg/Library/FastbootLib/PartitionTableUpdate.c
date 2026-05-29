#define GUARD(code)                                                            \
  do {                                                                         \
    Status = (code);                                                           \
    if (Status != EFI_SUCCESS) {                                               \
      DEBUG ((EFI_D_ERROR, "Err: line:%d %a() status: %r\n", __LINE__,         \
              __FUNCTION__, Status));                                          \
      return Status;                                                           \
    }                                                                          \
  } while (0)

#define GUARD_OUT(code)                                                        \
  do {                                                                         \
    Status = (code);                                                           \
    if (Status != EFI_SUCCESS) {                                               \
      DEBUG ((EFI_D_ERROR, "Err: line:%d %a() status: %r\n", __LINE__,         \
              __FUNCTION__, Status));                                          \
      goto out;                                                                \
    }                                                                          \
  } while (0)
/*
 * Copyright (c) 2015-2021, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of The Linux Foundation nor
 *       the names of its contributors may be used to endorse or promote
 *       products derived from this software without specific prior written
 *       permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/*
 * Changes from Qualcomm Innovation Center are provided under the following license:
 *
 * Copyright (c) 2022-2025 Qualcomm Innovation Center, Inc. All rights reserved.
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
#define CHECK_ADD64(a, b) ((MAX_UINT64 - b < a) ? TRUE : FALSE)
#include "PartitionTableUpdate.h"
#include "AutoGen.h"
#ifdef AUTO_VIRT_ABL
#include <Library/Recovery.h>
#endif
#include <Library/Board.h>
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
#include <Library/LinuxLoaderLib.h>
#include <Library/UefiLib.h>
#include <Library/DebugLib.h>
#include <Library/Debug.h>
#include <Uefi.h>
#include <Uefi/UefiSpec.h>

STATIC BOOLEAN FlashingGpt;
STATIC BOOLEAN ParseSecondaryGpt;
struct StoragePartInfo Ptable[MAX_LUNS];
struct PartitionEntry PtnEntries[MAX_NUM_PARTITIONS];
STATIC UINT32 MaxLuns;
UINT32 PartitionCount;
STATIC BOOLEAN FirstBoot;
STATIC struct PartitionEntry PtnEntriesBak[MAX_NUM_PARTITIONS];

STATIC struct BootPartsLinkedList *HeadNode;
EFI_STATUS
GetActiveSlot (Slot *ActiveSlot);
STATIC EFI_STATUS
GetAtomicABActiveSlot (Slot *ActiveSlot);

Slot GetCurrentSlotSuffix (VOID)
{
  Slot CurrentSlot = {{0}};
  BOOLEAN IsMultiSlot = PartitionHasMultiSlot ((CONST CHAR16 *)L"boot");

  if (IsMultiSlot == FALSE) {
    return CurrentSlot;
  }

  GetActiveSlot (&CurrentSlot);
  return CurrentSlot;
}

UINT32 GetMaxLuns (VOID)
{
  return MaxLuns;
}

UINT32
GetPartitionLunFromIndex (UINT32 Index)
{
  return PtnEntries[Index].lun;
}

VOID
GetPartitionCount (UINT32 *Val)
{
  *Val = PartitionCount;
  return;
}

INT32
GetPartitionIdxInLun (CHAR16 *Pname, UINT32 Lun)
{
  UINT32 n;
  UINT32 RelativeIndex = 0;

  for (n = 0; n < PartitionCount; n++) {
    if (Lun == PtnEntries[n].lun) {
      if (!StrnCmp (Pname, PtnEntries[n].PartEntry.PartitionName,
                    ARRAY_SIZE (PtnEntries[n].PartEntry.PartitionName))) {
        return RelativeIndex;
      }
      RelativeIndex++;
    }
  }
  return INVALID_PTN;
}

VOID UpdatePartitionEntries (VOID)
{
  UINT32 i;
  UINT32 j;
  UINT32 Index = 0;
  EFI_STATUS Status;
  EFI_PARTITION_ENTRY *PartEntry;

  PartitionCount = 0;
  /*Nullify the PtnEntries array before using it*/
  gBS->SetMem ((VOID *)PtnEntries,
               (sizeof (PtnEntries[0]) * MAX_NUM_PARTITIONS), 0);

  for (i = 0; i < MaxLuns; i++) {
    for (j = 0; (j < Ptable[i].MaxHandles) && (Index < MAX_NUM_PARTITIONS);
         j++, Index++) {
      Status =
          gBS->HandleProtocol (Ptable[i].HandleInfoList[j].Handle,
                               &gEfiPartitionRecordGuid, (VOID **)&PartEntry);
      PartitionCount++;
      if (EFI_ERROR (Status)) {
        DEBUG ((EFI_D_VERBOSE, "Selected Lun : %d, handle: %d does not have "
                               "partition record, ignore\n",
                i, j));
        PtnEntries[Index].lun = i;
        continue;
      }

      gBS->CopyMem ((&PtnEntries[Index]), PartEntry, sizeof (PartEntry[0]));
      DEBUG ((EFI_D_VERBOSE, "[%a] Partition name is %s\n",
                    __func__, PartEntry->PartitionName));
      PtnEntries[Index].lun = i;
    }
  }
  /* Back up the ptn entries */
  gBS->CopyMem (PtnEntriesBak, PtnEntries, sizeof (PtnEntries));
}

INT32
GetPartitionIndex (CHAR16 *Pname)
{
  INT32 i;

  for (i = 0; i < PartitionCount; i++) {
    if (!StrnCmp (PtnEntries[i].PartEntry.PartitionName, Pname,
                  ARRAY_SIZE (PtnEntries[i].PartEntry.PartitionName))) {
      return i;
    }
  }

  return INVALID_PTN;
}

STATIC EFI_STATUS
GetStorageHandle (INT32 Lun, HandleInfo *BlockIoHandle, UINT32 *MaxHandles)
{
  EFI_STATUS Status = EFI_INVALID_PARAMETER;
  UINT32 Attribs = 0;
  PartiSelectFilter HandleFilter;
  // UFS LUN GUIDs
  EFI_GUID LunGuids[] = {
      gEfiUfsLU0Guid, gEfiUfsLU1Guid, gEfiUfsLU2Guid, gEfiUfsLU3Guid,
      gEfiUfsLU4Guid, gEfiUfsLU5Guid, gEfiUfsLU6Guid, gEfiUfsLU7Guid,
  };

  Attribs |= BLK_IO_SEL_SELECT_ROOT_DEVICE_ONLY;
  HandleFilter.PartitionType = NULL;
  HandleFilter.VolumeName = NULL;

  if (Lun == NO_LUN) {
    HandleFilter.RootDeviceType = &gEfiEmmcUserPartitionGuid;
    Status =
        GetBlkIOHandles (Attribs, &HandleFilter, BlockIoHandle, MaxHandles);
    if (EFI_ERROR (Status)) {
      HandleFilter.RootDeviceType = &gEfiNvme0Guid;
      Status =
        GetBlkIOHandles (Attribs, &HandleFilter, BlockIoHandle, MaxHandles);
      if (EFI_ERROR (Status)) {
        DEBUG ((EFI_D_ERROR,
               "Error getting block IO handle for Emmc and Nvme\n"));
        return Status;
      }
    }
  } else {
    HandleFilter.RootDeviceType = &LunGuids[Lun];
    Status =
        GetBlkIOHandles (Attribs, &HandleFilter, BlockIoHandle, MaxHandles);
    if (EFI_ERROR (Status)) {
      DEBUG ((EFI_D_ERROR, "Error getting block IO handle for Lun:%x\n", Lun));
      return Status;
    }
  }

  return Status;
}

STATIC BOOLEAN IsUpdatePartitionAttributes ()
{
  if (CompareMem (PtnEntries, PtnEntriesBak, sizeof (PtnEntries))) {
    return TRUE;
  }
  return FALSE;
}

UINT64 GetPartitionSize (EFI_BLOCK_IO_PROTOCOL *BlockIo)
{
  UINT64 PartitionSize;

  if (!BlockIo) {
    DEBUG ((EFI_D_ERROR, "Invalid parameter, pleae check BlockIo info!!!\n"));
    return 0;
  }

  if (CHECK_ADD64 (BlockIo->Media->LastBlock, 1)) {
    DEBUG ((EFI_D_ERROR, "Integer overflow while adding LastBlock and 1\n"));
    return 0;
  }

  if ((MAX_UINT64 / (BlockIo->Media->LastBlock + 1)) <
    (UINT64)BlockIo->Media->BlockSize) {
    DEBUG ((EFI_D_ERROR,
     "Integer overflow while multiplying LastBlock and BlockSize\n"));
    return 0;
  }

  PartitionSize = (BlockIo->Media->LastBlock + 1) * BlockIo->Media->BlockSize;
  return  PartitionSize;
}

VOID UpdatePartitionAttributes (UINT32 UpdateType)
{
  UINT32 BlkSz;
  UINT8 *GptHdr = NULL;
  UINT8 *GptHdrPtr = NULL;
  UINTN MaxGptPartEntrySzBytes;
  UINT32 Offset;
  UINT32 MaxPtnCount = 0;
  UINT32 PtnEntrySz = 0;
  UINT32 i = 0;
  UINT8 *PtnEntriesPtr;
  UINT8 *Ptn_Entries;
  UINT32 CrcVal = 0;
  UINT32 Iter;
  UINT32 HdrSz = GPT_HEADER_SIZE;
  UINT64 DeviceDensity;
  UINT64 CardSizeSec;
  EFI_STATUS Status;
  INT32 Lun;
  EFI_BLOCK_IO_PROTOCOL *BlockIo = NULL;
  HandleInfo BlockIoHandle[MAX_HANDLEINF_LST_SIZE];
  UINT32 MaxHandles = MAX_HANDLEINF_LST_SIZE;
  CHAR8 BootDeviceType[BOOT_DEV_NAME_SIZE_MAX];
  UINT32 PartEntriesblocks = 0;
  BOOLEAN SkipUpdation;
  UINT64 Attr;
  struct PartitionEntry *InMemPtnEnt;

  /* The PtnEntries is the same as PtnEntriesBak by default
   *  It needs to update attributes or GUID when PtnEntries is changed
   */
  if (!IsUpdatePartitionAttributes ()) {
    return;
  }

  GetRootDeviceType (BootDeviceType, BOOT_DEV_NAME_SIZE_MAX);
  for (Lun = 0; Lun < MaxLuns; Lun++) {

    if (!AsciiStrnCmp (BootDeviceType, "EMMC", AsciiStrLen ("EMMC")) ||
        !AsciiStrnCmp (BootDeviceType, "NVME", AsciiStrLen ("NVME"))) {
      Status = GetStorageHandle (NO_LUN, BlockIoHandle, &MaxHandles);
    } else if (!AsciiStrnCmp (BootDeviceType, "UFS", AsciiStrLen ("UFS"))) {
      Status = GetStorageHandle (Lun, BlockIoHandle, &MaxHandles);
    } else if (!AsciiStrnCmp (BootDeviceType, "NAND", AsciiStrLen ("NAND"))) {
      DEBUG ((EFI_D_ERROR, "Skip setting if boot device type is NAND\n"));
      return;
    } else {
      DEBUG ((EFI_D_ERROR, "Unsupported  boot device type\n"));
      return;
    }

    if (Status != EFI_SUCCESS) {
      DEBUG ((EFI_D_ERROR,
              "Failed to get BlkIo for device. MaxHandles:%d - %r\n",
              MaxHandles, Status));
      return;
    }
    if (MaxHandles != 1) {
      DEBUG ((EFI_D_VERBOSE,
              "Failed to get the BlockIo for device. MaxHandle:%d, %r\n",
              MaxHandles, Status));
      continue;
    }

    BlockIo = BlockIoHandle[0].BlkIo;
    DeviceDensity = GetPartitionSize (BlockIo);
    if (!DeviceDensity) {
      return;
    }
    BlkSz = BlockIo->Media->BlockSize;
    PartEntriesblocks = MAX_PARTITION_ENTRIES_SZ / BlkSz;
    MaxGptPartEntrySzBytes = (GPT_HDR_BLOCKS + PartEntriesblocks) * BlkSz;
    CardSizeSec = (DeviceDensity) / BlkSz;
    Offset = PRIMARY_HDR_LBA;
    GptHdr = AllocateZeroPool (MaxGptPartEntrySzBytes);
    if (!GptHdr) {
      DEBUG ((EFI_D_ERROR, "Unable to Allocate Memory for GptHdr \n"));
      return;
    }

    GptHdrPtr = GptHdr;

    /* This loop iterates twice to update both primary and backup Gpt*/
    for (Iter = 0; Iter < 2;
         Iter++, (Offset = CardSizeSec - MaxGptPartEntrySzBytes / BlkSz)) {
      SkipUpdation = TRUE;
      Status = BlockIo->ReadBlocks (BlockIo, BlockIo->Media->MediaId, Offset,
                                    MaxGptPartEntrySzBytes, GptHdr);

      if (EFI_ERROR (Status)) {
        DEBUG ((EFI_D_ERROR, "Unable to read the media \n"));
        goto Exit;
      }

      if (Iter == 0x1) {
        /* This is the back up GPT */
        Ptn_Entries = GptHdr;
        GptHdr = GptHdr + ((PartEntriesblocks)*BlkSz);
      } else
        /* otherwise we are at the primary gpt */
        Ptn_Entries = GptHdr + BlkSz;

      PtnEntriesPtr = Ptn_Entries;

      for (i = 0; i < PartitionCount; i++) {
        InMemPtnEnt = (struct PartitionEntry *)PtnEntriesPtr;
        /*If GUID is not present, then it is BlkIo Handle of the Lun. Skip*/
        if (!(PtnEntries[i].PartEntry.PartitionTypeGUID.Data1)) {
          DEBUG ((EFI_D_VERBOSE, " Skipping Lun:%d, i=%d\n", Lun, i));
          continue;
        }

        if (!AsciiStrnCmp (BootDeviceType, "UFS", AsciiStrLen ("UFS"))) {
          /* Partition table is populated with entries from lun 0 to max lun.
           * break out of the loop once we see the partition lun is > current
           * lun */
          if (PtnEntries[i].lun > Lun)
            break;
          /* Find the entry where the partition table for 'lun' starts and then
           * update the attributes */
          if (PtnEntries[i].lun != Lun)
            continue;
        }
        Attr = GET_LLWORD_FROM_BYTE (&PtnEntriesPtr[ATTRIBUTE_FLAG_OFFSET]);
        if (UpdateType & PARTITION_GUID_MASK) {
          if (CompareMem (&InMemPtnEnt->PartEntry.PartitionTypeGUID,
              &PtnEntries[i].PartEntry.PartitionTypeGUID,
              sizeof (EFI_GUID))) {
            /* Update the partition GUID values */
            gBS->CopyMem ((VOID *)PtnEntriesPtr,
                          (VOID *)&PtnEntries[i].PartEntry.PartitionTypeGUID,
                          GUID_SIZE);
            /* Update the PtnEntriesBak for next comparison */
            gBS->CopyMem (
                        (VOID *)&PtnEntriesBak[i].PartEntry.PartitionTypeGUID,
                        (VOID *)&PtnEntries[i].PartEntry.PartitionTypeGUID,
                        GUID_SIZE);
            SkipUpdation = FALSE;
          }
        }

        if (UpdateType & PARTITION_ATTRIBUTES_MASK) {
          /*  If GUID is not present, then it is back up GPT, update it
           *  If GUID is present,  and the GUID is matched, update it
           */
          if (!(InMemPtnEnt->PartEntry.PartitionTypeGUID.Data1) ||
              !CompareMem (&InMemPtnEnt->PartEntry.PartitionTypeGUID,
              &PtnEntries[i].PartEntry.PartitionTypeGUID,
              sizeof (EFI_GUID))) {
            if (Attr != PtnEntries[i].PartEntry.Attributes) {
              /* Update the partition attributes */
              PUT_LONG_LONG (&PtnEntriesPtr[ATTRIBUTE_FLAG_OFFSET],
                              PtnEntries[i].PartEntry.Attributes);
              /* Update the PtnEntriesBak for next comparison */
              PtnEntriesBak[i].PartEntry.Attributes =
                              PtnEntries[i].PartEntry.Attributes;
              SkipUpdation = FALSE;
            }
          } else {
            if (InMemPtnEnt->PartEntry.PartitionTypeGUID.Data1) {
              DEBUG ((EFI_D_ERROR,
                    "Error in GPT header, GUID is not match!\n"));
              continue;
            }
          }
        }

        /* point to the next partition entry */
        PtnEntriesPtr += PARTITION_ENTRY_SIZE;
      }

      if (SkipUpdation)
        continue;

      MaxPtnCount = GET_LWORD_FROM_BYTE (&GptHdr[PARTITION_COUNT_OFFSET]);
      PtnEntrySz = GET_LWORD_FROM_BYTE (&GptHdr[PENTRY_SIZE_OFFSET]);

      if (((UINT64) (MaxPtnCount)*PtnEntrySz) > MAX_PARTITION_ENTRIES_SZ) {
        DEBUG ((EFI_D_ERROR,
                "Invalid GPT header fields MaxPtnCount = %x, PtnEntrySz = %x\n",
                MaxPtnCount, PtnEntrySz));
        goto Exit;
      }

      Status = gBS->CalculateCrc32 (Ptn_Entries, ((MaxPtnCount) * (PtnEntrySz)),
                                    &CrcVal);
      if (Status != EFI_SUCCESS) {
        DEBUG ((EFI_D_ERROR, "Error Calculating CRC32 on the Gpt header: %x\n",
                Status));
        goto Exit;
      }

      PUT_LONG (&GptHdr[PARTITION_CRC_OFFSET], CrcVal);

      /*Write CRC to 0 before we calculate the crc of the GPT header*/
      CrcVal = 0;
      PUT_LONG (&GptHdr[HEADER_CRC_OFFSET], CrcVal);

      Status = gBS->CalculateCrc32 (GptHdr, HdrSz, &CrcVal);
      if (Status != EFI_SUCCESS) {
        DEBUG ((EFI_D_ERROR, "Error Calculating CRC32 on the Gpt header: %x\n",
                Status));
        goto Exit;
      }

      PUT_LONG (&GptHdr[HEADER_CRC_OFFSET], CrcVal);

      if (Iter == 0x1)
        /* Write the backup GPT header, which is at an offset of CardSizeSec -
         * MaxGptPartEntrySzBytes/BlkSz in blocks*/
        Status =
            BlockIo->WriteBlocks (BlockIo, BlockIo->Media->MediaId, Offset,
                                  MaxGptPartEntrySzBytes, (VOID *)Ptn_Entries);
      else
        /* Write the primary GPT header, which is at an offset of BlkSz */
        Status = BlockIo->WriteBlocks (BlockIo, BlockIo->Media->MediaId, Offset,
                                       MaxGptPartEntrySzBytes, (VOID *)GptHdr);

      if (EFI_ERROR (Status)) {
        DEBUG ((EFI_D_ERROR, "Error writing primary GPT header: %r\n", Status));
        goto Exit;
      }
    }

    Status = BlockIo->FlushBlocks (BlockIo);
    if (EFI_ERROR (Status)) {
      DEBUG ((EFI_D_ERROR, "Error flushing blocks primary GPT header: %r\n",
              Status));
      goto Exit;
    }
    FreePool (GptHdrPtr);
    GptHdrPtr = NULL;
  }

Exit:
  if (GptHdrPtr) {
    FreePool (GptHdrPtr);
    GptHdrPtr = NULL;
  }
}

STATIC BOOLEAN IsSubStrPresentAtLast (
  IN      CONST CHAR16              *String,
  IN      CONST CHAR16              *SearchString
  )
{
  UINT32 SrcLen = StrLen (String);
  UINT32 SearchStrLen = StrLen (SearchString);

  if (SrcLen < SearchStrLen) {
    return FALSE;
  }

  if (!StrnCmp (String + SrcLen - SearchStrLen, SearchString, SearchStrLen)) {
    return TRUE;
  } else {
    return FALSE;
  }
}

STATIC VOID
MarkPtnActive (CHAR16 *ActiveSlot)
{
  UINT32 i;
  for (i = 0; i < PartitionCount; i++) {
    /* Mark all the slots with current ActiveSlot as active */
    if (IsSubStrPresentAtLast (PtnEntries[i].PartEntry.PartitionName,
         ActiveSlot)) {
      PtnEntries[i].PartEntry.Attributes |= PART_ATT_ACTIVE_VAL;
    } else {
      PtnEntries[i].PartEntry.Attributes &= ~PART_ATT_ACTIVE_VAL;
    }
  }

  /* Update the partition table */
  UpdatePartitionAttributes (PARTITION_ATTRIBUTES);
}

STATIC VOID
SwapPtnGuid (EFI_PARTITION_ENTRY *p1, EFI_PARTITION_ENTRY *p2)
{
  EFI_GUID Temp;

  if (p1 == NULL || p2 == NULL)
    return;
  gBS->CopyMem ((VOID *)&Temp, (VOID *)&p1->PartitionTypeGUID,
                sizeof (EFI_GUID));
  gBS->CopyMem ((VOID *)&p1->PartitionTypeGUID, (VOID *)&p2->PartitionTypeGUID,
                sizeof (EFI_GUID));
  gBS->CopyMem ((VOID *)&p2->PartitionTypeGUID, (VOID *)&Temp,
                sizeof (EFI_GUID));
}

STATIC EFI_STATUS GetMultiSlotPartsList (VOID)
{
  UINT32 i = 0;
  UINT32 j = 0;
  UINT32 Len = 0;
  UINT32 PtnLen = 0;
  CHAR16 *SearchString = NULL;
  struct BootPartsLinkedList *TempNode = NULL;

  for (i = 0; i < PartitionCount; i++) {
    SearchString = PtnEntries[i].PartEntry.PartitionName;
    if (!SearchString[0])
      continue;

    for (j = i + 1; j < PartitionCount; j++) {
      if (!PtnEntries[j].PartEntry.PartitionName[0])
        continue;
      Len = StrLen (SearchString);
      PtnLen = StrLen (PtnEntries[j].PartEntry.PartitionName);

      /*Need to compare till "boot_"a hence skip last Char from StrLen value*/
      if ((PtnLen == Len) &&
          !StrnCmp (PtnEntries[j].PartEntry.PartitionName,
          SearchString, Len - 1) &&
          (StrStr (SearchString, (CONST CHAR16 *)L"_a") ||
          StrStr (SearchString, (CONST CHAR16 *)L"_b"))) {
        TempNode = AllocateZeroPool (sizeof (struct BootPartsLinkedList));
        if (TempNode) {
          /*Skip _a/_b from partition name*/
          StrnCpyS (TempNode->PartName, sizeof (TempNode->PartName),
                    SearchString, Len - 2);
          TempNode->Next = HeadNode;
          HeadNode = TempNode;
        } else {
          DEBUG ((EFI_D_ERROR,
                  "Unable to Allocate Memory for MultiSlot Partition list\n"));
          return EFI_OUT_OF_RESOURCES;
        }
        break;
      }
    }
  }
  return EFI_SUCCESS;
}

STATIC VOID
SwitchPtnSlots (CONST CHAR16 *SetActive)
{
  UINT32 i;
  struct PartitionEntry *PtnCurrent = NULL;
  struct PartitionEntry *PtnNew = NULL;
  CHAR16 CurSlot[BOOT_PART_SIZE];
  CHAR16 NewSlot[BOOT_PART_SIZE];
  CHAR16 SetInactive[MAX_SLOT_SUFFIX_SZ];
  UINT32 UfsBootLun = 0;
  BOOLEAN UfsGet = TRUE;
  BOOLEAN UfsSet = FALSE;
  struct BootPartsLinkedList *TempNode = NULL;
  EFI_STATUS Status;
  CHAR8 BootDeviceType[BOOT_DEV_NAME_SIZE_MAX];

  /* Create the partition name string for active and non active slots*/
  if (!StrnCmp (SetActive, (CONST CHAR16 *)L"_a",
                StrLen ((CONST CHAR16 *)L"_a")))
    StrnCpyS (SetInactive, MAX_SLOT_SUFFIX_SZ, (CONST CHAR16 *)L"_b",
              StrLen ((CONST CHAR16 *)L"_b"));
  else
    StrnCpyS (SetInactive, MAX_SLOT_SUFFIX_SZ, (CONST CHAR16 *)L"_a",
              StrLen ((CONST CHAR16 *)L"_a"));

  if (!HeadNode) {
    Status = GetMultiSlotPartsList ();
    if (Status != EFI_SUCCESS) {
      DEBUG ((EFI_D_INFO, "Unable to get GetMultiSlotPartsList\n"));
      return;
    }
  }

  for (TempNode = HeadNode; TempNode; TempNode = TempNode->Next) {
    gBS->SetMem (CurSlot, BOOT_PART_SIZE, 0);
    gBS->SetMem (NewSlot, BOOT_PART_SIZE, 0);

    StrnCpyS (CurSlot, BOOT_PART_SIZE, TempNode->PartName,
              StrLen (TempNode->PartName));
    StrnCatS (CurSlot, BOOT_PART_SIZE, SetInactive, StrLen (SetInactive));

    StrnCpyS (NewSlot, BOOT_PART_SIZE, TempNode->PartName,
              StrLen (TempNode->PartName));
    StrnCatS (NewSlot, BOOT_PART_SIZE, SetActive, StrLen (SetActive));

    /* Find the pointer to partition table entry for active and non-active
     * slots*/
    for (i = 0; i < PartitionCount; i++) {
      if (!StrnCmp (PtnEntries[i].PartEntry.PartitionName, CurSlot,
                    StrLen (CurSlot))) {
        PtnCurrent = &PtnEntries[i];
      } else if (!StrnCmp (PtnEntries[i].PartEntry.PartitionName, NewSlot,
                           StrLen (NewSlot))) {
        PtnNew = &PtnEntries[i];
      }
    }
    /* Swap the guids for the slots */
    SwapPtnGuid (&PtnCurrent->PartEntry, &PtnNew->PartEntry);
    PtnCurrent = PtnNew = NULL;
  }

  GetRootDeviceType (BootDeviceType, BOOT_DEV_NAME_SIZE_MAX);
  if (!AsciiStrnCmp (BootDeviceType, "UFS", AsciiStrLen ("UFS"))) {
    UfsGetSetBootLun (&UfsBootLun, UfsGet);
    // Special case for XBL is to change the bootlun instead of swapping the
    // guid
    if (UfsBootLun == 0x1 &&
        !StrnCmp (SetActive, (CONST CHAR16 *)L"_b",
                  StrLen ((CONST CHAR16 *)L"_b"))) {
      DEBUG ((EFI_D_INFO, "Switching the boot lun from 1 to 2\n"));
      UfsBootLun = 0x2;
    } else if (UfsBootLun == 0x2 &&
               !StrnCmp (SetActive, (CONST CHAR16 *)L"_a",
                         StrLen ((CONST CHAR16 *)L"_a"))) {
      DEBUG ((EFI_D_INFO, "Switching the boot lun from 2 to 1\n"));
      UfsBootLun = 0x1;
    }
    UfsGetSetBootLun (&UfsBootLun, UfsSet);
  }

  UpdatePartitionAttributes (PARTITION_GUID);
}

EFI_STATUS
EnumeratePartitions (VOID)
{
  EFI_STATUS Status;
  PartiSelectFilter HandleFilter;
  UINT32 Attribs = 0;
  UINT32 i;
  // UFS LUN GUIDs
  EFI_GUID LunGuids[] = {
      gEfiUfsLU0Guid, gEfiUfsLU1Guid, gEfiUfsLU2Guid, gEfiUfsLU3Guid,
      gEfiUfsLU4Guid, gEfiUfsLU5Guid, gEfiUfsLU6Guid, gEfiUfsLU7Guid,
  };

  gBS->SetMem ((VOID *)Ptable, (sizeof (struct StoragePartInfo) * MAX_LUNS), 0);

  /* By default look for emmc partitions if not found look for UFS */
  Attribs |= BLK_IO_SEL_MATCH_ROOT_DEVICE;

  Ptable[0].MaxHandles = ARRAY_SIZE (Ptable[0].HandleInfoList);
  HandleFilter.PartitionType = NULL;
  HandleFilter.VolumeName = NULL;
#ifdef AUTO_VIRT_ABL
  HandleFilter.RootDeviceType = &gVirtioMmioTransportGuid;
#else
  HandleFilter.RootDeviceType = &gEfiNandUserPartitionGuid;
#endif

  Status =
      GetBlkIOHandles (Attribs, &HandleFilter, &Ptable[0].HandleInfoList[0],
                       &Ptable[0].MaxHandles);
  /* For Emmc/NAND devices the Lun concept does not exist, we will always one
   * lun and the lun number is '0'
   * to have the partition selection implementation same acros
   */
  if (Status == EFI_SUCCESS &&
      Ptable[0].MaxHandles > 0) {
    MaxLuns = 1;
    return Status;
  }

  Ptable[0].MaxHandles = ARRAY_SIZE (Ptable[0].HandleInfoList);
  HandleFilter.PartitionType = NULL;
  HandleFilter.VolumeName = NULL;
  HandleFilter.RootDeviceType = &gEfiEmmcUserPartitionGuid;

  Status =
      GetBlkIOHandles (Attribs, &HandleFilter, &Ptable[0].HandleInfoList[0],
                       &Ptable[0].MaxHandles);
  if (Status == EFI_SUCCESS &&
      Ptable[0].MaxHandles > 0) {
    MaxLuns = 1;
    return Status;
  }

  Ptable[0].MaxHandles = ARRAY_SIZE (Ptable[0].HandleInfoList);
  HandleFilter.PartitionType = NULL;
  HandleFilter.VolumeName = NULL;
  HandleFilter.RootDeviceType = &gEfiNvme0Guid;

  Status =
      GetBlkIOHandles (Attribs, &HandleFilter, &Ptable[0].HandleInfoList[0],
                       &Ptable[0].MaxHandles);
  if (Status == EFI_SUCCESS &&
      Ptable[0].MaxHandles > 0) {
    MaxLuns = 1;
    return Status;
  }
  /* If the media is not emmc/nvme/spinor then look for UFS */
  else if (EFI_ERROR (Status) || Ptable[0].MaxHandles == 0) {
    /* By default max 8 luns are supported but HW could be configured to use
     * only few of them or all of them
     * Based on the information read update the MaxLuns to reflect the max
     * supported luns */
    for (i = 0; i < MAX_LUNS; i++) {
      Ptable[i].MaxHandles = ARRAY_SIZE (Ptable[i].HandleInfoList);
      HandleFilter.PartitionType = NULL;
      HandleFilter.VolumeName = NULL;
      HandleFilter.RootDeviceType = &LunGuids[i];

      Status =
          GetBlkIOHandles (Attribs, &HandleFilter, &Ptable[i].HandleInfoList[0],
                           &Ptable[i].MaxHandles);
      /* If we fail to get block for a lun that means the lun is not configured
       * and unsed, ignore the error
       * and continue with the next Lun */
      if (EFI_ERROR (Status)) {
        DEBUG ((EFI_D_ERROR,
                "Error getting block IO handle for %d lun, Lun may be unused\n",
                i));
        continue;
      }
    }
    MaxLuns = i;
  } else {
    DEBUG ((EFI_D_ERROR, "Error populating block IO handles\n"));
    return EFI_NOT_FOUND;
  }

  return Status;
}

/*Function to provide has-slot info
 *Pname: the partition name
 *return: 1 or 0.
 */
BOOLEAN
PartitionHasMultiSlot (CONST CHAR16 *Pname)
{
  UINT32 i;
  UINT32 SlotCount = 0;
  UINT32 Len = StrLen (Pname);

  for (i = 0; i < PartitionCount; i++) {
    if (!(StrnCmp (PtnEntries[i].PartEntry.PartitionName, Pname, Len))) {
      if (PtnEntries[i].PartEntry.PartitionName[Len] == L'_' &&
          (PtnEntries[i].PartEntry.PartitionName[Len + 1] == L'a' ||
           PtnEntries[i].PartEntry.PartitionName[Len + 1] == L'b'))
        if (++SlotCount > MIN_SLOTS) {
          return TRUE;
        }
    }
  }
  return FALSE;
}

VOID FindPtnActiveSlot (VOID)
{
  Slot ActiveSlot = {{0}};

  GetActiveSlot (&ActiveSlot);
  return;
}

STATIC UINT32
PartitionVerifyMbrSignature (UINT32 Sz, UINT8 *Gpt)
{
  if ((MBR_SIGNATURE + 1) >= Sz) {
    DEBUG ((EFI_D_ERROR, "Gpt Image size is invalid\n"));
    return FAILURE;
  }

  /* Check for the signature */
  if ((Gpt[MBR_SIGNATURE] != MBR_SIGNATURE_BYTE_0) ||
      (Gpt[MBR_SIGNATURE + 1] != MBR_SIGNATURE_BYTE_1)) {
    DEBUG ((EFI_D_ERROR, "MBR signature do not match\n"));
    return FAILURE;
  }
  return SUCCESS;
}

UINT32
PartitionVerifyMibibImage (UINT8 *Image)
{

  /* Check for the MIBIB Magic */
  if ((((UINT32 *)Image)[0] != MIBIB_MAGIC1) ||
      (((UINT32 *)Image)[1] != MIBIB_MAGIC2)) {
    DEBUG ((EFI_D_ERROR, "Mibib Magic do not match\n"));
    return FAILURE;
  }
  return SUCCESS;
}

STATIC UINT32
MbrGetPartitionType (UINT32 Sz, UINT8 *Gpt, UINT32 *Ptype)
{
  UINT32 PtypeOffset = MBR_PARTITION_RECORD + OS_TYPE;

  if (Sz <= PtypeOffset) {
    DEBUG ((EFI_D_ERROR,
            "Input gpt image does not have gpt partition record data\n"));
    return FAILURE;
  }

  *Ptype = Gpt[PtypeOffset];

  return SUCCESS;
}

STATIC UINT32
PartitionGetType (UINT32 Sz, UINT8 *Gpt, UINT32 *Ptype)
{
  UINT32 Ret;

  Ret = PartitionVerifyMbrSignature (Sz, Gpt);
  if (!Ret) {
    /* MBR signature match, this coulb be MBR, MBR + EBR or GPT */
    Ret = MbrGetPartitionType (Sz, Gpt, Ptype);
    if (!Ret) {
      if (*Ptype == GPT_PROTECTIVE)
        *Ptype = PARTITION_TYPE_GPT;
      else
        *Ptype = PARTITION_TYPE_MBR;
    }
  } else {
    /* This could be GPT back up */
    *Ptype = PARTITION_TYPE_GPT_BACKUP;
    Ret = SUCCESS;
  }

  return Ret;
}

STATIC UINT32
ParseGptHeader (struct GptHeaderData *GptHeader,
                UINT8 *GptBuffer,
                UINT64 DeviceDensity,
                UINT32 BlkSz)
{
  UINT32 CrcOrig;
  UINT32 CrcVal;
  UINT32 CurrentLba;
  EFI_STATUS Status;

  if (((UINT32 *)GptBuffer)[0] != GPT_SIGNATURE_2 ||
      ((UINT32 *)GptBuffer)[1] != GPT_SIGNATURE_1) {
    DEBUG ((EFI_D_ERROR, "Gpt signature is not correct\n"));
    return FAILURE;
  }

  GptHeader->HeaderSz = GET_LWORD_FROM_BYTE (&GptBuffer[HEADER_SIZE_OFFSET]);
  /* Validate the header size */
  if (GptHeader->HeaderSz < GPT_HEADER_SIZE) {
    DEBUG ((EFI_D_ERROR, "GPT Header size is too small: %u\n",
            GptHeader->HeaderSz));
    return FAILURE;
  }

  if (GptHeader->HeaderSz > BlkSz) {
    DEBUG ((EFI_D_ERROR, "GPT Header is too large: %u\n", GptHeader->HeaderSz));
    return FAILURE;
  }

  CrcOrig = GET_LWORD_FROM_BYTE (&GptBuffer[HEADER_CRC_OFFSET]);
  /* CRC value is computed by setting this field to 0, and computing the 32-bit
   * CRC for HeaderSize bytes */
  CrcVal = 0;
  PUT_LONG (&GptBuffer[HEADER_CRC_OFFSET], CrcVal);

  Status = gBS->CalculateCrc32 (GptBuffer, GptHeader->HeaderSz, &CrcVal);
  if (Status != EFI_SUCCESS) {
    DEBUG ((EFI_D_ERROR, "Error Calculating CRC32 on the Gpt header: %x\n",
            Status));
    return FAILURE;
  }

  if (CrcVal != CrcOrig) {
    DEBUG ((EFI_D_ERROR, "Header CRC mismatch CrcVal = %u and CrcOrig = %u\n",
            CrcVal, CrcOrig));
    return FAILURE;
  } else
    PUT_LONG (&GptBuffer[HEADER_CRC_OFFSET], CrcVal);

  CurrentLba = GET_LLWORD_FROM_BYTE (&GptBuffer[PRIMARY_HEADER_OFFSET]);
  GptHeader->FirstUsableLba =
      GET_LLWORD_FROM_BYTE (&GptBuffer[FIRST_USABLE_LBA_OFFSET]);
  GptHeader->MaxPtCnt =
      GET_LWORD_FROM_BYTE (&GptBuffer[PARTITION_COUNT_OFFSET]);
  GptHeader->PartEntrySz = GET_LWORD_FROM_BYTE (&GptBuffer[PENTRY_SIZE_OFFSET]);
  GptHeader->LastUsableLba =
      GET_LLWORD_FROM_BYTE (&GptBuffer[LAST_USABLE_LBA_OFFSET]);
  if (!ParseSecondaryGpt) {
    if (CurrentLba != GPT_LBA) {
      DEBUG ((EFI_D_ERROR, "GPT first usable LBA mismatch\n"));
      return FAILURE;
    }
  }

  /* Check for first lba should be within valid range */
  if (GptHeader->FirstUsableLba > (DeviceDensity / BlkSz)) {
    DEBUG ((EFI_D_ERROR, "FirstUsableLba: %u out of Device capacity\n",
            GptHeader->FirstUsableLba));
    return FAILURE;
  }

  /* Check for Last lba should be within valid range */
  if (GptHeader->LastUsableLba > (DeviceDensity / BlkSz)) {
    DEBUG ((EFI_D_ERROR, "LastUsableLba: %u out of device capacity\n",
            GptHeader->LastUsableLba));
    return FAILURE;
  }

  if (GptHeader->PartEntrySz != GPT_PART_ENTRY_SIZE) {
    DEBUG ((EFI_D_ERROR, "Invalid partition entry size: %u\n",
            GptHeader->PartEntrySz));
    return FAILURE;
  }

  if (GptHeader->MaxPtCnt >
      (MIN_PARTITION_ARRAY_SIZE / (GptHeader->PartEntrySz))) {
    DEBUG ((EFI_D_ERROR, "Invalid Max Partition Count: %u\n",
            GptHeader->MaxPtCnt));
    return FAILURE;
  }

  /* Todo: Check CRC during reading partition table*/
  if (!FlashingGpt) {
  }

  return SUCCESS;
}

STATIC UINT32
PatchGpt (UINT8 *Gpt,
          UINT64 DeviceDensity,
          UINT32 PartEntryArrSz,
          struct GptHeaderData *GptHeader,
          UINT32 BlkSz)
{
  UINT8 *PrimaryGptHeader;
  UINT8 *SecondaryGptHeader;
  //define as 64 bit unsigned int
  UINT64 *LastPartitionEntry;
  UINT64 NumSectors;
  UINT32 Offset;
  UINT32 TotalPart = 0;
  UINT32 LastPartOffset;
  UINT8 *PartitionEntryArrStart;
  UINT32 CrcVal;
  EFI_STATUS Status;
  UINT32 PtnEntryBlks = (MAX_PARTITION_ENTRIES_SZ / BlkSz) + GPT_HDR_BLOCKS;
  NumSectors = DeviceDensity / BlkSz;

  /* Update the primary and backup GPT header offset with the sector location */
  PrimaryGptHeader = (Gpt + BlkSz);
  /* Patch primary GPT */
  PUT_LONG_LONG (PrimaryGptHeader + BACKUP_HEADER_OFFSET,
                 (UINT64) (NumSectors - 1));
  PUT_LONG_LONG (PrimaryGptHeader + LAST_USABLE_LBA_OFFSET,
                 (UINT64) (NumSectors - (PtnEntryBlks + 1)));

  /* Patch Backup GPT */
  Offset = (2 * PartEntryArrSz);
  SecondaryGptHeader = Offset + BlkSz + PrimaryGptHeader;
  PUT_LONG_LONG (SecondaryGptHeader + PRIMARY_HEADER_OFFSET, (UINT64)1);
  PUT_LONG_LONG (SecondaryGptHeader + LAST_USABLE_LBA_OFFSET,
                 (UINT64) (NumSectors - (PtnEntryBlks + 1)));
  PUT_LONG_LONG (SecondaryGptHeader + PARTITION_ENTRIES_OFFSET,
                 (UINT64) (NumSectors - (PtnEntryBlks)));

  /* Patch the last partition */
  LastPartitionEntry = (UINT64 *)
    (PrimaryGptHeader + BlkSz + TotalPart * PARTITION_ENTRY_SIZE);

  //need check 128 bit for GUID
  while ((TotalPart < GptHeader->MaxPtCnt) &&
    ((*LastPartitionEntry != 0) ||
    (*(LastPartitionEntry + 1) != 0))) {
    TotalPart++;
    LastPartitionEntry = (UINT64 *)
      (PrimaryGptHeader + BlkSz + TotalPart * PARTITION_ENTRY_SIZE);
  }

  LastPartOffset =
      (TotalPart - 1) * PARTITION_ENTRY_SIZE + PARTITION_ENTRY_LAST_LBA;

  PUT_LONG_LONG (PrimaryGptHeader + BlkSz + LastPartOffset,
                 (UINT64) (NumSectors - (PtnEntryBlks + 1)));
  PUT_LONG_LONG (PrimaryGptHeader + BlkSz + LastPartOffset + PartEntryArrSz,
                 (UINT64) (NumSectors - (PtnEntryBlks + 1)));

  /* Update CRC of the partition entry array for both headers */
  PartitionEntryArrStart = PrimaryGptHeader + BlkSz;
  Status = gBS->CalculateCrc32 (PartitionEntryArrStart,
                                (GptHeader->MaxPtCnt * GptHeader->PartEntrySz),
                                &CrcVal);
  if (EFI_ERROR (Status)) {
    DEBUG (
        (EFI_D_ERROR, "Error calculating CRC for primary partition entry\n"));
    return FAILURE;
  }
  PUT_LONG (PrimaryGptHeader + PARTITION_CRC_OFFSET, CrcVal);

  Status = gBS->CalculateCrc32 (PartitionEntryArrStart + PartEntryArrSz,
                                (GptHeader->MaxPtCnt * GptHeader->PartEntrySz),
                                &CrcVal);
  if (EFI_ERROR (Status)) {
    DEBUG (
        (EFI_D_ERROR, "Error calculating CRC for secondary partition entry\n"));
    return FAILURE;
  }
  PUT_LONG (SecondaryGptHeader + PARTITION_CRC_OFFSET, CrcVal);

  /* Clear Header CRC field values & recalculate */
  PUT_LONG (PrimaryGptHeader + HEADER_CRC_OFFSET, 0);
  Status = gBS->CalculateCrc32 (PrimaryGptHeader, GPT_HEADER_SIZE, &CrcVal);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "Error calculating CRC for primary gpt header\n"));
    return FAILURE;
  }
  PUT_LONG (PrimaryGptHeader + HEADER_CRC_OFFSET, CrcVal);
  PUT_LONG (SecondaryGptHeader + HEADER_CRC_OFFSET, 0);
  Status = gBS->CalculateCrc32 (SecondaryGptHeader, GPT_HEADER_SIZE, &CrcVal);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "Error calculating CRC for secondary gpt header\n"));
    return FAILURE;
  }
  PUT_LONG (SecondaryGptHeader + HEADER_CRC_OFFSET, CrcVal);

  return SUCCESS;
}

STATIC UINT32
WriteGpt (INT32 Lun, UINT32 Sz, UINT8 *Gpt)
{
  UINT32 Ret = 1;
  struct GptHeaderData GptHeader;
  UINT8 *PartEntryArrSt;
  UINT32 Offset;
  UINT32 PartEntryArrSz;
  UINT64 DeviceDensity;
  UINT32 BlkSz;
  UINT8 *PrimaryGptHdr = NULL;
  UINT8 *SecondaryGptHdr = NULL;
  EFI_STATUS Status;
  UINTN BackUpGptLba;
  UINTN PartitionEntryLba;
  EFI_BLOCK_IO_PROTOCOL *BlockIo = NULL;
  HandleInfo BlockIoHandle[MAX_HANDLEINF_LST_SIZE];
  UINT32 MaxHandles = MAX_HANDLEINF_LST_SIZE;

  Ret = GetStorageHandle (Lun, BlockIoHandle, &MaxHandles);
  if (Ret || (MaxHandles != 1)) {
    DEBUG ((EFI_D_ERROR, "Failed to get the BlockIo for the device\n"));
    return Ret;
  }

  BlockIo = BlockIoHandle[0].BlkIo;
  DeviceDensity = GetPartitionSize (BlockIo);
  if (!DeviceDensity) {
    return FAILURE;
  }
  BlkSz = BlockIo->Media->BlockSize;

  /* Verity that passed block has valid GPT primary header
     * Sz is from mNumDataBytes and it will check at CmdDownload
     * if it is mNumDataBytes > MaxDownLoadSize it will fail early and
     * will not cause any oob
     */
  if (Sz <= BlkSz * 2) {
    DEBUG ((EFI_D_ERROR, "Gpt Image size is invalid!\n"));
    return FAILURE;
  }
  PrimaryGptHdr = (Gpt + BlkSz);
  Ret = ParseGptHeader (&GptHeader, PrimaryGptHdr, DeviceDensity, BlkSz);
  if (Ret) {
    DEBUG ((EFI_D_ERROR, "GPT: Error processing primary GPT header\n"));
    return Ret;
  }

  /* Check if a valid back up GPT is present */
  PartEntryArrSz = GptHeader.PartEntrySz * GptHeader.MaxPtCnt;
  if (PartEntryArrSz < MIN_PARTITION_ARRAY_SIZE)
    PartEntryArrSz = MIN_PARTITION_ARRAY_SIZE;

  /* Back up partition is stored in the reverse order with back GPT, followed by
   * part entries, find the offset to back up GPT */
  Offset = (2 * PartEntryArrSz);
  if (Sz < (Offset + (BlkSz * 3))) {
    DEBUG ((EFI_D_ERROR, "Gpt Image size is invalid!!\n"));
    return FAILURE;
  }
  SecondaryGptHdr = Offset + BlkSz + PrimaryGptHdr;
  ParseSecondaryGpt = TRUE;

  Ret = ParseGptHeader (&GptHeader, SecondaryGptHdr, DeviceDensity, BlkSz);
  if (Ret) {
    DEBUG ((EFI_D_ERROR, "GPT: Error processing backup GPT header\n"));
    return Ret;
  }

  Ret = PatchGpt (Gpt, DeviceDensity, PartEntryArrSz, &GptHeader, BlkSz);
  if (Ret) {
    DEBUG ((EFI_D_ERROR, "Failed to patch GPT\n"));
    return Ret;
  }
  /* Erase the entire card */
  Status = ErasePartition (BlockIo, BlockIoHandle[0].Handle);
  if (Status != EFI_SUCCESS) {
    DEBUG ((EFI_D_ERROR, "Error erasing the storage device: %r\n", Status));
    return FAILURE;
  }

  /* write the protective MBR */
  Status = BlockIo->WriteBlocks (BlockIo, BlockIo->Media->MediaId, 0, BlkSz,
                                 (VOID *)Gpt);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "Error writing protective MBR: %x\n", Status));
    return FAILURE;
  }

  /* Write the primary GPT header, which is at an offset of BlkSz */
  Status = BlockIo->WriteBlocks (BlockIo, BlockIo->Media->MediaId, 1, BlkSz,
                                 (VOID *)PrimaryGptHdr);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "Error writing primary GPT header: %r\n", Status));
    return FAILURE;
  }

  /* Write the back up GPT header */
  BackUpGptLba = GET_LLWORD_FROM_BYTE (&PrimaryGptHdr[BACKUP_HEADER_OFFSET]);
  Status = BlockIo->WriteBlocks (BlockIo, BlockIo->Media->MediaId, BackUpGptLba,
                                 BlkSz, (VOID *)SecondaryGptHdr);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "Error writing secondary GPT header: %x\n", Status));
    return FAILURE;
  }

  /* write Partition Entries for primary partition table*/
  PartEntryArrSt = PrimaryGptHdr + BlkSz;
  PartitionEntryLba =
      GET_LLWORD_FROM_BYTE (&PrimaryGptHdr[PARTITION_ENTRIES_OFFSET]);
  Status =
      BlockIo->WriteBlocks (BlockIo, BlockIo->Media->MediaId, PartitionEntryLba,
                            PartEntryArrSz, (VOID *)PartEntryArrSt);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR,
            "Error writing partition entries array for Primary Table: %x\n",
            Status));
    return FAILURE;
  }

  /* write Partition Entries for secondary partition table*/
  PartEntryArrSt = PrimaryGptHdr + BlkSz + PartEntryArrSz;
  PartitionEntryLba =
      GET_LLWORD_FROM_BYTE (&SecondaryGptHdr[PARTITION_ENTRIES_OFFSET]);
  Status =
      BlockIo->WriteBlocks (BlockIo, BlockIo->Media->MediaId, PartitionEntryLba,
                            PartEntryArrSz, (VOID *)PartEntryArrSt);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR,
            "Error writing partition entries array for Secondary Table: %x\n",
            Status));
    return FAILURE;
  }

  Status = BlockIo->FlushBlocks (BlockIo);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR,
            "Error flushing partition entries array for Secondary Table: %x\n",
            Status));
    return FAILURE;
  }

  FlashingGpt = 0;
  gBS->SetMem ((VOID *)Gpt, Sz, 0x0);

  DEBUG ((EFI_D_ERROR, "Updated Partition Table Successfully\n"));
  return SUCCESS;
}

EFI_STATUS
UpdatePartitionTable (UINT8 *GptImage,
                      UINT32 Sz,
                      INT32 Lun,
                      struct StoragePartInfo *Ptable)
{
  EFI_STATUS Status = EFI_SUCCESS;
  UINT32 Ptype;
  UINT32 Ret;

  /* Check if the partition type is GPT */
  Ret = PartitionGetType (Sz, GptImage, &Ptype);
  if (Ret != 0) {
    DEBUG (
        (EFI_D_ERROR, "Failed to get partition type from input gpt image\n"));
    return EFI_NOT_FOUND;
  }

  switch (Ptype) {
  case PARTITION_TYPE_GPT:
    DEBUG ((EFI_D_INFO, "Updating GPT partition\n"));
    FlashingGpt = TRUE;
    Ret = WriteGpt (Lun, Sz, GptImage);
    if (Ret != 0) {
      DEBUG ((EFI_D_ERROR, "Failed to write Gpt partition: %x\n", Ret));
      return EFI_VOLUME_CORRUPTED;
    }
    break;
  default:
    DEBUG ((EFI_D_ERROR, "Invalid Partition type: %x\n", Ptype));
    Status = EFI_UNSUPPORTED;
    break;
  }

  return Status;
}

STATIC struct PartitionEntry *
GetBootPartitionEntry (Slot *BootSlot)
{
  INT32 Index = INVALID_PTN;

  if (StrnCmp ((CONST CHAR16 *)L"_a", BootSlot->Suffix,
               StrLen (BootSlot->Suffix)) == 0) {
    Index = GetPartitionIndex ((CHAR16 *)L"boot_a");
  } else if (StrnCmp ((CONST CHAR16 *)L"_b", BootSlot->Suffix,
                      StrLen (BootSlot->Suffix)) == 0) {
    Index = GetPartitionIndex ((CHAR16 *)L"boot_b");
  } else {
    DEBUG ((EFI_D_ERROR, "GetBootPartitionEntry: No boot partition "
                         "entry for slot %s\n",
            BootSlot->Suffix));
    return NULL;
  }

  if (Index == INVALID_PTN) {
    DEBUG ((EFI_D_ERROR, "GetBootPartitionEntry: No boot partition entry "
                         "for slot %s, invalid index\n",
            BootSlot->Suffix));
    return NULL;
  }
  return &PtnEntries[Index];
}

BOOLEAN IsCurrentSlotBootable (VOID)
{
  Slot CurrentSlot = {{0}};
#ifdef AUTO_VIRT_ABL
  BOOLEAN Ret = FALSE;
  struct RecoveryMessage *Msg = NULL;
  EFI_GUID Ptype = gEfiMiscPartitionGuid;
  VOID *PartitionData = NULL;
  UINT32 PageSize;
#else
  struct PartitionEntry *BootPartition = NULL;
#endif
  EFI_STATUS Status = GetActiveSlot (&CurrentSlot);

  if (Status != EFI_SUCCESS) {
    DEBUG ((EFI_D_ERROR, "IsCurrentSlotBootable: no active slots found!\n"));
    return FALSE;
  }

#ifdef AUTO_VIRT_ABL
  GetPageSize (&PageSize);
  Status = ReadFromPartition (&Ptype, (VOID **)&PartitionData, (PageSize * 4));
  if (Status != EFI_SUCCESS) {
    DEBUG ((EFI_D_ERROR, "Error Reading from misc partition: %r\n", Status));
    return FALSE;
  }

  if (!PartitionData) {
    DEBUG ((EFI_D_ERROR, "Error in loading Data from misc partition\n"));
    return FALSE;
  }

  Msg = (struct RecoveryMessage *) PartitionData;
  if (Msg->Reserved[3] == 'y') {
    Ret = TRUE;
  } else if (Msg->Reserved[5] == 'y') {
    Ret = TRUE;
  }

  FreePool (PartitionData);
  PartitionData = NULL;
  Msg = NULL;

  if (Ret) {
    DEBUG ((EFI_D_VERBOSE, "Slot %s is bootable\n", CurrentSlot.Suffix));
    return TRUE;
  }
#else
  BootPartition = GetBootPartitionEntry (&CurrentSlot);
  if (BootPartition == NULL) {
    DEBUG ((EFI_D_ERROR, "IsCurrentSlotBootable: No boot partition "
                         "entry for slot %s\n",
            CurrentSlot.Suffix));
    return FALSE;
  }
  DEBUG ((EFI_D_VERBOSE, "Slot suffix %s Part Attr 0x%lx\n", CurrentSlot.Suffix,
          BootPartition->PartEntry.Attributes));

  if (!(BootPartition->PartEntry.Attributes & PART_ATT_UNBOOTABLE_VAL) &&
      BootPartition->PartEntry.Attributes & PART_ATT_SUCCESSFUL_VAL) {
    DEBUG ((EFI_D_VERBOSE, "Slot %s is bootable\n", CurrentSlot.Suffix));
    return TRUE;
  }
#endif

  DEBUG ((EFI_D_VERBOSE, "Slot %s is unbootable \n", CurrentSlot.Suffix));
  return FALSE;
}

BOOLEAN
IsSuffixEmpty (Slot *CheckSlot)
{
  if (CheckSlot == NULL) {
    return TRUE;
  }

  if (StrLen (CheckSlot->Suffix) == 0) {
    return TRUE;
  }
  return FALSE;
}

BOOLEAN IsSlotsUbootable (VOID)
{
  Slot Slots[] = {{L"_a"}, {L"_b"}};
  struct PartitionEntry *BootEntry = NULL;
  UINT32 Count = 0;
  UINT32 UnbootableCount = 0;
  BOOLEAN IsMultiSlot = PartitionHasMultiSlot ((CONST CHAR16 *)L"boot");

  if (IsMultiSlot == FALSE) {
    return FALSE;
  }

  for (Count = 0; Count < ARRAY_SIZE (Slots); Count++) {
    BootEntry = GetBootPartitionEntry (&Slots[Count]);
    if (BootEntry == NULL) {
      DEBUG ((EFI_D_ERROR, "CheckBootableSlot: No boot partition "
                           "entry for slot %s\n", Slots[Count].Suffix));
      return FALSE;
    }
    if (BootEntry->PartEntry.Attributes & PART_ATT_UNBOOTABLE_VAL) {
      UnbootableCount += 1;
    }
  }

  if (UnbootableCount < ARRAY_SIZE (Slots)) {
    return FALSE;
  }
  return TRUE;
}

STATIC EFI_STATUS
GetAtomicABActiveSlot (Slot *ActiveSlot)
{
  EFI_STATUS Status = EFI_SUCCESS;
  CHAR8 BootDeviceType[BOOT_DEV_NAME_SIZE_MAX];
  UINT32 UfsBootLun = 0;
  Slot Slots[] = {{L"_a"}, {L"_b"}};

  GetRootDeviceType (BootDeviceType, BOOT_DEV_NAME_SIZE_MAX);
  if (!AsciiStrnCmp (BootDeviceType, "UFS", AsciiStrLen ("UFS"))) {
    GUARD (UfsGetSetBootLun (&UfsBootLun, TRUE));
    if (UfsBootLun == 0x1) {
      GUARD (StrnCpyS (ActiveSlot->Suffix, ARRAY_SIZE (ActiveSlot->Suffix),
                       Slots[0].Suffix,
                       StrLen (Slots[0].Suffix)));
      DEBUG ((EFI_D_INFO, "GetACtiveSlot: ufs device boot lun is %x, "
                          " select active slot %s\n",
                          UfsBootLun, ActiveSlot->Suffix));
    } else if (UfsBootLun == 0x2) {
      GUARD (StrnCpyS (ActiveSlot->Suffix, ARRAY_SIZE (ActiveSlot->Suffix),
                       Slots[1].Suffix,
                       StrLen (Slots[1].Suffix)));
      DEBUG ((EFI_D_INFO, "GetACtiveSlot: ufs device boot lun is %x, "
                          " select active slot %s\n",
                          UfsBootLun, ActiveSlot->Suffix));
    } else {
      DEBUG ((EFI_D_ERROR, "Boot lun: %x invalid\n", UfsBootLun));
      return EFI_DEVICE_ERROR;
    }
    return EFI_SUCCESS;
  }

  else {
    return EFI_UNSUPPORTED;
  }
}

#if ENABLE_LV_ATOMIC_AB
STATIC BOOLEAN
AtomicABEnabled (VOID)
{
  return TRUE;
}
#else
STATIC BOOLEAN
AtomicABEnabled (VOID)
{
  return FALSE;
}
#endif

EFI_STATUS
GetActiveSlot (Slot *ActiveSlot)
{
  EFI_STATUS Status = EFI_SUCCESS;
  Slot Slots[] = {{L"_a"}, {L"_b"}};
  UINT64 Priority = 0;

#ifdef AUTO_VIRT_ABL
  UINTN DataSize = 0;
  CHAR16 TempActiveSlot[] = L"_x";

  DataSize = sizeof (TempActiveSlot);
  Status = gRT->GetVariable ((CHAR16 *)L"ActiveSlot", &gQcomTokenSpaceGuid,
                          NULL, &DataSize, TempActiveSlot);
  GUARD (StrnCpyS (ActiveSlot->Suffix, ARRAY_SIZE (ActiveSlot->Suffix),
                  TempActiveSlot, StrLen (TempActiveSlot)));

  return Status;
#endif

  if (ActiveSlot == NULL) {
    DEBUG ((EFI_D_ERROR, "GetActiveSlot: bad parameter\n"));
    return EFI_INVALID_PARAMETER;
  }

  for (UINTN SlotIndex = 0; SlotIndex < ARRAY_SIZE (Slots); SlotIndex++) {
    struct PartitionEntry *BootPartition =
        GetBootPartitionEntry (&Slots[SlotIndex]);
    UINT64 BootPriority = 0;
    if (BootPartition == NULL) {
      DEBUG ((EFI_D_ERROR, "GetActiveSlot: No boot partition "
                           "entry for slot %s\n",
              Slots[SlotIndex].Suffix));
      return EFI_NOT_FOUND;
    }

    BootPriority =
        (BootPartition->PartEntry.Attributes & PART_ATT_PRIORITY_VAL) >>
        PART_ATT_PRIORITY_BIT;

    if ((BootPartition->PartEntry.Attributes & PART_ATT_ACTIVE_VAL) &&
        (BootPriority > Priority)) {
      GUARD (StrnCpyS (ActiveSlot->Suffix, ARRAY_SIZE (ActiveSlot->Suffix),
                       Slots[SlotIndex].Suffix,
                       StrLen (Slots[SlotIndex].Suffix)));
      Priority = BootPriority;
    }
  }

  DEBUG ((EFI_D_VERBOSE, "GetActiveSlot: found active slot %s, priority %d\n",
          ActiveSlot->Suffix, Priority));

  if (AtomicABEnabled ()) {
    return GetAtomicABActiveSlot (ActiveSlot);
  }

  if (IsSuffixEmpty (ActiveSlot) == TRUE) {
    /* Check for first boot and set default slot */
    /* For First boot all A/B attributes for the slot would be 0 */
    UINT64 BootPriority = 0;
    struct PartitionEntry *SlotA = GetBootPartitionEntry (&Slots[0]);
    if (SlotA == NULL) {
      DEBUG ((EFI_D_ERROR, "GetActiveSlot: First Boot: No boot partition "
                           "entry for slot %s\n",
              Slots[0].Suffix));
      return EFI_NOT_FOUND;
    }

    BootPriority = (SlotA->PartEntry.Attributes & PART_ATT_PRIORITY_VAL) >>
                   PART_ATT_PRIORITY_BIT;

    if ((SlotA->PartEntry.Attributes & PART_ATT_ACTIVE_VAL) == 0 &&
        (SlotA->PartEntry.Attributes & PART_ATT_SUCCESSFUL_VAL) == 0 &&
        (SlotA->PartEntry.Attributes & PART_ATT_UNBOOTABLE_VAL) == 0 &&
        BootPriority == 0) {

      DEBUG ((EFI_D_INFO, "GetActiveSlot: First boot: set "
                          "default slot _a\n"));
      SlotA->PartEntry.Attributes &=
          (~PART_ATT_SUCCESSFUL_VAL & ~PART_ATT_UNBOOTABLE_VAL);
      SlotA->PartEntry.Attributes |=
          (PART_ATT_PRIORITY_VAL | PART_ATT_ACTIVE_VAL |
           PART_ATT_MAX_RETRY_COUNT_VAL);

      GUARD (StrnCpyS (ActiveSlot->Suffix, ARRAY_SIZE (ActiveSlot->Suffix),
                       Slots[0].Suffix, StrLen (Slots[0].Suffix)));
      UpdatePartitionAttributes (PARTITION_ATTRIBUTES);
      FirstBoot = TRUE;
      return EFI_SUCCESS;
    }

    DEBUG ((EFI_D_ERROR, "GetActiveSlot: No active slot found\n"));
    DEBUG ((EFI_D_ERROR, "GetActiveSlot: Slot attr: Priority %ld, Retry "
                         "%ld, Active %ld, Success %ld, unboot %ld\n",
            BootPriority, RetryCount,
            (SlotA->PartEntry.Attributes & PART_ATT_ACTIVE_VAL) >>
                PART_ATT_ACTIVE_BIT,
            (SlotA->PartEntry.Attributes & PART_ATT_SUCCESSFUL_VAL),
            (SlotA->PartEntry.Attributes & PART_ATT_UNBOOTABLE_VAL)));

    return EFI_NOT_FOUND;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
SetActiveSlot (Slot *NewSlot, BOOLEAN ResetSuccessBit)
{
  EFI_STATUS Status = EFI_SUCCESS;
  Slot CurrentSlot = {{0}};
  Slot *AlternateSlot = NULL;
  Slot Slots[] = {{L"_a"}, {L"_b"}};
  BOOLEAN UfsGet = TRUE;
  BOOLEAN UfsSet = FALSE;
  UINT32 UfsBootLun = 0;
  CHAR8 BootDeviceType[BOOT_DEV_NAME_SIZE_MAX];
  struct PartitionEntry *BootEntry = NULL;

  if (NewSlot == NULL) {
    DEBUG ((EFI_D_ERROR, "SetActiveSlot: input parameter invalid\n"));
    return EFI_INVALID_PARAMETER;
  }

  GUARD (GetActiveSlot (&CurrentSlot));

  if (StrnCmp (NewSlot->Suffix, Slots[0].Suffix, StrLen (Slots[0].Suffix)) ==
      0) {
    AlternateSlot = &Slots[1];
  } else {
    AlternateSlot = &Slots[0];
  }

  BootEntry = GetBootPartitionEntry (NewSlot);
  if (BootEntry == NULL) {
    DEBUG ((EFI_D_ERROR, "SetActiveSlot: No boot partition entry for slot %s\n",
            NewSlot->Suffix));
    return EFI_NOT_FOUND;
  }

  BootEntry->PartEntry.Attributes |=
      (PART_ATT_PRIORITY_VAL | PART_ATT_ACTIVE_VAL |
       PART_ATT_MAX_RETRY_COUNT_VAL);

  BootEntry->PartEntry.Attributes &= (~PART_ATT_UNBOOTABLE_VAL);

  if (ResetSuccessBit &&
      (BootEntry->PartEntry.Attributes & PART_ATT_SUCCESSFUL_VAL)) {
    BootEntry->PartEntry.Attributes &= (~PART_ATT_SUCCESSFUL_VAL);
  }

  /* Reduce the priority and clear the active flag for alternate slot*/
  BootEntry = GetBootPartitionEntry (AlternateSlot);
  if (BootEntry == NULL) {
    DEBUG ((EFI_D_ERROR, "SetActiveSlot: No boot partition entry for slot %s\n",
            AlternateSlot->Suffix));
    return EFI_NOT_FOUND;
  }

  BootEntry->PartEntry.Attributes &=
      (~PART_ATT_PRIORITY_VAL & ~PART_ATT_ACTIVE_VAL);
  BootEntry->PartEntry.Attributes |=
      (((UINT64)MAX_PRIORITY - 1) << PART_ATT_PRIORITY_BIT);

  UpdatePartitionAttributes (PARTITION_ATTRIBUTES);
  if (StrnCmp (CurrentSlot.Suffix, NewSlot->Suffix,
               StrLen (CurrentSlot.Suffix)) == 0) {
    DEBUG ((EFI_D_INFO, "SetActiveSlot: %s already active slot\n",
            NewSlot->Suffix));

    /* Check if BootLun is matching with Slot */
    GetRootDeviceType (BootDeviceType, BOOT_DEV_NAME_SIZE_MAX);
    if (!AsciiStrnCmp (BootDeviceType, "UFS", AsciiStrLen ("UFS"))) {
      UfsGetSetBootLun (&UfsBootLun, UfsGet);
      if (UfsBootLun == 0x1 &&
          !StrnCmp (CurrentSlot.Suffix, (CONST CHAR16 *)L"_b",
          StrLen ((CONST CHAR16 *)L"_b"))) {
        DEBUG ((EFI_D_INFO, "Boot lun mismatch switch from 1 to 2\n"));
        DEBUG ((EFI_D_INFO, "Reboot Required\n"));
        UfsBootLun = 0x2;
        UfsGetSetBootLun (&UfsBootLun, UfsSet);
      } else if (UfsBootLun == 0x2 &&
               !StrnCmp (CurrentSlot.Suffix, (CONST CHAR16 *)L"_a",
               StrLen ((CONST CHAR16 *)L"_a"))) {
        DEBUG ((EFI_D_INFO, "Boot lun mismatch switch from 2 to 1\n"));
        DEBUG ((EFI_D_INFO, "Reboot Required\n"));
        UfsBootLun = 0x1;
        UfsGetSetBootLun (&UfsBootLun, UfsSet);
      }
    }
  } else {
    DEBUG ((EFI_D_INFO, "Alternate slot %s, New slot %s\n",
            AlternateSlot->Suffix, NewSlot->Suffix));
    SwitchPtnSlots (NewSlot->Suffix);
    MarkPtnActive (NewSlot->Suffix);
  }
  return EFI_SUCCESS;
}

EFI_STATUS HandleActiveSlotUnbootable (BOOLEAN ForceBootAlternateSlot)
{
  EFI_STATUS Status = EFI_SUCCESS;
  struct PartitionEntry *BootEntry = NULL;
  Slot ActiveSlot = {{0}};
  Slot *AlternateSlot = NULL;
  Slot Slots[] = {{L"_a"}, {L"_b"}};
  UINT64 Unbootable = 0;
  UINT64 BootSuccess = 0;

  /* Mark current Slot as unbootable */
  GUARD (GetActiveSlot (&ActiveSlot));
  BootEntry = GetBootPartitionEntry (&ActiveSlot);
  if (BootEntry == NULL) {
    DEBUG ((EFI_D_ERROR, "HandleActiveSlotUnbootable: No boot "
                         "partition entry for slot %s\n",
            ActiveSlot.Suffix));
    return EFI_NOT_FOUND;
  }

  if (FirstBoot) {
    DEBUG ((EFI_D_VERBOSE, "FirstBoot, skipping slot Unbootable\n"));
    FirstBoot = FALSE;
  } else {
    BootEntry->PartEntry.Attributes |=
        (PART_ATT_UNBOOTABLE_VAL) & (~PART_ATT_SUCCESSFUL_VAL);
    UpdatePartitionAttributes (PARTITION_ATTRIBUTES);
  }

  if (StrnCmp (ActiveSlot.Suffix, Slots[0].Suffix, StrLen (Slots[0].Suffix)) ==
      0) {
    AlternateSlot = &Slots[1];
  } else {
    AlternateSlot = &Slots[0];
  }

  /* Validate Aternate Slot is bootable */
  BootEntry = GetBootPartitionEntry (AlternateSlot);
  if (BootEntry == NULL) {
    DEBUG ((EFI_D_ERROR, "HandleActiveSlotUnbootable: No boot "
                         "partition entry for slot %s\n",
            AlternateSlot->Suffix));
    return EFI_NOT_FOUND;
  }

  Unbootable = (BootEntry->PartEntry.Attributes & PART_ATT_UNBOOTABLE_VAL) >>
               PART_ATT_UNBOOTABLE_BIT;
  BootSuccess = (BootEntry->PartEntry.Attributes & PART_ATT_SUCCESSFUL_VAL) >>
                PART_ATT_SUCCESS_BIT;

  if ((Unbootable == 0 &&
       BootSuccess == 1) ||
       ForceBootAlternateSlot) {
    DEBUG (
        (EFI_D_INFO, "Alternate Slot %s is bootable\n", AlternateSlot->Suffix));
    GUARD (SetActiveSlot (AlternateSlot, FALSE));

    DEBUG ((EFI_D_INFO, "HandleActiveSlotUnbootable: Rebooting\n"));
    gRT->ResetSystem (EfiResetCold, EFI_SUCCESS, 0, NULL);

    // Shouldn't get here
    DEBUG ((EFI_D_ERROR, "HandleActiveSlotUnbootable: "
                         "gRT->Resetystem didn't work\n"));
    return EFI_LOAD_ERROR;
  }

  return EFI_LOAD_ERROR;
}

EFI_STATUS ClearUnbootable (VOID)
{
  EFI_STATUS Status = EFI_SUCCESS;
  Slot ActiveSlot = {{0}};
  struct PartitionEntry *BootEntry = NULL;

  Status = GetActiveSlot (&ActiveSlot);
  if (Status != EFI_SUCCESS) {
    DEBUG ((EFI_D_ERROR, "ClearUnbootable: GetActiveSlot failed.\n"));
    return Status;
  }
  BootEntry = GetBootPartitionEntry (&ActiveSlot);
  if (BootEntry == NULL) {
    DEBUG ((EFI_D_ERROR,
            "ClearUnbootable: No boot partition entry for slot %s\n",
            ActiveSlot.Suffix));
    return EFI_NOT_FOUND;
  }
  BootEntry->PartEntry.Attributes &= ~PART_ATT_UNBOOTABLE_VAL;
  UpdatePartitionAttributes (PARTITION_ATTRIBUTES);
  return EFI_SUCCESS;
}

