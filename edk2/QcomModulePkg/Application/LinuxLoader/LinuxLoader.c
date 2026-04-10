/*
 * Copyright (c) 2009, Google Inc.
 * All rights reserved.
 *
 * Copyright (c) 2009-2021, The Linux Foundation. All rights reserved.
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
 *  Changes from Qualcomm Innovation Center are provided under the following license:
 *
 *  Copyright (c) 2022 - 2025 Qualcomm Innovation Center, Inc. All rights
 *  reserved.
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
#include "BootStats.h"
#include "KeyPad.h"
#include "LinuxLoaderLib.h"
#include <FastbootLib/FastbootMain.h>
#include <Library/DeviceInfo.h>
#include <Library/DrawUI.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PartitionTableUpdate.h>
#include <Library/ShutdownServices.h>
#include <Library/StackCanary.h>
#include "Library/ThreadStack.h"
#include <Library/HypervisorMvCalls.h>
#include <Library/UpdateCmdLine.h>
#include <Protocol/EFICardInfo.h>

#define MAX_APP_STR_LEN 64
#define MAX_NUM_FS 10
#define DEFAULT_STACK_CHK_GUARD 0xc0c0c0c0

#if HIBERNATION_SUPPORT_NO_AES
VOID BootIntoHibernationImage (BootInfo *Info,
                               BOOLEAN *SetRotAndBootStateAndVBH);
#endif

STATIC BOOLEAN BootIntoFastboot = FALSE;
STATIC BOOLEAN BootIntoRecovery = FALSE;
UINT64 FlashlessBootImageAddr = 0;
STATIC DeviceInfo DevInfo;


STATIC VOID
SetDefaultAudioFw ()
{
  CHAR8 AudioFW[MAX_AUDIO_FW_LENGTH];
  STATIC CHAR8* Src;
  STATIC CHAR8* AUDIOFRAMEWORK;
  STATIC UINT32 Length;
  EFI_STATUS Status;

  AUDIOFRAMEWORK = GetAudioFw ();
  Status = ReadAudioFrameWork (&Src, &Length);
  if ((AsciiStrCmp (Src, "audioreach") == 0) ||
                              (AsciiStrCmp (Src, "elite") == 0) ||
                              (AsciiStrCmp (Src, "awe") == 0)) {
    if (Status == EFI_SUCCESS) {
      if (AsciiStrLen (Src) == 0) {
        if (AsciiStrLen (AUDIOFRAMEWORK) > 0) {
          AsciiStrnCpyS (AudioFW, MAX_AUDIO_FW_LENGTH, AUDIOFRAMEWORK,
          AsciiStrLen (AUDIOFRAMEWORK));
          StoreAudioFrameWork (AudioFW, AsciiStrLen (AUDIOFRAMEWORK));
        }
      }
    }
    else {
      DEBUG ((EFI_D_ERROR, "AUDIOFRAMEWORK is NOT updated length =%d, %a\n",
      Length, AUDIOFRAMEWORK));
    }
  }
  else {
    if (Src != NULL) {
      Status =
      ReadWriteDeviceInfo (READ_CONFIG, (VOID *)&DevInfo, sizeof (DevInfo));
      if (Status != EFI_SUCCESS) {
        DEBUG ((EFI_D_ERROR, "Unable to Read Device Info: %r\n", Status));
       }
      gBS->SetMem (DevInfo.AudioFramework, sizeof (DevInfo.AudioFramework), 0);
      gBS->CopyMem (DevInfo.AudioFramework, AUDIOFRAMEWORK,
                                      AsciiStrLen (AUDIOFRAMEWORK));
      Status =
      ReadWriteDeviceInfo (WRITE_CONFIG, (VOID *)&DevInfo, sizeof (DevInfo));
      if (Status != EFI_SUCCESS) {
        DEBUG ((EFI_D_ERROR, "Unable to store audio framework: %r\n", Status));
        return;
      }
    }
  }
}

BOOLEAN IsABRetryCountUpdateRequired (VOID)
{
 BOOLEAN BatteryStatus;

 /* Check power off charging */
 TargetPauseForBatteryCharge (&BatteryStatus);

 /* Do not decrement bootable retry count in below states:
 * fastboot, fastbootd, charger, recovery
 */
 if ((BatteryStatus &&
 IsChargingScreenEnable ()) ||
 BootIntoFastboot ||
 BootIntoRecovery) {
  return FALSE;
 }
  return TRUE;
}


/**
  Linux Loader Application EntryPoint

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

 **/
/**
 * 等待指定时间内检测音量下键
 * 使用 WaitForEvent 避免轮询，更高效
 *
 * @param TimeoutMs超时时间（毫秒）
 * @return TRUE       检测到音量下键
 * @return FALSE      超时未检测到
 */
STATIC UINT8
WaitForVolumeDownKey (IN UINT32 TimeoutMs)
{
  EFI_STATUS    Status;
  EFI_EVENT     TimerEvent;
  EFI_EVENT     WaitList[2];
  UINTN         EventIndex;
  EFI_INPUT_KEY Key;
  UINT8         KeyDetected = 0;

  /* 先清空输入缓冲区 */
  gST->ConIn->Reset (gST->ConIn, FALSE);

  /* 创建定时器事件 */
  Status = gBS->CreateEvent (
                  EVT_TIMER,
                  TPL_CALLBACK,
                  NULL,
                  NULL,
                  &TimerEvent
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "CreateEvent Timer failed: %r\n", Status));
    return FALSE;
  }

  /* 设置定时器：一次性触发，单位为100ns
   * 5秒 = 5 * 1000 * 1000 * 10= 50,000,000 (100ns单位)
   */
  Status = gBS->SetTimer (
                  TimerEvent,
                  TimerRelative,
                  (UINT64)TimeoutMs * 10000   /* ms ->100ns */
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "SetTimer failed: %r\n", Status));
    gBS->CloseEvent (TimerEvent);
    return FALSE;
  }

  /* 等待事件列表：按键事件 或 定时器超时 */
  WaitList[0] = gST->ConIn->WaitForKey;
  WaitList[1] = TimerEvent;

  while (TRUE) {
    Status = gBS->WaitForEvent (2, WaitList, &EventIndex);
    if (EFI_ERROR (Status)) {
      DEBUG ((EFI_D_ERROR, "WaitForEvent failed: %r\n", Status));
      break;
    }

    if (EventIndex == 0) {
      /* 按键事件触发 */
      Status = gST->ConIn->ReadKeyStroke (gST->ConIn, &Key);
      if (!EFI_ERROR (Status)) {
        DEBUG ((EFI_D_INFO, "Key detected: ScanCode=0x%x, UnicodeChar=0x%x\n",Key.ScanCode, Key.UnicodeChar));

        if (Key.ScanCode == SCAN_DOWN) {//fastboot key
          /* 检测到音量下键 */
          KeyDetected = 1;
          break;
        }
        if (Key.ScanCode == SCAN_UP) { //recovery key
          /* 检测到音量上键*/
          KeyDetected = 2;
          break;
        }
        /* 不是目标按键，继续等待 */
        DEBUG ((EFI_D_INFO, "Not volume down key, continue waiting...\n"));
      }
    } else {
      /* 定时器超时 */
      DEBUG ((EFI_D_INFO, "Timeout: %d ms expired, no key pressed\n", TimeoutMs));
      break;
    }
  }

  /* 清理定时器事件 */
  gBS->CloseEvent (TimerEvent);

  return KeyDetected;
}
#ifndef TEST_ADAPTER
STATIC EFI_STATUS
BootEfiImage (VOID *Data, UINT32 Size)
{
  EFI_STATUS  Status;
  EFI_HANDLE  ImageHandle = NULL;

  Status = gBS->LoadImage (
                  FALSE,
                  gImageHandle,
                  NULL,
                  Data,
                  Size,
                  &ImageHandle
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "LoadImage failed: %r\n", Status));
    return Status;
  }

  Status = gBS->StartImage (ImageHandle, NULL, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "StartImage failed: %r\n", Status));
  }

  return Status;
}
#ifndef AUTO_PATCH_ABL
#include "ABL.h"
#else
#include <Protocol/DiskIo.h>
#include <Pi/PiFirmwareVolume.h>
#include <Pi/PiFirmwareFile.h>
#ifndef DISABLE_PRINT
#define PRINT(fmt, ...) Print(fmt, ##__VA_ARGS__)
#else
#define PRINT(fmt, ...) do {} while(0)
#endif

// ─── 工具函数：读取 FFS 3字节大小字段 ────────────────────────────

// ─── 获取分区 BlkIo Handle ───────────────────────────────────────
EFI_STATUS
GetHandle (CHAR16 *UnicodePartition, EFI_BLOCK_IO_PROTOCOL **PartHandle)
{
  EFI_STATUS        Status;
  PartiSelectFilter HandleFilter  = {0};
  UINT32            BlkIOAttrib   = 0;
  HandleInfo        HandleInfoList[MAX_HANDLEINF_LST_SIZE];
  UINT32            MaxHandles    = ARRAY_SIZE (HandleInfoList);

  BlkIOAttrib |= BLK_IO_SEL_PARTITIONED_GPT;
  BlkIOAttrib |= BLK_IO_SEL_PARTITIONED_MBR;
  BlkIOAttrib |= BLK_IO_SEL_MEDIA_TYPE_NON_REMOVABLE;
  BlkIOAttrib |= BLK_IO_SEL_MATCH_PARTITION_LABEL;

  HandleFilter.PartitionLabel = UnicodePartition;
  HandleFilter.RootDeviceType = NULL;
  HandleFilter.VolumeName     = 0;

  Status = GetBlkIOHandles (BlkIOAttrib, &HandleFilter,
                             HandleInfoList, &MaxHandles);
  if (EFI_ERROR (Status)) {
    PRINT(L"GetHandle: GetBlkIOHandles failed: %r\n", Status);
    return EFI_NOT_FOUND;
  }

  if (MaxHandles == 0) {
    PRINT(L"GetHandle: partition not found: %s\n", UnicodePartition);
    return EFI_NOT_FOUND;
  }

  if (MaxHandles > 1) {
    PRINT(L"GetHandle: ambiguous, %u handles found\n", MaxHandles);
    return EFI_NOT_FOUND;
  }

  *PartHandle = HandleInfoList[0].BlkIo;
  return EFI_SUCCESS;
}

// ─── 读取整个分区内容 ─────────────────────────────────────────────
EFI_STATUS
ReadEntirePartition (CHAR16 *PartitionName, VOID **Buffer, UINTN *BufferSize)
{
  EFI_STATUS             Status;
  EFI_BLOCK_IO_PROTOCOL *BlkIo = NULL;

  Status = GetHandle (PartitionName, &BlkIo);
  if (EFI_ERROR (Status)) {
    PRINT(L"ReadEntirePartition: GetHandle failed: %r\n", Status);
    return Status;
  }

  if (!BlkIo->Media->MediaPresent) {
    PRINT(L"ReadEntirePartition: no media\n");
    return EFI_NO_MEDIA;
  }

  UINTN Size = (UINTN)BlkIo->Media->BlockSize *
               (UINTN)(BlkIo->Media->LastBlock + 1);

  VOID *Buf = AllocatePool (Size);
  if (Buf == NULL) {
    PRINT(L"ReadEntirePartition: AllocatePool failed\n");
    return EFI_OUT_OF_RESOURCES;
  }

  Status = BlkIo->ReadBlocks (BlkIo, BlkIo->Media->MediaId,
                               0, Size, Buf);
  if (EFI_ERROR (Status)) {
    PRINT(L"ReadEntirePartition: ReadBlocks failed: %r\n", Status);
    FreePool (Buf);
    return Status;
  }

  *Buffer     = Buf;
  *BufferSize = Size;
  return EFI_SUCCESS;
}

// ─── 在原始数据中查找固件卷 FV ───────────────────────────────────
STATIC UINT8 *
FindFirmwareVolume (UINT8 *Data, UINTN Size, UINTN *OutFvSize)
{
  if (Data == NULL || Size < sizeof (EFI_FIRMWARE_VOLUME_HEADER))
    return NULL;

  for (UINTN i = 0; i + sizeof (EFI_FIRMWARE_VOLUME_HEADER) <= Size; i++) {
    EFI_FIRMWARE_VOLUME_HEADER *FvH =
        (EFI_FIRMWARE_VOLUME_HEADER *)(Data + i);
    if (FvH->Signature == EFI_FVH_SIGNATURE &&
        FvH->FvLength  >  0 &&
        FvH->FvLength  <= (UINT64)(Size - i)) {
      *OutFvSize = (UINTN)FvH->FvLength;
      return Data + i;
    }
  }
  return NULL;
}

#include <Library/LzmaCustomDecompressLib/LzmaDecompressLibInternal.h>

STATIC EFI_GUID mLzmaGuid = {
  0xEE4E5898, 0x3914, 0x4259,
  { 0x9D, 0x6E, 0xDC, 0x7B, 0xD7, 0x94, 0x03, 0xCF }
};

#pragma pack(1)
typedef struct {
  UINT32  UncompressedLength;
  UINT8   CompressionType;
} EFI_COMPRESSION_SECTION_HEADER;
#pragma pack()

#define EFI_NOT_COMPRESSED        0x00
#define EFI_STANDARD_COMPRESSION  0x01

STATIC BOOLEAN
ScanAndFindPe32 (
  IN  UINT8   *Buf,
  IN  UINTN    BufSize,
  OUT UINT8  **PeOut,
  OUT UINTN   *PeSizeOut
  );

STATIC BOOLEAN
FindPe32InSectionStream (
  IN  UINT8   *Buf,
  IN  UINTN    BufSize,
  OUT UINT8  **PeOut,
  OUT UINTN   *PeSizeOut
  );

// 对应 Python get_section_size：支持扩展大小
STATIC UINTN
GetSectionSizeEx (
  IN  UINT8  *SecBase,
  OUT UINTN  *HdrSize
  )
{
  UINTN S = (UINTN)SecBase[0]
          | ((UINTN)SecBase[1] << 8)
          | ((UINTN)SecBase[2] << 16);
  if (S == 0xFFFFFF) {
    *HdrSize = 8;
    return (UINTN)(*(UINT32 *)(SecBase + 4));
  }
  *HdrSize = 4;
  return S;
}

// 对应 Python get_ffs_size：支持扩展64位大小
STATIC UINTN
GetFfsSizeEx (
  IN  UINT8  *FfsBase,
  OUT UINTN  *HdrSize
  )
{
  UINT8 Attrs = FfsBase[19];
  UINTN S = (UINTN)FfsBase[20]
          | ((UINTN)FfsBase[21] << 8)
          | ((UINTN)FfsBase[22] << 16);

  if (S == 0xFFFFFF && (Attrs & 0x01)) {
    *HdrSize = 32;
    return (UINTN)(*(UINT64 *)(FfsBase + 24));
  }
  *HdrSize = 24;
  return S;
}

// 对应 Python _parse_fv
STATIC BOOLEAN
FindPe32InFv (
  IN  UINT8   *FvBuf,
  IN  UINTN    FvSize,
  OUT UINT8  **PeOut,
  OUT UINTN   *PeSizeOut
  )
{
  EFI_FIRMWARE_VOLUME_HEADER *FvH = (EFI_FIRMWARE_VOLUME_HEADER *)FvBuf;

  if (FvSize < sizeof(EFI_FIRMWARE_VOLUME_HEADER) ||
      FvH->Signature != EFI_FVH_SIGNATURE ||
      FvH->HeaderLength < sizeof(EFI_FIRMWARE_VOLUME_HEADER) ||
      FvH->HeaderLength > FvSize ||
      FvH->FvLength > (UINT64)FvSize) {
    PRINT(L"FindPe32InFv: invalid header\n");
    return FALSE;
  }

  // 对应 Python: offset = align_up(fv_start + fv_hdr_len, 8)
  // 这里 FvBuf 是 fv_start，所以从 HeaderLength 对齐8起
  UINTN Offset = (FvH->HeaderLength + 7) & ~(UINTN)7;
  UINTN FvEnd  = (UINTN)FvH->FvLength;

  PRINT(L"FindPe32InFv: HdrLen=%d FvLen=0x%x firstFFS=0x%x\n",
        FvH->HeaderLength, FvEnd, Offset);

  while (Offset + 24 <= FvEnd) {
    // 对应 Python: if all(b == 0xFF for b in buf[offset:offset+24])
    BOOLEAN AllFF = TRUE;
    for (UINTN k = 0; k < 24; k++) {
      if (FvBuf[Offset + k] != 0xFF) { AllFF = FALSE; break; }
    }
    if (AllFF) {
      Offset += 8;
      continue;
    }

    UINTN FfsHdrSz = 0;
    UINTN FileSize = GetFfsSizeEx(FvBuf + Offset, &FfsHdrSz);
    UINT8 FfsType  = FvBuf[Offset + 18];

    PRINT(L"  FFS @ 0x%x type=0x%02x size=0x%x hdr=%d\n",
          Offset, FfsType, FileSize, FfsHdrSz);

    if (FfsType == EFI_FV_FILETYPE_FFS_PAD) {
      if (FileSize < FfsHdrSz) break;
      Offset = (Offset + FileSize + 7) & ~(UINTN)7;
      continue;
    }

    if (FileSize < FfsHdrSz || Offset + FileSize > FvEnd) {
      PRINT(L"  FFS size invalid, stop\n");
      break;
    }

    UINT8 *FileData     = FvBuf + Offset + FfsHdrSz;
    UINTN  FileDataSize = FileSize - FfsHdrSz;

    UINT8 *Pe = NULL; UINTN PeSz = 0;
    if (FindPe32InSectionStream(FileData, FileDataSize, &Pe, &PeSz)) {
      *PeOut = Pe; *PeSizeOut = PeSz;
      return TRUE;
    }

    // 对应 Python: offset = align_up(offset + ffs_size, 8)
    Offset = (Offset + FileSize + 7) & ~(UINTN)7;
  }

  return FALSE;
}

// 对应 Python _scan_and_parse
STATIC BOOLEAN
ScanAndFindPe32 (
  IN  UINT8   *Buf,
  IN  UINTN    BufSize,
  OUT UINT8  **PeOut,
  OUT UINTN   *PeSizeOut
  )
{
  // 对应 Python: while off < len(buf) - 0x38
  UINTN Off = 0;

  while (Off + 0x38 < BufSize) {
    // 搜索 '_FVH'
    UINTN Found = (UINTN)-1;
    for (UINTN i = Off; i + 4 <= BufSize; i++) {
      if (Buf[i]   == '_' && Buf[i+1] == 'F' &&
          Buf[i+2] == 'V' && Buf[i+3] == 'H') {
        Found = i;
        break;
      }
    }
    if (Found == (UINTN)-1) break;

    // 对应 Python: fv_start = pos - 0x28; if fv_start < 0: continue
    if (Found < 0x28) {
      Off = Found + 4;
      continue;
    }

    UINT8 *FvStart  = Buf + Found - 0x28;
    UINTN  FvRemain = BufSize - (UINTN)(FvStart - Buf);

    EFI_FIRMWARE_VOLUME_HEADER *H = (EFI_FIRMWARE_VOLUME_HEADER *)FvStart;

    // 对应 Python 校验条件
    if (H->HeaderLength >= 0x48    &&
        H->HeaderLength <= 0x200   &&
        H->FvLength > (UINT64)H->HeaderLength &&
        H->FvLength <= (UINT64)FvRemain) {

      PRINT(L"ScanAndFindPe32: FV @ buf+0x%x FvLen=0x%lx HdrLen=%d\n",
            (UINTN)(FvStart - Buf), H->FvLength, H->HeaderLength);

      UINT8 *Pe = NULL; UINTN PeSz = 0;
      if (FindPe32InFv(FvStart, (UINTN)H->FvLength, &Pe, &PeSz)) {
        *PeOut = Pe; *PeSizeOut = PeSz;
        return TRUE;
      }
    }

    Off = Found + 4;
  }

  return FALSE;
}

STATIC BOOLEAN
FindPe32InSectionStream (
  IN  UINT8   *Buf,
  IN  UINTN    BufSize,
  OUT UINT8  **PeOut,
  OUT UINTN   *PeSizeOut
  )
{
  UINTN Offset = 0;

  while (Offset + 4 <= BufSize) {
    Offset = (Offset + 3) & ~(UINTN)3;
    if (Offset + 4 > BufSize) break;

    UINTN SecHdrSize = 0;
    UINTN SecSize    = GetSectionSizeEx(Buf + Offset, &SecHdrSize);
    UINT8 SecType    = Buf[Offset + 3];

    PRINT(L"Section @ 0x%x type=0x%02x size=0x%x hdr=%d\n",
          Offset, SecType, SecSize, SecHdrSize);

    if (SecSize < SecHdrSize || SecSize == 0 || Offset + SecSize > BufSize) {
      PRINT(L"  Section size invalid, stop\n");
      break;
    }

    UINT8 *SecData     = Buf + Offset + SecHdrSize;
    UINTN  SecDataSize = SecSize - SecHdrSize;

    switch (SecType) {

    case EFI_SECTION_PE32:
    case EFI_SECTION_TE:
    {
      UINT8 *Copy = AllocatePool(SecDataSize);
      if (!Copy) return FALSE;
      CopyMem(Copy, SecData, SecDataSize);
      *PeOut = Copy; *PeSizeOut = SecDataSize;
      PRINT(L"  Found PE/TE %d bytes\n", SecDataSize);
      return TRUE;
    }

    case 0x01: // EFI_SECTION_COMPRESSION
    {
      if (SecDataSize < 5) break;
#ifndef DISABLE_PRINT
      UINT32 UncompLen = *(UINT32 *)SecData;
#endif
      UINT8  CompType  = SecData[4];
      UINT8 *CompData  = SecData + 5;
      UINTN  CompLen   = SecDataSize - 5;

      PRINT(L"  COMPRESSION type=0x%02x uncomp=%u comp=%u\n",
            CompType, UncompLen, CompLen);

      if (CompType == EFI_NOT_COMPRESSED) {
        UINTN CompDataOff     = (UINTN)(CompData - Buf);
        UINTN CompDataOffAlgn = (CompDataOff + 3) & ~(UINTN)3;
        UINTN Skip            = CompDataOffAlgn - CompDataOff;
        if (CompLen > Skip) {
          UINT8 *Pe = NULL; UINTN PeSz = 0;
          if (FindPe32InSectionStream(CompData + Skip, CompLen - Skip, &Pe, &PeSz)) {
            *PeOut = Pe; *PeSizeOut = PeSz; return TRUE;
          }
        }
      } else if (CompType == EFI_STANDARD_COMPRESSION) {
        UINT32 DestSize = 0, ScratchSize = 0;
        if (EFI_ERROR(LzmaUefiDecompressGetInfo(
                CompData, (UINT32)CompLen, &DestSize, &ScratchSize))) break;

        UINT8 *Scratch = AllocatePool(ScratchSize);
        UINT8 *Dest    = AllocatePool(DestSize);
        if (!Scratch || !Dest) {
          if (Scratch) FreePool(Scratch);
          if (Dest)    FreePool(Dest);
          break;
        }
        EFI_STATUS St = LzmaUefiDecompress(
                CompData, (UINT32)CompLen, Dest, Scratch);
        FreePool(Scratch);
        if (EFI_ERROR(St)) { FreePool(Dest); break; }

        PRINT(L"  COMPRESSION LZMA decompressed %u bytes\n", DestSize);

        UINT8 *Pe = NULL; UINTN PeSz = 0;
        if (FindPe32InSectionStream(Dest, DestSize, &Pe, &PeSz)) {
          FreePool(Dest); *PeOut = Pe; *PeSizeOut = PeSz; return TRUE;
        }
        if (ScanAndFindPe32(Dest, DestSize, &Pe, &PeSz)) {
          FreePool(Dest); *PeOut = Pe; *PeSizeOut = PeSz; return TRUE;
        }
        FreePool(Dest);
      }
      break;
    }

    case EFI_SECTION_GUID_DEFINED:
    {
      if (SecDataSize < 20) break;
      EFI_GUID *Guid         = (EFI_GUID *)SecData;
      UINT16    DataOffField = *(UINT16 *)(SecData + 16);
      UINT8    *InnerData    = Buf + Offset + DataOffField;
      UINTN     InnerSize    = SecSize - DataOffField;

      PRINT(L"  GUID_DEFINED: DataOffset=%d InnerSize=%d\n",
            DataOffField, InnerSize);

      if (InnerData + InnerSize > Buf + BufSize) break;

      if (CompareGuid(Guid, &mLzmaGuid)) {
        PRINT(L"  GUID LZMA\n");
        UINT32 DestSize = 0, ScratchSize = 0;
        if (EFI_ERROR(LzmaUefiDecompressGetInfo(
                InnerData, (UINT32)InnerSize, &DestSize, &ScratchSize))) break;

        UINT8 *Scratch = AllocatePool(ScratchSize);
        UINT8 *Dest    = AllocatePool(DestSize);
        if (!Scratch || !Dest) {
          if (Scratch) FreePool(Scratch);
          if (Dest)    FreePool(Dest);
          break;
        }
        EFI_STATUS St = LzmaUefiDecompress(
                InnerData, (UINT32)InnerSize, Dest, Scratch);
        FreePool(Scratch);
        if (EFI_ERROR(St)) { FreePool(Dest); break; }

        PRINT(L"  GUID LZMA decompressed %u bytes\n", DestSize);

        UINT8 *Pe = NULL; UINTN PeSz = 0;
        if (FindPe32InSectionStream(Dest, DestSize, &Pe, &PeSz)) {
          FreePool(Dest); *PeOut = Pe; *PeSizeOut = PeSz; return TRUE;
        }
        if (ScanAndFindPe32(Dest, DestSize, &Pe, &PeSz)) {
          FreePool(Dest); *PeOut = Pe; *PeSizeOut = PeSz; return TRUE;
        }
        FreePool(Dest);

      } else {
        UINTN InnerStart      = Offset + DataOffField;
        UINTN InnerStartAlgn  = (InnerStart + 3) & ~(UINTN)3;
        UINTN InnerEnd        = Offset + SecSize;
        if (InnerStartAlgn < InnerEnd) {
          UINT8 *Pe = NULL; UINTN PeSz = 0;
          if (FindPe32InSectionStream(Buf + InnerStartAlgn,
                                      InnerEnd - InnerStartAlgn,
                                      &Pe, &PeSz)) {
            *PeOut = Pe; *PeSizeOut = PeSz; return TRUE;
          }
        }
      }
      break;
    }

    case EFI_SECTION_FIRMWARE_VOLUME_IMAGE: // 0x17
    {
      PRINT(L"  FV_IMAGE section, scanning %d bytes\n", SecDataSize);
      UINT8 *Pe = NULL; UINTN PeSz = 0;
      if (ScanAndFindPe32(SecData, SecDataSize, &Pe, &PeSz)) {
        *PeOut = Pe; *PeSizeOut = PeSz; return TRUE;
      }
      break;
    }

    default:
      break;
    }
    Offset += SecSize;
  }

  return FALSE;
}

EFI_STATUS
GetActiveSlot (Slot *ActiveSlot);

// ─── 主入口：从 abl 分区加载 PE 镜像 ────────────────────────────
VOID
LoadAblFromPartition (CHAR8 **OutBuffer, UINT32 *OutSize)
{
  *OutBuffer = NULL;
  *OutSize   = 0;

  // 获取当前活跃 Slot
  Slot ActiveSlot = {{0}};
  EFI_STATUS Status = GetActiveSlot (&ActiveSlot);
  if (EFI_ERROR (Status)) {
    PRINT(L"LoadAblFromPartition: GetActiveSlot failed: %r\n", Status);
    return;
  }

  // 拼接分区名，例如 "abl_a"
  CHAR16 PartName[MAX_GPT_NAME_SIZE];
  StrnCpyS (PartName, MAX_GPT_NAME_SIZE,
            L"abl", StrLen (L"abl"));
  StrnCatS (PartName, MAX_GPT_NAME_SIZE,
            ActiveSlot.Suffix, StrLen (ActiveSlot.Suffix));

  // 读取整个分区
  VOID  *PartBuf  = NULL;
  UINTN  PartSize = 0;
  Status = ReadEntirePartition (PartName, &PartBuf, &PartSize);
  if (EFI_ERROR (Status)) {
    PRINT(L"LoadAblFromPartition: ReadEntirePartition failed: %r\n", Status);
    return;
  }

  // 查找固件卷 FV
  UINTN  FvSize = 0;
  UINT8 *FvPtr  = FindFirmwareVolume ((UINT8 *)PartBuf, PartSize, &FvSize);
  if (FvPtr == NULL) {
    PRINT(L"LoadAblFromPartition: FV not found\n");
    FreePool (PartBuf);
    return;
  }
  PRINT(L"LoadAblFromPartition: FV @ offset 0x%x size 0x%x\n",
         (UINTN)(FvPtr - (UINT8 *)PartBuf), FvSize);

  // 在 FV 中查找 PE32/TE
  UINT8 *PePtr  = NULL;
  UINTN  PeSize = 0;
  if (!FindPe32InFv (FvPtr, FvSize, &PePtr, &PeSize)) {
    PRINT(L"LoadAblFromPartition: PE32/TE not found\n");
    FreePool (PartBuf);
    return;
  }
  PRINT(L"LoadAblFromPartition: PE/TE size 0x%x\n", PeSize);

  // 拷贝到独立 buffer
  CHAR8 *Buf = AllocatePool (PeSize);
  if (Buf == NULL) {
    PRINT(L"LoadAblFromPartition: AllocatePool failed\n");
    FreePool (PartBuf);
    return;
  }

  CopyMem (Buf, PePtr, PeSize);
  *OutBuffer = Buf;
  *OutSize   = (UINT32)PeSize;

  FreePool (PartBuf);
}
#ifndef UEFI
#define UEFI
#endif
#include "../../../../tools/patchlib.h"
#endif
STATIC VOID LoadIntegratedEfi(VOID){
#ifndef AUTO_PATCH_ABL
    BootEfiImage(dist_ABL_efi,dist_ABL_efi_len);
#else
    CHAR8* buffer;
    UINT32 size;
    LoadAblFromPartition(&buffer, &size);
    if(!PatchBuffer(buffer, size)) {
        PRINT(L"LoadIntegratedEfi: Failed to patch buffer\n");
        FreePool(buffer);
        return;
    } 
    BootEfiImage(buffer, size);
#endif
}
#endif
EFI_STATUS
ReadAllowUnlockValue (UINT32 *IsAllowUnlock);
EFI_STATUS EFIAPI  __attribute__ ( (no_sanitize ("safe-stack")))
LinuxLoaderEntry (IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable)
{

  EFI_STATUS Status;
  UINT32 IsAllowUnlock = FALSE;

   /* Update stack check guard with random value for better security */
  /* SilentMode Boot */
  /* MultiSlot Boot */
  /* Flashless Boot */
  EFI_MEM_CARDINFO_PROTOCOL *CardInfo = NULL;
  /* set ROT, BootState and VBH only once per boot*/

  /* RED = entry point reached */

  DEBUG ((EFI_D_INFO, "Loader Build Info: %a %a\n", __DATE__, __TIME__));
  DEBUG ((EFI_D_VERBOSE, "LinuxLoader Load Address to debug ABL: 0x%llx\n",
         (UINTN)LinuxLoaderEntry & (~ (0xFFF))));
  DEBUG ((EFI_D_VERBOSE, "LinuxLoaderEntry Address: 0x%llx\n",
         (UINTN)LinuxLoaderEntry));

  Status = InitThreadUnsafeStack ();

  if (Status != EFI_SUCCESS) {
    DEBUG ((EFI_D_ERROR, "Unable to Allocate memory for Unsafe Stack: %r\n",
            Status));
    goto stack_guard_update_default;
  }


  /* Check if memory card is present; goto flashless if not */
  Status = gBS->LocateProtocol (&gEfiMemCardInfoProtocolGuid, NULL,
                                  (VOID **)&CardInfo);

  // Initialize verified boot & Read Device Info
  Status = DeviceInfoInit ();
  if (Status != EFI_SUCCESS) {
    DEBUG ((EFI_D_ERROR, "Initialize the device info failed: %r\n", Status));
    goto stack_guard_update_default;
  }

  Status = EnumeratePartitions ();

  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "LinuxLoader: Could not enumerate partitions: %r\n",
            Status));
    goto stack_guard_update_default;
  }


  UpdatePartitionEntries ();
  /*Check for multislot boot support*/
#ifndef TEST_ADAPTER
    Status = ReadAllowUnlockValue (&IsAllowUnlock);
#else
    IsAllowUnlock = TRUE; // For test adapter, directly set allow unlock to true to enter fastboot
    Status = EFI_SUCCESS;
#endif
  if (Status != EFI_SUCCESS|| !IsAllowUnlock) {
    DEBUG ((EFI_D_ERROR, "Unable to read allow unlock value: %r\n", Status));
#ifndef TEST_ADAPTER
    LoadIntegratedEfi();
 #endif
    return EFI_SUCCESS;
  }

  //wait for 5 sec for key press
  //Print(L"Press Volume Down key to enter Fastboot mode, waiting for 3 seconds into Normal mode...\n");
  //Print(L"Press Volume Up key to enter Normal mode\n");
  INT8 KeyStatus = WaitForVolumeDownKey (3000);
  if(KeyStatus == 1) {
    Print(L"Volume Down key detected, entering Fastboot mode...\n");
  } else {
    DEBUG ((EFI_D_INFO, "No key detected, proceeding with normal boot...\n"));
#ifndef TEST_ADAPTER
    LoadIntegratedEfi();
#endif
    return EFI_SUCCESS;
   }
  FindPtnActiveSlot ();
  

  BootIntoFastboot = TRUE;

  SetDefaultAudioFw ();


#ifdef AUTO_VIRT_ABL
  DEBUG ((EFI_D_INFO, "Rebooting the device.\n"));
  RebootDevice (NORMAL_MODE);
#endif
  DEBUG ((EFI_D_INFO, "Launching fastboot\n"));
  Status = FastbootInitialize ();
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "Failed to Launch Fastboot App: %d\n", Status));
    goto stack_guard_update_default;
  }

stack_guard_update_default:
  /*Update stack check guard with defualt value then return*/
  __stack_chk_guard = DEFAULT_STACK_CHK_GUARD;

  DeInitThreadUnsafeStack ();

  return Status;
}
