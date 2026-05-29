/* Copyright (c) 2019-2021, The Linux Foundation. All rights reserved.
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
 * Changes from Qualcomm Technologies, Inc. are provided under the
 * following license:
 *
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
*/
#if HIBERNATION_SUPPORT_NO_AES

#include <Library/DeviceInfo.h>
#include <Library/DrawUI.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PartitionTableUpdate.h>
#include <Library/ShutdownServices.h>
#include <Library/StackCanary.h>
#include "Hibernation.h"
#include "BootStats.h"
#include <Library/ThreadStack.h>
#include <Library/DxeServicesTableLib.h>
#include <VerifiedBoot.h>
#include <Protocol/EFIKernelInterface.h>
#if HIBERNATION_SUPPORT_AES
#include <Library/aes/aes_public.h>
#include <Library/lz4/lib/lz4.h>
#include <Protocol/EFIQseecom.h>
#include "SmciInvokeUtils.h"
#include "SmciLowPowerKeyMgr.h"
#include <Protocol/EFIScm.h>
#endif
#include "KeymasterClient.h"

#define IS_ZERO_PFN(pfn) ((pfn) & ((UINT64) 1 << 63))
#define IS_COMPRESS(flag) ((flag) & ( 1 << 4 ))
#define BUG(Fmt, ...) {\
                printf ("Fatal error " Fmt, ##__VA_ARGS__); \
                while (1); \
       }

#define ALIGN_1GB(address) address &= ~((1 << 30) - 1)
#define ALIGN_2MB(address) address &= ~((1 << 21) - 1)

/* Reserved some free memory for UEFI use */
#define RESERVE_FREE_SIZE 1024 * 1024 * 10
typedef struct FreeRanges {
        UINT64 Start;
        UINT64 End;
}FreeRanges;

#if HIBERNATION_SUPPORT_AES
#if HIBERNATION_TZ_ENCRYPTION
#define NUM_CORES 4
#define NUM_SILVER_CORES 2
#else
#define NUM_CORES 8
#define NUM_SILVER_CORES 4
#endif
#define NUM_PAGES_PER_GOLD_CORE ((NrCopyPages / 54) * 9)
#define NUM_PAGES_PER_SILVER_CORE ((NrCopyPages / 54) * 4)

static struct DecryptParam *Dp;
static VOID *Authslot;
static UINT32 AuthslotStart;
static CHAR8 *Authtags;
static VOID *AuthCur[NUM_CORES];
static VOID *TempOut[NUM_CORES];
static UINT8 UnwrappedKey[32];
#define QSEECOM_ALIGN_SIZE      0x40
#define QSEECOM_ALIGN_MASK      (QSEECOM_ALIGN_SIZE - 1)
#define QSEECOM_ALIGN(x)        \
        ((x + QSEECOM_ALIGN_MASK) & (~QSEECOM_ALIGN_MASK))

#if HIBERNATION_TZ_ENCRYPTION
Object ClientEnvObj = Object_NULL;
Object AppClientObj = Object_NULL;
#endif
#else
#define NUM_CORES 1
#define NUM_SILVER_CORES 0
#define NUM_PAGES_PER_GOLD_CORE 0
#define NUM_PAGES_PER_SILVER_CORE 0
#endif

/* Holds free memory ranges read from UEFI memory map */
static struct FreeRanges FreeRangeBuf[100];
static INT32 FreeRangeCount;

typedef struct MappedRange {
        UINT64 Start, End;
        struct MappedRange * Next;
}MappedRange;

/* number of data pages to be copied from swap */
static UINT32 NrCopyPages;
/* number of meta pages or pages which hold pfn indexes */
static UINT32 NrMetaPages;
/* number of image kernel pages bounced due to conflict with UEFI */
static UINT64 BouncedPages;
/* zero page pfn */
static UINT64 ZeroPagePfn;

static struct ArchHibernateHdr *ResumeHdr;

typedef struct PfnBlock {
        UINT64 BasePfn;
        UINT32 AvailablePfns;
}PfnBlock;

typedef struct KernelPfnIterator {
        UINT64 *PfnArray;
        INT32 CurIndex;
        INT32 MaxIndex;
        struct PfnBlock CurBlock;
}KernelPfnIterator;
static struct KernelPfnIterator KernelPfnIteratorObj;

static struct SwsUspHeader *SwsuspHeader;
/*
 * Bounce Pages - During the copy of pages from snapshot image to
 * RAM, certain pages can conflicts with concurrently running UEFI/ABL
 * pages. These pages are copied temporarily to bounce pages. And
 * during later stage, upon exit from UEFI boot services, these
 * bounced pages are copied to their real destination. BouncePfnEntry
 * is used to store the location of temporary bounce pages and their
 * real destination.
 */
typedef struct BouncePfnEntry {
        UINT64 DstPfn;
        UINT64 SrcPfn;
}BouncePfnEntry;

/*
 * Size of the buffer where disk IO is performed.If any kernel pages are
 * destined to be here, they will be bounced.
 */
#define DISK_BUFFER_SIZE 64 * 1024 * 1024
#define DISK_BUFFER_PAGES (DISK_BUFFER_SIZE / PAGE_SIZE)

#define OUT_OF_MEMORY -1

#define BOUNCE_TABLE_ENTRY_SIZE sizeof(struct BouncePfnEntry)
#define ENTRIES_PER_TABLE (PAGE_SIZE / BOUNCE_TABLE_ENTRY_SIZE) - 1

/* Number pf pages occupied by the header, swap info and first swap_map page */
#define HDR_SWP_INFO_NUM_PAGES 4

/*
 * Bounce Tables -  bounced pfn entries are stored in bounced tables.
 * Bounce tables are discontinuous pages linked by the last element
 * of the page. Bounced table are allocated using unused pfn allocator.
 *
 *       ---------                    ---------
 * 0   | dst | src |    ----->  0   | dst | src |
 * 1   | dst | src |    |       1   | dst | src |
 * 2   | dst | src |    |       2   | dst | src |
 * .   |           |    |       .   |           |
 * .   |           |    |       .   |           |
 * 256 | addr|     |-----       256 |addr |     |------>
 *       ---------                    ---------
 */
typedef struct BounceTable {
        struct BouncePfnEntry BounceEntry[ENTRIES_PER_TABLE];
        UINT64 NextBounceTable;
        UINT64 Padding;
}BounceTable;

typedef struct BounceTableIterator {
        struct BounceTable *FirstTable;
        struct BounceTable *CurTable;
        /* next available free table entry */
        INT32 CurIndex;
}BounceTableIterator;

#if HIBERNATION_SUPPORT_AES
typedef struct Secs2dTaHandle {
        QCOM_QSEECOM_PROTOCOL *QseeComProtocol;
        UINT32 AppId;
}Secs2dTaHandle;
static INT32 InitAesDecrypt (VOID);
GcmAesStruct Ctx[8];
#endif

typedef struct RestoreInfo {
        UINT64 *KernelPfnIndexes;
        UINT64 Offset;
        VOID *DiskReadBuffer;
        Semaphore* Sem;
        EFI_STATUS Status;
        UINT32 NumPages;
#if HIBERNATION_SUPPORT_AES
        CHAR8 *Authtags;
        VOID *TempOut;
        VOID *AuthCur;
        UINT32 ThreadId;
        UINT8 Iv[12];
        struct DecryptInfo *DecInfo;
        struct CmpBuffer *CmpBuf;
#endif
}RestoreInfo;

struct BounceTableIterator TableIterator;

UINT64 *KernelPfnIndexes;
UINT64 RelocateAddress;
STATIC EFI_KERNEL_PROTOCOL  *KernIntf = NULL;

#define PFN_INDEXES_PER_PAGE 512
/* Final entry is used to link swap_map pages together */
#define ENTRIES_PER_SWAPMAP_PAGE (PFN_INDEXES_PER_PAGE - 1)

#define SWAP_INFO_OFFSET        (SwsuspHeader->Image + 1)
#define FIRST_PFN_INDEX_OFFSET  (SWAP_INFO_OFFSET + 1)

/*
 * target_addr  : address where page allocation is needed
 *
 * return       : 1 if address falls in free range
 *                0 if address is not in free range
 */
static INT32 CheckFreeRanges (UINT64 TargetAddr)
{
        INT32 Iter = 0;
        while (Iter < FreeRangeCount) {
                if (TargetAddr >= FreeRangeBuf[Iter].Start &&
                        TargetAddr < FreeRangeBuf[Iter].End)
                        return 1;
                Iter++;
        }
        return 0;
}

static INT32 EnableAllCores ()
{
        INT32 Iter = 0;
        UINT32 NumCpus;
        UINT32 Status;

        DEBUG ((EFI_D_VERBOSE, "Getting count of Max CPUs\n"));
        NumCpus = KernIntf->MpCpu->MpcoreGetMaxCpuCount ();
        DEBUG ((EFI_D_VERBOSE, "Available Cores for hibernation: %d\n",
                NumCpus));

        while (Iter < NumCpus) {
                DEBUG ((EFI_D_VERBOSE, "Getting Status of core: %d\n", Iter));
                Status = KernIntf->MpCpu->MpcoreIsCpuActive (Iter);
                DEBUG ((EFI_D_VERBOSE, "Core: %d, Status: %d\n", Iter, Status));
                if (!Status) {
                        DEBUG ((EFI_D_VERBOSE, "Enabling Core: %d\n", Iter));
                        KernIntf->MpCpu->MpcoreInitDeferredCores (1 << Iter);
                        KernIntf->Thread->ThreadSleep (10);
                }
                Iter++;
        }

        Iter = 0;
        while (Iter < NumCpus) {
                Status = KernIntf->MpCpu->MpcoreIsCpuActive (Iter);
                DEBUG ((EFI_D_VERBOSE, "Core: %d, Status: %d\n", Iter, Status));
                Iter++;
        }

        return 0;
}

static INT32 MemCmp (CONST VOID *S1, CONST VOID *S2, INT32 MemSize)
{
        CONST UINT8 *Us1 = S1;
        CONST UINT8 *Us2 = S2;

        if (MemSize == 0 ) {
                return 0;
        }

        while (MemSize--) {
                if (*Us1++ ^ *Us2++) {
                        return 1;
                }
        }
        return 0;
}

static VOID CopyPage (UINT64 SrcPfn, UINT64 DstPfn)
{
        UINT64 *Src = (UINT64*)(SrcPfn << PAGE_SHIFT);
        UINT64 *Dst = (UINT64*)(DstPfn << PAGE_SHIFT);

        gBS->CopyMem (Dst, Src, PAGE_SIZE);
}

static VOID InitKernelPfnIterator (UINT64 *Array)
{
        struct KernelPfnIterator *Iter = &KernelPfnIteratorObj;
        Iter->PfnArray = Array;
        Iter->MaxIndex = NrCopyPages;
}

static INT32 FindNextAvailableBlock (struct KernelPfnIterator *Iter)
{
        UINT32 AvailablePfns;

        do {
                UINT64 CurPfn, NextPfn;
                Iter->CurIndex++;
                if (Iter->CurIndex >= Iter->MaxIndex) {
                        BUG ("index maxed out. Line %d\n", __LINE__);
                }
                CurPfn = Iter->PfnArray[Iter->CurIndex];
                NextPfn = Iter->PfnArray[Iter->CurIndex + 1];
                AvailablePfns = NextPfn - CurPfn - 1;
        } while (!AvailablePfns);

        Iter->CurBlock.BasePfn = Iter->PfnArray[Iter->CurIndex];
        Iter->CurBlock.AvailablePfns = AvailablePfns;
        return 0;
}

static UINT64 GetUnusedKernelPfn (VOID)
{
        struct KernelPfnIterator *Iter = &KernelPfnIteratorObj;

        if (!Iter->CurBlock.AvailablePfns) {
                FindNextAvailableBlock (Iter);
        }

        Iter->CurBlock.AvailablePfns--;
        return ++Iter->CurBlock.BasePfn;
}

/*
 * get a pfn which is unused by kernel and UEFI.
 *
 * unused pfns are pnfs which doesn't overlap with image kernel pages
 * or UEFI pages. These pfns are used for bounce pages, bounce tables
 * and relocation code.
 */
static UINT64 GetUnusedPfn ()
{
        UINT64 Pfn;

        do {
                Pfn = GetUnusedKernelPfn ();
        } while (!CheckFreeRanges (Pfn << PAGE_SHIFT));

        return Pfn;
}

/*
 * Preallocation is done for performance reason. We want to map memory
 * as big as possible. So that UEFI can create bigger page table mappings.
 * We have seen mapping single page is taking time in terms of few ms.
 * But we cannot preallocate every free page, becasue that causes allocation
 * failures for UEFI. Hence allocate most of the free pages but some(10MB)
 * are kept unallocated for UEFI to use. If kernel has any destined pages in
 * this region, that will be bounced.
 */
static INT32 PreallocateFreeRanges (VOID)
{
        INT32 Iter = 0, Ret;
        INT32 ReservationDone = 0;
        UINT64 AllocAddr, RangeSize;
        UINT64 NumPages;

        for (Iter = FreeRangeCount - 1; Iter >= 0 ; Iter--) {
                RangeSize = FreeRangeBuf[Iter].End - FreeRangeBuf[Iter].Start;
                if ((!ReservationDone)
                    &&
                    (RangeSize > RESERVE_FREE_SIZE)) {
                        /*
                         * We have more buffer. Remove reserved buf and allocate
                         * rest in the range.
                         */
                        ReservationDone = 1;
                        AllocAddr = FreeRangeBuf[Iter].Start +
                                        RESERVE_FREE_SIZE;
                        RangeSize -=  RESERVE_FREE_SIZE;
                        NumPages = RangeSize / PAGE_SIZE;
                        printf ("Reserved range = 0x%lx - 0x%lx\n",
                                FreeRangeBuf[Iter].Start, AllocAddr - 1);
                        /* Modify the free range start */
                        FreeRangeBuf[Iter].Start = AllocAddr;
                } else {
                        AllocAddr = FreeRangeBuf[Iter].Start;
                        NumPages = RangeSize / PAGE_SIZE;
                }

                Ret = gBS->AllocatePages (AllocateAddress, EfiBootServicesData,
                               NumPages, &AllocAddr);
                if (Ret) {
                        printf (
                        "WARN: Prealloc falied LINE %d alloc_addr = 0x%lx\n",
                         __LINE__, AllocAddr);
                        return Ret;
                }
        }
        return 0;
}

/* Assumption: There is no overlap in the regions */
static struct MappedRange * AddRangeSorted (struct MappedRange * Head,
                                UINT64 Start, UINT64 End)
{
        struct MappedRange * Elem, * P;

        Elem  = AllocateZeroPool (sizeof (struct MappedRange));
        if (!Elem) {
                printf ("Failed to AllocateZeroPool %d\n", __LINE__);
                return NULL;
        }
        Elem->Start = Start;
        Elem->End = End;
        Elem->Next = NULL;

        if (Head == NULL) {
                return Elem;
        }

        if (Start <= Head->Start) {
                Elem->Next = Head;
                return Elem;
        }

        P = Head;
        while ((P->Next != NULL)
               &&
               (P->Next->Start < Start)) {
                P = P->Next;
        }

        Elem->Next = P->Next;
        P->Next = Elem;

        return Head;
}

/*
 * Get the UEFI memory map to collect ranges of
 * memory of type EfiConventional
 */
static INT32 GetConventionalMemoryRanges (VOID)
{
        EFI_MEMORY_DESCRIPTOR   *MemMap;
        EFI_MEMORY_DESCRIPTOR   *MemMapPtr;
        UINTN                   MemMapSize;
        UINTN                   MapKey, DescriptorSize;
        UINTN                   Index;
        UINT32                  DescriptorVersion;
        EFI_STATUS              Status;
        INT32 IndexB = 0;

        MemMapSize = 0;
        MemMap     = NULL;

        Status = gBS->GetMemoryMap (&MemMapSize, MemMap, &MapKey,
                        &DescriptorSize, &DescriptorVersion);
        if (Status != EFI_BUFFER_TOO_SMALL) {
                printf ("ERROR: Undefined response get memory map\n");
                return -1;
        }
        if (CHECK_ADD64 (MemMapSize, EFI_PAGE_SIZE)) {
                printf ("ERROR: integer Overflow while adding additional"
                                        "memory to MemMapSize");
                return -1;
        }
        MemMapSize = MemMapSize + EFI_PAGE_SIZE;
        MemMap = AllocateZeroPool (MemMapSize);
        if (!MemMap) {
                printf ("ERROR: Failed to allocate memory for memory map\n");
                return -1;
        }
        MemMapPtr = MemMap;
        Status = gBS->GetMemoryMap (&MemMapSize, MemMap, &MapKey,
                        &DescriptorSize, &DescriptorVersion);
        if (EFI_ERROR (Status)) {
                printf ("ERROR: Failed to query memory map\n");
                FreePool (MemMapPtr);
                return -1;
        }
        for (Index = 0; Index < MemMapSize / DescriptorSize; Index ++) {
                if (MemMap->Type == EfiConventionalMemory) {
                        FreeRangeBuf[IndexB].Start = MemMap->PhysicalStart;
                        FreeRangeBuf[IndexB].End =
                                MemMap->PhysicalStart +
                                        MemMap->NumberOfPages * PAGE_SIZE;
                        printf ("Free Range 0x%lx --- 0x%lx\n",
                                FreeRangeBuf[IndexB].Start,
                                FreeRangeBuf[IndexB].End);
                        IndexB++;
                }
                MemMap = (EFI_MEMORY_DESCRIPTOR *)
                                ((UINTN)MemMap + DescriptorSize);
        }
        FreeRangeCount = IndexB;
        FreePool (MemMapPtr);
        return 0;
}

typedef struct PartitionDetails {
        EFI_BLOCK_IO_PROTOCOL *BlockIo;
        EFI_HANDLE *Handle;
        INT32 BlocksPerPage;
}PartitionDetails;
static struct PartitionDetails SwapDetails;

static INT32 VerifySwapPartition (VOID)
{
        INT32 Status;
        EFI_BLOCK_IO_PROTOCOL *BlockIo = NULL;
        EFI_HANDLE *Handle = NULL;

        Status = PartitionGetInfo (SWAP_PARTITION_NAME, &BlockIo, &Handle);
        if (Status != EFI_SUCCESS) {
                return Status;
        }

        if (!Handle) {
                printf ("EFI handle for swap partition is corrupted\n");
                return -1;
        }

        if (CHECK_ADD64 (BlockIo->Media->LastBlock, 1)) {
                printf ("Integer overflow while adding LastBlock and 1\n");
                return -1;
        }

        if ((MAX_UINT64 / (BlockIo->Media->LastBlock + 1)) <
                        (UINT64)BlockIo->Media->BlockSize) {
                printf (
                "Integer overflow while multiplying LastBlock and BlockSize\n"
                );
                return -1;
        }

        SwapDetails.BlockIo = BlockIo;
        SwapDetails.Handle = Handle;
        SwapDetails.BlocksPerPage = EFI_PAGE_SIZE / BlockIo->Media->BlockSize;
        return 0;
}

/* We need to remember how much compressed data we need to read. */
#define CMP_HEADER      sizeof(size_t)

/* Number of pages/bytes we'll compress at one time. */
#define CMP_UNC_PAGES   32
#define CMP_UNC_SIZE    (CMP_UNC_PAGES * PAGE_SIZE)

/* Number of pages/bytes we need for compressed data (worst case). */
#define CMP1X_WORST_CMP  CMP_UNC_SIZE + (CMP_UNC_SIZE / 16) + 64 + 3
#define CMP_CMP_SIZE     CMP1X_WORST_CMP + CMP_HEADER

/* Structure used for data decompression */
typedef struct DecryptData {
        INT64 Ret;                           /* return code */
        size_t UNCLen;                       /* uncompressed length */
        size_t CMPLen;                       /* compressed length */
        UINT8 Unc[CMP_UNC_SIZE];             /* uncompressed buffer */
        UINT8 Cmp[CMP_CMP_SIZE];             /* compressed buffer */
}DecryptData;

/* If the previous block remains uncompressed, then
 * DecryptInfo structure contains information regarding previous iterations.
 * RByte    - The number of bytes yet to be read in the upcoming iterations.
 * Pos      - If the previous block hasn't been decompressed, then "Pos"
 *            indicates the number of blocks read in the previous iterations.
 * CMPLen   - If the previous block remains uncompressed, then "CMPLen" holds
 *            the compression length from the previous iteration.
 * TBytes   - Total count of uncompressed bytes.
 * CmpG     - If the previous block remains uncompressed,
 *            then the "CmpG" buffer retains the bytes from previous iterations.
 */
typedef struct DecryptInfo {
    INT64 RByte;
    INT64 Pos;
    INT64 CMPLen;
    INT64 TBytes;
    UINT8 CmpG[CMP_CMP_SIZE];
}DecryptInfo;

/* CmpBuffer serves as a buffer for data
 * following the decompression process,
 * as the data size increases upon decompression.
 */
typedef struct CmpBuffer {
        UINT8 *Buf;                          /* Contains decompressed data */
        INT32 RPos;                          /* Buffer's read position */
        INT32 WPos;                          /* Buffer's write position */
}CmpBuffer;

/* DecInfo is used for handling uncompresse block for Meta Pages */
static struct DecryptInfo DecInfo = {
    .RByte = 0,
    .Pos = 0,
    .CMPLen = 0,
    .TBytes = 0,
};

/* CmpBf holds both the read and write positions of the buffer for meta pages */
static struct CmpBuffer CmpBuf = {
        .RPos = 0,
        .WPos = 0,
};

static INT32 ReadImage (UINT64 Offset, VOID *Buff, INT32 NrPages)
{
        INT32 Status;
        EFI_BLOCK_IO_PROTOCOL *BlockIo = SwapDetails.BlockIo;
        EFI_LBA Lba;

        Lba = Offset * SwapDetails.BlocksPerPage;
        Status = BlockIo->ReadBlocks (BlockIo,
                        BlockIo->Media->MediaId,
                        Lba,
                        EFI_PAGE_SIZE * NrPages,
                        (VOID*)Buff);
        if (Status != EFI_SUCCESS) {
                printf ("Read image failed Line = %d\n", __LINE__);
                return Status;
        }

        return 0;
}

#if HIBERNATION_SUPPORT_AES
static VOID CopyPageToDst (UINT64 SrcPfn, UINT64 DstPfn);
static VOID CopyZeroPageToDst (UINT64 DstPfn);
static INT32 CheckSwapMapPage (UINT64 Offset);
static INT32 DecryptPage (VOID *EncryptData, CHAR8 *Auth, VOID *TempOut,
                          VOID *AuthCurrent, UINT32 ThreadId, UINT8* Iv);
static VOID IncrementIV (UINT8 *Iv, UINT8 Size, UINT64 Val);

/* The InitBlockArr function is reading a compressed unit block array
 * from the disk. This block array is stored at the end of the disk as
 * part of the hibernation image creation process, and this approach
 * is employed for optimization purposes.
 * In this specific example, a 32-page unit is compressed.
 * Here's a representation of the stored block array:
 *
 * |-----|-----|-----|-----|-----|-----|-----|-----|-----|-----|-----|-
 * | 12  | 15  | 22  | 25  | 19  | 17  | 20  | 9   | 26  | 29  | 30  |
 * |-----|-----|-----|-----|-----|-----|-----|-----|-----|-----|-----|-
 *
 * An alternative method is to populate the block array in the bootloader,
 * but this approach introduces a regression.
 */

static INT32 InitBlockArr (UINT8 **BlkArr)
{
        static INT32 BlkSlot;
        static INT64 BlkIndex, BlkPages, BlkBytes;

        BlkSlot = AuthslotStart + Dp->AuthCount ;
        BlkIndex = ((NrCopyPages + NrMetaPages + 1) % CMP_UNC_PAGES) == 0 ?
                ((NrCopyPages + NrMetaPages + 1) / CMP_UNC_PAGES) :
                ((NrCopyPages + NrMetaPages + 1) / CMP_UNC_PAGES + 1);

        BlkBytes = (BlkIndex + 1) * sizeof (UINT8);
        BlkPages = BlkBytes % PAGE_SIZE == 0 ?
                BlkBytes / PAGE_SIZE : BlkBytes / PAGE_SIZE + 1;
        *BlkArr = AllocatePages (BlkPages);
        if (!BlkArr) {
                return -1;
        }

        if (ReadImage (BlkSlot, *BlkArr, BlkPages)) {
                return -1;
        }
        return 0;
}

/* The Decompress_LZ4 function decompresses Nrpages pages from the Buff buffer
 * and stores the decompressed data in the CmpBuf.
 */
static INT32 Decompress_LZ4 (UINT64 Offset, VOID *Buff, INT32 Nrpages,
                             INT32 *Rread, INT32 ThreadId,
                             struct DecryptInfo *DecInfo,
                             struct CmpBuffer *CmpBuf
                             )
{
        struct DecryptData *Data = NULL;
        Data = AllocateZeroPool (sizeof (*Data));
        Data->UNCLen = CMP_UNC_SIZE;
        UINT32 Cnt = 0, K = 0;
        UINT64 TByte = 0;

        /* If the previous block unit has not been decompressed,
         * then the first step is to read the required data and decompress it
         */
        if (DecInfo->RByte != 0) {
                K = 0;
                CopyMem (Data->Cmp, DecInfo->CmpG, (DecInfo->Pos) * PAGE_SIZE);
                while ((( K * PAGE_SIZE) < (DecInfo->RByte))
                                &&
                                (Cnt < Nrpages)) {
                        if ( !CheckSwapMapPage (Offset + Cnt)) {
                            CopyMem (Data->Cmp + (DecInfo->Pos * PAGE_SIZE),
                                           Buff + (Cnt * PAGE_SIZE), PAGE_SIZE);
                            DecInfo->Pos++;
                            K++;
                        }
                        Cnt++;
                }
                DecInfo->Pos = 0, DecInfo->RByte = 0;
                Data->CMPLen = DecInfo->CMPLen;

                Data->Ret = LZ4_decompress_safe ((CONST CHAR8*) (Data->Cmp
                                        + CMP_HEADER),
                               (CHAR8*) Data->Unc, Data->CMPLen, Data->UNCLen);
                if (Data->Ret > 0) {
                        TByte += Data->Ret;
                        CopyMem (CmpBuf->Buf, Data->Unc, Data->Ret);
                        CmpBuf->WPos += Data->Ret;
                } else {
                        printf (
                             "The decompression process for the data in thread \
                              %d failed at line %d \n", ThreadId, __LINE__
                        );
                        return -1;
                }
        }

        while (Cnt < Nrpages) {
                Data->UNCLen = CMP_UNC_SIZE;
                if (CheckSwapMapPage (Offset + Cnt)) {
                    Cnt++;
                    continue;
                }
                Data->CMPLen = *(size_t *) (Buff + (Cnt * PAGE_SIZE));
                K = 0, DecInfo->RByte = 0;
                while (((K * PAGE_SIZE) < (CMP_HEADER + Data->CMPLen))
                                &&
                                (Cnt < Nrpages)) {
                        if ( !CheckSwapMapPage (Offset + Cnt)) {
                                CopyMem (Data->Cmp + (K * PAGE_SIZE),
                                                Buff + (Cnt * PAGE_SIZE),
                                                PAGE_SIZE);
                                K++;
                        }
                        Cnt++;
                }
                if (K * PAGE_SIZE < (CMP_HEADER + Data->CMPLen)) {
                        DecInfo->RByte = CMP_HEADER + Data->CMPLen
                                         - (K * PAGE_SIZE);
                        DecInfo->Pos = K;
                        DecInfo->CMPLen = Data->CMPLen;
                        CopyMem (DecInfo->CmpG, Data->Cmp, K * PAGE_SIZE);
                }
                if (DecInfo->RByte == 0) {
                        Data->Ret =  LZ4_decompress_safe ((CONST CHAR8*)
                                        (Data->Cmp + CMP_HEADER),
                                        (CHAR8*) Data->Unc,
                                        Data->CMPLen, Data->UNCLen);
                        if (Data->Ret > 0) {
                                TByte += Data->Ret;
                                CopyMem (CmpBuf->Buf + CmpBuf->WPos, Data->Unc,
                                                Data->Ret);
                                CmpBuf->WPos += Data->Ret;
                        } else {
                                printf (
                                     "The decompression process for the data \
                                      in thread %d failed at line %d \n",
                                      ThreadId, __LINE__
                                );
                                return -1;
                        }
                }
        }
        if ((DecInfo->RByte == 0)
                        &&
                        ((*Rread) > TByte))
        {
                *Rread = TByte;
        }

        CmpBuf->RPos += *Rread;
        DecInfo->TBytes += TByte;
        return 0;
}

/* The MetaDecompress function performs decryption
 * and decompression of meta pages.
 */
static INT32 MetaDecompress (UINT64 *Soffset, VOID *Buff, INT32 NrPages,
                             CHAR8 **RAuthtags, VOID *RTempOut, VOID *RAuthCur,
                             UINT32 RThreadId, UINT8 RIv[12],
                             struct DecryptInfo *DecInfo,
                             struct CmpBuffer *CmpBuf
                             )
{
        INT32 Ret, Loop = 0;
        INT32 Len = 0, Rread;
        INT32 SMPage = 0;

        if ((CmpBuf->WPos - CmpBuf->RPos) < NrPages * PAGE_SIZE) {
                SMPage = (*Soffset - 1 + NrPages) / PFN_INDEXES_PER_PAGE;
                if ( DISK_BUFFER_PAGES < NrPages + SMPage
                                ||
                                ReadImage (*Soffset, Buff, NrPages + SMPage))
                {
                    printf ("Thread %d failed to read Line %d\n",
                                    RThreadId, __LINE__);
                    return -1;
                }
                Len = CmpBuf->WPos - CmpBuf->RPos;
                Rread = (NrPages * PAGE_SIZE) - Len;
                CmpBuf->RPos = 0, CmpBuf->WPos = 0;
                while (NrPages + SMPage - Loop > 0) {
                        if ( !CheckSwapMapPage ( Loop + *Soffset )) {
                                if (DecryptPage ((VOID *) (Buff
                                                           + Loop * PAGE_SIZE),
                                                        Authtags, RTempOut,
                                                        RAuthCur, RThreadId,
                                                        RIv))
                                {
                                       printf (
                                            "The decryption process for the \
                                             meta pages in thread %d failed at \
                                             line %d\n", RThreadId, __LINE__
                                       );

                                        return -1;
                                }
                                Authtags += Dp->Authsize;
                        }
                        Loop++;
                }
                if (SwsuspHeader->Flags & ( 1 << 4 )) {
                        Ret = Decompress_LZ4 (*Soffset, Buff, NrPages + SMPage,
                                        &Rread, RThreadId, DecInfo, CmpBuf);
                        printf ("Decompressing using LZ4 algorithm\n");
                } else {
                        printf (
                           "ABL only supports the LZ4 decompression algorithm\n"
                        );
                        Ret = -1;
                }
                if (Ret) {
                    return -1;
                }
                CopyMem (Buff, CmpBuf->Buf, NrPages * PAGE_SIZE);
                *Soffset += NrPages + SMPage;
        } else {
                CopyMem (Buff, CmpBuf->Buf + CmpBuf->RPos, NrPages * PAGE_SIZE);
                CmpBuf->RPos += NrPages * PAGE_SIZE;
        }
        return 0;
}

/* The DataDecompress function decrypts, decompresses, and copy the
 * data to the appropriate PfnOffset.
 */
static INT32 DataDecompress (UINT64 *Soffset, VOID *Buff, INT32 NrPages,
                             CHAR8 **RAuthtags, VOID *RTempOut, VOID *RAuthCur,
                             UINT32 RThreadId, UINT8 RIv[12],
                             struct DecryptInfo *DecInfo,
                             struct CmpBuffer *CmpBuf,
                             UINT64 *PfnIndex, UINT64 *KernelPfnIndexes
                             )
{
        UINT64 SrcPfn, DstPfn;
        INT32 Loop = 0, Rread = 0;
        INT32 Ret;

        if (ReadImage (*Soffset, Buff, NrPages)) {
                printf (
                      "Thread %d failed to read Line %d\n", RThreadId, __LINE__
                );
                return -1;
        }
        while (NrPages - Loop > 0) {
                /* skip swap_map pages */
                if ( !CheckSwapMapPage (*Soffset + Loop)) {
                    if (DecryptPage ((VOID *) (Buff + Loop * PAGE_SIZE),
                                            *RAuthtags, RTempOut, RAuthCur,
                                            RThreadId, RIv))
                    {
                           printf (
                                 "The decryption process for the data pages in \
                                  thread %d failed at line %d \n",
                                  RThreadId, __LINE__
                           );
                            return -1;
                    }
                    *RAuthtags += Dp->Authsize;
                }
                Loop++;
        }
        Rread = (NrPages * PAGE_SIZE);
        CmpBuf->RPos = 0, CmpBuf->WPos = 0;
        Ret = Decompress_LZ4 (*Soffset, Buff, NrPages, &Rread, RThreadId,
                        DecInfo, CmpBuf);
        if (Ret) {
            return -1;
        }
        *Soffset += NrPages;
        SrcPfn = (UINT64) (CmpBuf->Buf) >> PAGE_SHIFT;
        Loop = (CmpBuf->WPos) / PAGE_SIZE;
        while (Loop > 0) {
                DstPfn = KernelPfnIndexes[(*PfnIndex)];
                if (IS_ZERO_PFN (DstPfn)) {
                        CopyZeroPageToDst (DstPfn);
                } else {
                        CopyPageToDst (SrcPfn, DstPfn);
                        SrcPfn++;
                        Loop--;
                }
                (*PfnIndex)++;
        }
        return 0;
}
#endif

static INT32 IsCurrentTableFull (struct BounceTableIterator *Bti)
{
        return (Bti->CurIndex == ENTRIES_PER_TABLE);
}

static VOID AllocNextTable (struct BounceTableIterator *Bti)
{
        /* Allocate and chain next bounce table */
        Bti->CurTable->NextBounceTable = GetUnusedPfn () << PAGE_SHIFT;
        Bti->CurTable = (struct BounceTable *)
                                Bti->CurTable->NextBounceTable;
        Bti->CurIndex = 0;
}

static struct BouncePfnEntry * FindNextBounceEntry (VOID)
{
        struct BounceTableIterator *Bti = &TableIterator;

        if (IsCurrentTableFull (Bti)) {
                AllocNextTable (Bti);
        }

        return &Bti->CurTable->BounceEntry[Bti->CurIndex++];
}

static VOID UpdateBounceEntry (UINT64 DstPfn, UINT64 SrcPfn)
{
        struct BouncePfnEntry *Entry;

        Entry = FindNextBounceEntry ();
        Entry->DstPfn = DstPfn;
        Entry->SrcPfn = SrcPfn;
        BouncedPages++;
}

/*
 * Copy page to destination if page is free and is not in reserved area.
 * Bounce page otherwise.
 */
Mutex *mx;
static VOID CopyPageToDst (UINT64 SrcPfn, UINT64 DstPfn)
{
        UINT64 TargetAddr = DstPfn << PAGE_SHIFT;

        if (CheckFreeRanges (TargetAddr)) {
                CopyPage (SrcPfn, DstPfn);
        } else {
                KernIntf->Mutex->MutexAcquire (mx);
                UINT64 BouncePfn = GetUnusedPfn ();
                CopyPage (SrcPfn, BouncePfn);
                UpdateBounceEntry (DstPfn, BouncePfn);
                KernIntf->Mutex->MutexRelease (mx);
        }
}

static VOID CopyZeroPageToDst (UINT64 DstPfn)
{
        UINT64 TargetAddr = DstPfn << PAGE_SHIFT;

        if (CheckFreeRanges (TargetAddr)) {
                CopyPage (ZeroPagePfn, DstPfn);
        } else {
                KernIntf->Mutex->MutexAcquire (mx);
                UpdateBounceEntry (DstPfn, ZeroPagePfn);
                KernIntf->Mutex->MutexRelease (mx);
        }
}

static VOID PrintImageKernelDetails (struct SwsuspInfo *info)
{
        /*TODO: implement printing of kernel details here*/
        return;
}

/*
 * swsusp_header->image points to first swap_map page. From there onwards,
 * swap_map pages are repeated at every PFN_INDEXES_PER_PAGE intervals.
 * This function returns true if offset belongs to a swap_map page.
 */
static INT32 CheckSwapMapPage (UINT64 Offset)
{
        Offset -= SwsuspHeader->Image;
        return (Offset % PFN_INDEXES_PER_PAGE) == 0;
}

VOID InitReadMultiThreadEnv (VOID)
{
        EFI_STATUS Status = EFI_SUCCESS;
        Status = gBS->LocateProtocol (&gEfiKernelProtocolGuid, NULL,
                                      (VOID **)&KernIntf);

        if ((Status != EFI_SUCCESS) ||
            (KernIntf == NULL)) {
                DEBUG ((EFI_D_INFO,
                       "InitReadMultiThreadEnv: Multithread not supported\n"));
                return;
        }
        return;
}

#if HIBERNATION_SUPPORT_AES
static UINT8 IvGlb[12];
#define BYTE_SIZE 8
static VOID IncrementIV (UINT8 *Iv, UINT8 Size, UINT64 Val)
{
        UINT32 Offset;
        UINT64 Num;
        UINT64 Mask = 0xFFUL;

        Offset = Size - 1;
        do {
                Num = (UINT8) Iv[Offset];
                Num += Val;
                Iv[Offset] = Num & Mask;
                Val = (Num > Mask) ? ((Num & ~Mask) >> BYTE_SIZE) : 0;
                Offset--;
        } while (Offset >= 0 &&
                   Val != 0);
}

static INT32 DecryptPage (VOID *EncryptData, CHAR8 *Auth, VOID *TempOut,
                          VOID *AuthCurrent, UINT32 ThreadId, UINT8* Iv)
{
        SW_CipherEncryptDir Dir = SW_CIPHER_DECRYPT;
        SW_CipherModeType Mode = SW_CIPHER_MODE_GCM;
        IovecListType   ioVecIn;
        IovecListType   ioVecOut;
        IovecType       IovecIn;
        IovecType       IovecOut;
        UINT32 Ret;

        ioVecIn.size = 1;
        ioVecIn.iov = &IovecIn;
        ioVecIn.iov[0].dwLen = PAGE_SIZE;
        ioVecIn.iov[0].pvBase = EncryptData;
        ioVecOut.size = 1;
        ioVecOut.iov = &IovecOut;
        ioVecOut.iov[0].dwLen = PAGE_SIZE;
        ioVecOut.iov[0].pvBase = TempOut;

        Ctx[ThreadId].InstanceId = ThreadId;
        Ret = SW_Cipher_Init (SW_CIPHER_ALG_AES256, &Ctx[ThreadId]);
        if (Ret) {
                return -1;
        }

        if (SW_Cipher_SetParam (SW_CIPHER_PARAM_DIRECTION, &Dir,
                sizeof (SW_CipherEncryptDir), &Ctx[ThreadId])) {
                return -1;
        }
        if (SW_Cipher_SetParam (SW_CIPHER_PARAM_MODE, &Mode, sizeof (Mode),
                                &Ctx[ThreadId])) {
                return -1;
        }
        if (SW_Cipher_SetParam (SW_CIPHER_PARAM_KEY, UnwrappedKey,
                sizeof (UnwrappedKey), &Ctx[ThreadId])) {
                return -1;
        }
        IncrementIV (Iv, sizeof (Dp->Iv), 1);
        if (SW_Cipher_SetParam (SW_CIPHER_PARAM_IV, Iv, sizeof (Dp->Iv),
                                &Ctx[ThreadId])) {
                return -1;
        }
        if (SW_Cipher_SetParam (SW_CIPHER_PARAM_AAD, (VOID *)Dp->Aad,
                sizeof (Dp->Aad), &Ctx[ThreadId])) {
                return -1;
        }
        if (SW_CipherData (ioVecIn, &ioVecOut, &Ctx[ThreadId])) {
                return -1;
        }
        if (SW_Cipher_GetParam (SW_CIPHER_PARAM_TAG, (VOID*)(AuthCurrent),
                Dp->Authsize, &Ctx[ThreadId])) {
                return -1;
        }

        if (MemCmp (AuthCurrent, Auth, Dp->Authsize)) {
                printf ("Auth Comparsion failed 0x%llx\n", Auth);
                return -1;
        }

        gBS->CopyMem ((VOID *)(EncryptData), (VOID *)(TempOut), PAGE_SIZE);
        SW_Cipher_DeInit (&Ctx[ThreadId]);
        return 0;
}
#endif

static INT32 ReadSwapInfoStruct (VOID)
{
        struct SwsuspInfo *Info;

        BootStatsSetTimeStamp (BS_KERNEL_LOAD_BOOT_START);

        Info = AllocateZeroPool (PAGE_SIZE);
        if (!Info) {
                printf ("Memory alloc failed Line %d\n", __LINE__);
                return -1;
        }
        if (ReadImage (SWAP_INFO_OFFSET, Info, 1)) {
                printf ("Failed to read Line %d\n", __LINE__);
                FreePages (Info, 1);
                return -1;
        }
        ResumeHdr = (struct ArchHibernateHdr *)Info;
        NrMetaPages = Info->Pages - Info->ImagePages - 1;
        NrCopyPages = Info->ImagePages;
        printf ("Total pages to copy = %lu Total meta pages = %lu\n",
                                NrCopyPages, NrMetaPages);
        PrintImageKernelDetails (Info);
        return 0;
}

/*
 * Reads image kernel pfn indexes by stripping off interleaved swap_map pages.
 *
 * swap_map pages are particularly useful when swap slot allocations are
 * randomized. For bootloader based hibernation we have disabled this for
 * performance reasons. But swap_map pages are still interleaved because
 * kernel/power/snapshot.c is written to handle both scenarios(sequential
 * and randomized swap slot).
 *
 * Snapshot layout in disk with randomization disabled for swap allocations in
 * kernel looks likes:
 *
 *                      disk offsets
 *                              |
 *                              |
 *                              V
 *                                 -----------------------
 *                              0 |     header            |
 *                                |-----------------------|
 *                              1 |  swap_map page 0      |
 *                                |-----------------------|           ------
 *                              2 |  swsusp_info struct   |              ^
 *      ------                    |-----------------------|              |
 *        ^                     3 |  PFN INDEX Page 0     |              |
 *        |                       |-----------------------|              |
 *        |                     4 |  PFN INDEX Page 1     |              |
 *        |                       |-----------------------| 511 swap map entries
 * 510 pfn index pages            |     :       :         |              |
 *        |                       |     :       :         |              |
 *        |                       |     :       :         |              |
 *        |                       |-----------------------|              |
 *        |                   512 |  PFN INDEX Page 509   |              V
 *      ------                    |-----------------------|            ------
 *                            513 |  swap_map page 1      |
 *      ------                    |-----------------------|            ------
 *        ^                   514 |  PFN INDEX Page 510   |              ^
 *        |                       |-----------------------|              |
 *        |                   515 |  PFN INDEX Page 511   |              |
 *        |                       |-----------------------|              |
 * 511 pfn index pages            |     :       :         | 511 swap map entries
 *        |                       |     :       :         |              |
 *        |                       |     :       :         |              |
 *        |                       |-----------------------|              |
 *        V                  1024 |  PFN INDEX Page 1021  |              V
 *      ------                    |-----------------------|            ------
 *                           1025 |  swap_map page 2      |
 *      ------                    |-----------------------|
 *        ^                  1026 |  PFN INDEX Page 1022  |
 *        |                       |-----------------------|
 *        |                  1027 |  PFN INDEX Page 1023  |
 *        |                       |-----------------------|
 * 511 pfn index pages            |     :       :         |
 *        |                       |     :       :         |
 *        |                       |     :       :         |
 *        |                       |-----------------------|
 *        V                  1536 |  PFN INDEX Page 1532  |
 *      ------                    |-----------------------|
 *                           1537 |  swap_map page 3      |
 *                                |-----------------------|
 *                           1538 |  PFN INDEX Page 1533  |
 *                                |-----------------------|
 *                           1539 |  PFN INDEX Page 1534  |
 *                                |-----------------------|
 *                                |     :       :         |
 *                                |     :       :         |
 */
static UINT64* ReadKernelImagePfnIndexes (UINT64 *Offset,
                                          struct DecryptInfo *DecInfo,
                                          struct CmpBuffer *CmpBuf,
                                          UINT64 MPages
                                          )
{
        UINT64 *PfnArray, *ArrayIndex;
        UINT64 PendingPages = NrMetaPages;
        UINT64 PagesToRead, PagesRead = 0;
        UINT64 DiskOffset;
        INT32 Loop = 0, Ret;
        DiskOffset = FIRST_PFN_INDEX_OFFSET;

        VOID *PfnArrayStart;
#if HIBERNATION_SUPPORT_AES
        UINT64 Soffset = DiskOffset;
#endif
        PfnArray = AllocatePages (NrMetaPages);
        if (!PfnArray) {
                printf ("Memory alloc failed Line %d\n", __LINE__);
                return NULL;
        }
        if (!IS_COMPRESS (SwsuspHeader->Flags)) {
                PfnArrayStart = PfnArray;
        }
        /*
         * First swap_map page has one less pfn_index page
         * because of presence of swsusp_info struct. Handle
         * it separately.
         */
        PagesToRead = MIN (PendingPages, MPages);
        ArrayIndex = PfnArray;
        do {
                if (IS_COMPRESS (SwsuspHeader->Flags)) {
#if HIBERNATION_SUPPORT_AES
                        /*The MetaDecompress function performs decryption
                         * and decompression of meta pages.
                         */
                        Ret = MetaDecompress (&Soffset, ArrayIndex, PagesToRead,
                                &Authtags, TempOut[0], AuthCur[0], 0,
                                IvGlb, DecInfo, CmpBuf);
#endif
                } else {
                        Ret = ReadImage (DiskOffset, ArrayIndex, PagesToRead);
                }
                if (Ret) {
                        printf ("Disk read failed Line %d\n", __LINE__);
                        goto err;
                }
                PagesRead += PagesToRead;
                PendingPages -= PagesToRead;
                if (!PendingPages) {
                        break;
                }
                Loop++;
                /*
                 * SwsuspHeader->Image points to first SwapMap page.
                 * From there onwards,
                 * swap_map pages are repeated at PFN_INDEXES_PER_PAGE interval.
                 * pfn_index pages follows the swap map page.
                 * So we can arrive at next pfn_index by using below formula,
                 *
                 * base_swap_map_slot + PFN_INDEXES_PER_PAGE * n + 1
                 */
                DiskOffset = SwsuspHeader->Image +
                                (PFN_INDEXES_PER_PAGE * Loop) + 1;
                PagesToRead = MIN (PendingPages, ENTRIES_PER_SWAPMAP_PAGE);
                ArrayIndex = PfnArray + PagesRead * PFN_INDEXES_PER_PAGE;
        } while (1);

        *Offset = DiskOffset + PagesToRead;
        if (!IS_COMPRESS (SwsuspHeader->Flags)) {
                while (PendingPages != NrMetaPages) {
#if HIBERNATION_SUPPORT_AES
                        if (DecryptPage (PfnArrayStart, Authtags, TempOut[0],
                                         AuthCur[0], 0, IvGlb)) {
                                printf ("Decryption failed for pfn array\n");
                                return NULL;
                        }
                        Authtags += Dp->Authsize;
#endif
                        PfnArrayStart = (CHAR8 *)PfnArrayStart + PAGE_SIZE;
                        PendingPages++;
                }
        }
        return PfnArray;
err:
        FreePages (PfnArray, NrMetaPages);
        return NULL;
}

static INT32 ReadDataPages (VOID *Arg)
{
        UINT32 PendingPages, NrReadPages;
        UINT64 PfnIndex = 0;
        INT32 Ret;
        Thread* CurrentThread = KernIntf->Thread->GetCurrentThread ();
        RestoreInfo *Info = (RestoreInfo *) Arg;

        PendingPages = Info->NumPages;
        if (IS_COMPRESS (SwsuspHeader->Flags)) {
#if HIBERNATION_SUPPORT_AES
                UINT64 Soffset = Info->Offset;
                while (PendingPages > 0) {
                        /* read pages in chunks to improve disk read performance
                         */
                        NrReadPages = PendingPages > DISK_BUFFER_PAGES ?
                                               DISK_BUFFER_PAGES : PendingPages;
                        /* The DataDecompress function decrypts, decompresses,
                         * and copy the data to the appropriate PfnOffset
                         */
                        Ret = DataDecompress (&Soffset, Info->DiskReadBuffer,
                                              NrReadPages, &Info->Authtags,
                                              Info->TempOut, Info->AuthCur,
                                              Info->ThreadId, Info->Iv,
                                              Info->DecInfo, Info->CmpBuf,
                                              &PfnIndex, Info->KernelPfnIndexes
                                              );
                        if (Ret < 0) {
                                Info->Status = -1;
                                goto err;
                        }
                        PendingPages -= NrReadPages;
                }
#endif
        } else {
                UINT64 SrcPfn, DstPfn;
                while (PendingPages > 0) {
                        /* read pages in chunks to improve disk read performance
                         */

                        NrReadPages = PendingPages > DISK_BUFFER_PAGES ?
                                               DISK_BUFFER_PAGES : PendingPages;
                        Ret = ReadImage (Info->Offset, Info->DiskReadBuffer,
                                         NrReadPages);
                        if (Ret < 0) {
                                printf ("Disk read failed Line %d\n", __LINE__);
                                Info->Status = -1;
                                return -1;
                        }

                        SrcPfn = (UINT64) Info->DiskReadBuffer >> PAGE_SHIFT;
                        while (NrReadPages > 0) {
                                /* skip swap_map pages */
                                if ( !CheckSwapMapPage (Info->Offset)) {
                                    DstPfn = Info->KernelPfnIndexes[PfnIndex++];
                                    /* When generating a hibernation
                                     * snapshot image, pages in memory that
                                     * consist entirely of zeros are
                                     * excluded from the snapshot.
                                     * To identify such zero pages, the MSB
                                     * of the PFN is set.
                                     */
                                    if (IS_ZERO_PFN (DstPfn)) {
                                            CopyZeroPageToDst (DstPfn);
                                            continue;
                                    }
                                    PendingPages--;
#if HIBERNATION_SUPPORT_AES
                                    if (DecryptPage (
                                             (VOID *)(SrcPfn << PAGE_SHIFT),
                                              Info->Authtags, Info->TempOut,
                                              Info->AuthCur, Info->ThreadId,
                                              Info->Iv))
                                    {
                                            printf (
                                              "Decrypt failed for Data \
                                               pages \n"
                                            );
                                            Info->Status = -1;
                                            goto err;
                                    }
                                    Info->Authtags += Dp->Authsize;
#endif
                                    CopyPageToDst (SrcPfn, DstPfn);
                                }
                                SrcPfn++;
                                NrReadPages--;
                                Info->Offset++;
                        }
                }
        }
        Info->Status = 0;
#if HIBERNATION_SUPPORT_AES
err:
#endif
        KernIntf->Sem->SemPost (Info->Sem, FALSE);
        ThreadStackNodeRemove (CurrentThread);
        KernIntf->Thread->ThreadExit (0);
        return 0;
}

static struct MappedRange * GetUefiSortedMemoryMap ()
{
        EFI_MEMORY_DESCRIPTOR   *MemMap;
        EFI_MEMORY_DESCRIPTOR   *MemMapPtr;
        UINTN                   MemMapSize;
        UINTN                   MapKey, DescriptorSize;
        UINTN                   Index;
        UINT32                  DescriptorVersion;
        EFI_STATUS              Status;

        struct MappedRange * UefiMap = NULL;
        MemMapSize = 0;
        MemMap     = NULL;

        Status = gBS->GetMemoryMap (&MemMapSize, MemMap, &MapKey,
                                &DescriptorSize, &DescriptorVersion);
        if (Status != EFI_BUFFER_TOO_SMALL) {
                printf ("ERROR: Undefined response get memory map\n");
                return NULL;
        }
        if (CHECK_ADD64 (MemMapSize, EFI_PAGE_SIZE)) {
                printf ("ERROR: integer Overflow while adding additional"
                                "memory to MemMapSize");
                return NULL;
        }
        MemMapSize = MemMapSize + EFI_PAGE_SIZE;
        MemMap = AllocateZeroPool (MemMapSize);
        if (!MemMap) {
                printf ("ERROR: Failed to allocate memory for memory map\n");
                return NULL;
        }
        MemMapPtr = MemMap;
        Status = gBS->GetMemoryMap (&MemMapSize, MemMap, &MapKey,
                                &DescriptorSize, &DescriptorVersion);
        if (EFI_ERROR (Status)) {
                printf ("ERROR: Failed to query memory map\n");
                FreePool (MemMapPtr);
                return NULL;
        }
        for (Index = 0; Index < MemMapSize / DescriptorSize; Index ++) {
                UefiMap = AddRangeSorted (UefiMap,
                        MemMap->PhysicalStart,
                        MemMap->PhysicalStart +
                        MemMap->NumberOfPages * PAGE_SIZE);

                if (!UefiMap) {
                        printf ("ERROR: UefiMap is NULL\n");
                        return NULL;
                }

                MemMap = (EFI_MEMORY_DESCRIPTOR *)((UINTN)MemMap +
                                DescriptorSize);
        }
        FreePool (MemMapPtr);
        return UefiMap;
}

static EFI_STATUS CreateMapping (UINTN Addr, UINTN Size)
{
        EFI_STATUS Status;
        EFI_GCD_MEMORY_SPACE_DESCRIPTOR Descriptor;
        printf ("Address: %llx Size: %llx\n", Addr, Size);

        Status = gDS->GetMemorySpaceDescriptor (Addr, &Descriptor);
        if (EFI_ERROR (Status)) {
                printf ("Failed getMemorySpaceDescriptor Line %d\n", __LINE__);
                return Status;
        }

        if (Descriptor.GcdMemoryType != EfiGcdMemoryTypeMemoryMappedIo) {
                if (Descriptor.GcdMemoryType != EfiGcdMemoryTypeNonExistent) {
                        Status = gDS->RemoveMemorySpace (Addr, Size);
                        printf ("Falied RemoveMemorySpace %d: %d\n",
                                __LINE__, Status);
                }
                Status = gDS->AddMemorySpace (EfiGcdMemoryTypeReserved,
                                Addr, Size, EFI_MEMORY_UC);
                if (EFI_ERROR (Status)) {
                        printf ("Failed to AddMemorySpace 0x%x, size 0x%x\n",
                                Addr, Size);
                        return Status;
                }

                Status = gDS->SetMemorySpaceAttributes (Addr, Size,
                                                        EFI_MEMORY_UC);
                if (EFI_ERROR (Status)) {
                        printf (
                        "Failed to SetMemorySpaceAttributes 0x%x, size 0x%x\n",
                         Addr, Size);
                                return Status;
                }
        }

        return EFI_SUCCESS;
}

/*
 * Determine the unmapped uefi memory from the list 'uefi_mapped_sorted_list'
 * and map all the unmapped regions.
 */
static EFI_STATUS UefiMapUnmapped ()
{
        struct MappedRange * UefiMappedSortedList, * Cur, * Next;
        EFI_STATUS Status;

        UefiMappedSortedList = GetUefiSortedMemoryMap ();
        if (!UefiMappedSortedList) {
                printf ("ERROR: Unable to get UEFI memory map\n");
                return -1;
        }

        Cur = UefiMappedSortedList;
        Next = Cur->Next;

        while (Cur) {
                if (Next &&
                    ((Next->Start) >
                    (Cur->End))) {
                        Status = CreateMapping (Cur->End,
                                                Next->Start - Cur->End);
                        if (Status != EFI_SUCCESS) {
                                printf ("ERROR: Mapping failed\n");
                                return Status;
                        }
                }
                Status = gBS->FreePool (Cur);
                if (Status != EFI_SUCCESS) {
                        printf ("FreePool failed %d\n", __LINE__);
                        return -1;
                }
                Cur = Next;
                if (Next) {
                        Next = Next->Next;
                }
        }

        return EFI_SUCCESS;
}

#define PT_ENTRIES_PER_LEVEL 512

static VOID SetRwPerm (UINT64 *Entry)
{
        /* Clear AP perm bits */
        *Entry &= ~(0x3UL << 6);
}

static VOID SetExPerm (UINT64 *Entry)
{
        /* Clear UXN and PXN bits */
        *Entry &= ~(0x3UL << 53);
}

static INT32 RelocatePagetables (INT32 Level, UINT64 *Entry, INT32 PtCount)
{
        INT32 Iter;
        UINT64 Mask;
        UINT64 *PageAddr;
        UINT64 ApPerm;

        ApPerm = *Entry & (0x3 << 6);
        ApPerm = ApPerm >> 6;
        /* Strip out lower and higher page attribute fields */
        Mask = ~(0xFFFFUL << 48 | 0XFFFUL);

        /* Invalid entry */
        if ((Level > 3)
               ||
            (!(*Entry & 0x1))) {
                return PtCount;
        }

        if (Level == 3 ) {
                if ((*Entry & Mask) == RelocateAddress) {
                        SetExPerm (Entry);
                }
                if ((ApPerm == 2)
                        ||
                    (ApPerm == 3)) {
                        SetRwPerm (Entry);
                }
                return PtCount;
        }

        /* block entries */
        if ((*Entry & 0x3) == 1) {
                UINT64 Addr = RelocateAddress;
                if (Level == 1) {
                        ALIGN_1GB (Addr);
                }
                if (Level == 2) {
                        ALIGN_2MB (Addr);
                }
                if ((*Entry & Mask) == Addr) {
                        SetExPerm (Entry);
                }
                if ((ApPerm == 2)
                        ||
                    (ApPerm == 3)) {
                        SetRwPerm (Entry);
                }
                return PtCount;
        }

        /* Control reaches here only if it is a table entry */

        PageAddr = (UINT64*)(GetUnusedPfn () << PAGE_SHIFT);

        gBS->CopyMem ((VOID *)(PageAddr), (VOID *)(*Entry & Mask), PAGE_SIZE);
        PtCount++;
        /* Clear off the old address alone */
        *Entry &= ~Mask;
        /* Fill new table address */
        *Entry |= (UINT64 )PageAddr;

        for (Iter = 0 ; Iter < PT_ENTRIES_PER_LEVEL; Iter++)
                PtCount = RelocatePagetables (Level + 1, PageAddr + Iter,
                                PtCount);

        return PtCount;
}

static UINT64 GetTtbr0 ()
{
        UINT64 Base;

        asm __volatile__ (
        "mrs %[ttbr0_base], ttbr0_el1\n"
        :[ttbr0_base] "=r" (Base)
        :
        :"memory");

        return Base;
}

static UINT64 CopyPageTables ()
{
        UINT64 OldTtbr0 = GetTtbr0 ();
        UINT64 NewTtbr0;
        INT32  PtCount = 0;

        NewTtbr0 = GetUnusedPfn () << PAGE_SHIFT;
        gBS->CopyMem ((VOID *)(NewTtbr0), (VOID *)(OldTtbr0), PAGE_SIZE);
        PtCount = RelocatePagetables (0, (UINT64 *)NewTtbr0, 1);

        printf ("Copied %d Page Tables\n", PtCount);
        return NewTtbr0;
}

#if HIBERNATION_SUPPORT_AES
#if HIBERNATION_TZ_ENCRYPTION
static INT32 SetupSMCI (VOID)
{
        EFI_STATUS Status = EFI_SUCCESS;
        QCOM_SCM_PROTOCOL *pQcomScmProtocol = NULL;
        INT32 Ret = 0;

        Status = gBS->LocateProtocol (&gQcomScmProtocolGuid, NULL,
                                        (VOID **)&pQcomScmProtocol);
        if (Status != EFI_SUCCESS ||
            (pQcomScmProtocol == NULL)) {
                DEBUG ((EFI_D_ERROR,
                        "Locate SCM Protocol failed, Status: (0x%x)\n",
                        Status));
                Status = ERROR_SECURITY_STATE;
                    return Status;
        }

        Status = pQcomScmProtocol->ScmGetClientEnv (pQcomScmProtocol,
                                                    &ClientEnvObj);
        if (Object_isERROR (Status) ||
            Object_isNull (ClientEnvObj)) {
                DEBUG ((EFI_D_ERROR,
                        "Failed to get Client Env, Status: (0x%x)\n",
                        Status));
        }

        Status = IClientEnvOpen (ClientEnvObj, CLOWPOWERKEYMANAGER_UID,
                                 &AppClientObj);
        if (Object_isERROR (Status) ||
            Object_isNull (AppClientObj)) {
                DEBUG ((EFI_D_ERROR,
                        "Failed to get App Client, Status: (0x%x)\n",
                        Status));
        }

        return Ret;
}

static VOID SMCICleanup (VOID)
{
        Object_ASSIGN_NULL (AppClientObj);
        Object_ASSIGN_NULL (ClientEnvObj);
}

INT32 KeyMgrGetKey (UINT32 Event, VOID *Key, size_t KeyLen,
                    size_t *KeyLenOut)
{
        INT32 Ret = SetupSMCI ();

        if (Ret) {
                goto exit;
        }

        Ret = ILowPowerKeyManagerGetKey (AppClientObj, Event, Key,
                        KeyLen, KeyLenOut);
exit:
        SMCICleanup ();
        return Ret;
}

static INT32 InitTzAndGetKey ()
{
        INT32 Ret;
        size_t KeyLenOut;
        Ret = KeyMgrGetKey (ILOWPOWERKEYMANAGER_HIBERNATE_WITH_ENCRYPTION,
                        UnwrappedKey,
                        AES256_KEY_SIZE,
                        &KeyLenOut);

        gBS->CopyMem ((VOID *)(IvGlb), (VOID *)(Dp->Iv), sizeof (Dp->Iv));
        Ret = 0;
        return Ret;
}
#else
static INT32 InitTaAndGetKey (struct Secs2dTaHandle *TaHandle)
{
        INT32 Status;
        CmdReq Req = {0};
        CmdRsp Rsp = {0};
        UINT32 ReqLen, RspLen;

        Status = gBS->LocateProtocol (&gQcomQseecomProtocolGuid, NULL,
                (VOID **)&(TaHandle->QseeComProtocol));
        if (Status) {
                printf ("Error in locating Qseecom protocol Guid\n");
                return -1;
        }
        Status = TaHandle->QseeComProtocol->QseecomStartApp (
                TaHandle->QseeComProtocol, "secs2d_a", &(TaHandle->AppId));
        if (Status) {
                printf ("Error in secs2d app loading\n");
                return -1;
        }

        ReqLen = sizeof (CmdReq);
        if (ReqLen & QSEECOM_ALIGN_MASK) {
                ReqLen = QSEECOM_ALIGN (ReqLen);
        }

        RspLen = sizeof (CmdRsp);
        if (RspLen & QSEECOM_ALIGN_MASK) {
                RspLen = QSEECOM_ALIGN (RspLen);
        }

        gBS->CopyMem ((VOID *)(IvGlb), (VOID *)(Dp->Iv), sizeof (Dp->Iv));

        Req.Cmd = UNWRAP_KEY_CMD;
        Req.UnwrapkeyReq.WrappedKeySize = WRAPPED_KEY_SIZE;
        gBS->CopyMem ((VOID *)Req.UnwrapkeyReq.WrappedKeyBuffer,
                        (VOID *)Dp->KeyBlob, sizeof (Dp->KeyBlob));
        Req.UnwrapkeyReq.CurrTime.Hour = 4;
        Status = TaHandle->QseeComProtocol->QseecomSendCmd (
                TaHandle->QseeComProtocol, TaHandle->AppId,
                        (UINT8 *)&Req, ReqLen,
                        (UINT8 *)&Rsp, RspLen);
        if (Status) {
                printf ("Error in conversion wrappeded key to unwrapped key\n");
                return -1;
        }
        gBS->CopyMem ((VOID *)UnwrappedKey,
                        (VOID *)Rsp.UnwrapkeyRsp.KeyBuffer, 32);

        Status = TaHandle->QseeComProtocol->QseecomShutdownApp (
                TaHandle->QseeComProtocol, TaHandle->AppId);
        if (Status) {
                printf ("Error in secs2d app loading\n");
                return -1;
        }

        return 0;
}
#endif
static INT32 InitAesDecrypt (VOID)
{
        INT32 AuthslotCount;
#if !HIBERNATION_TZ_ENCRYPTION
        Secs2dTaHandle TaHandle = {0};
#endif
        UINT32 NrSwapMapPages, i;
        Authslot = AllocatePages (1);

        if (!Authslot) {
                return -1;
        }

        Dp = AllocatePages (1);
        if (!Dp) {
                printf ("Memory alloc failed Line %d\n", __LINE__);
                return -1;
        }
        if (IS_COMPRESS (SwsuspHeader->Flags)) {
                if (ReadImage (1, Authslot, 1)) {
                        return -1;
                }
                AuthslotStart = *(INT32 *) (Authslot);
        } else {
                NrSwapMapPages = (NrCopyPages + NrMetaPages)
                                        /
                                  ENTRIES_PER_SWAPMAP_PAGE;
                AuthslotStart = NrMetaPages + NrCopyPages + NrSwapMapPages +
                                HDR_SWP_INFO_NUM_PAGES;
        }

        if (ReadImage (AuthslotStart - 1, Dp, 1)) {
                return -1;
        }

        AuthslotCount = Dp->AuthCount;
        Authtags = AllocatePages (AuthslotCount);
        if (!Authtags) {
                return -1;
        }
        if (ReadImage (AuthslotStart, Authtags, AuthslotCount)) {
                return -1;
        }
        for (i = 0; i < NUM_CORES; i++) {
                TempOut[i] = AllocatePages (1);
                if (!TempOut[i]) {
                        return -1;
                }
                AuthCur[i] = AllocateZeroPool (Dp->Authsize);
                if (!AuthCur[i]) {
                        return -1;
                }
        }
#if HIBERNATION_TZ_ENCRYPTION
        if (InitTzAndGetKey ()) {
                return -1;
        }
#else
        if (InitTaAndGetKey (&TaHandle)) {
                return -1;
        }
#endif
        printf ("Hibernation: AES init done\n");
        return 0;
}
#else
static INT32 InitAesDecrypt (VOID)
{
        return 0;
}
#endif

/* In both compression and nocompression scenarios, the ThreadConstructor
 * populates the necessary parameter values for the RestoreInfo structure.
 */
static INT32 ThreadConstructor (VOID *Arg, UINT64 *Offset, UINT64 *PfnOffset,
                                INT32 *DataIndx, INT64 BlkLen, UINT8 *BlkArr,
                                INT64 NumPages
                                )
{

        RestoreInfo *Info = (RestoreInfo *) Arg;
        INT32 Iter2;
        UINT64 NumCompPages;
        Info->Offset = *Offset;
        Info->NumPages = NumPages;
        Info->KernelPfnIndexes = &KernelPfnIndexes[*PfnOffset];
        NumCompPages = NumPages;
        UINT64 DstPfn_z, NumDecomPage;
#if HIBERNATION_SUPPORT_AES
        UINT64 Count = 0;
        gBS->CopyMem (Info->Iv, IvGlb, sizeof (Dp->Iv));
        Info->Authtags = Authtags;
#endif
        if (IS_COMPRESS (SwsuspHeader->Flags)) {
#if HIBERNATION_SUPPORT_AES
                INT32 NSlot = 0;
                NumCompPages = 0;
                while ((NumCompPages <= NumPages)
                                &&
                                ((*DataIndx) < BlkLen))
                {
                    NumCompPages += BlkArr[(*DataIndx)++];
                    NSlot++;
                }
                Info->NumPages = NumCompPages;
                Info->CmpBuf =  AllocateZeroPool (sizeof (struct CmpBuffer));
                if (!Info->CmpBuf) {
                    printf ("Memory alloc failed Line %d\n", __LINE__);
                    return -1;
                }
                Info->CmpBuf->Buf = AllocatePages (CMP_UNC_PAGES * NSlot);
                if (!Info->CmpBuf->Buf) {
                    printf ("Memory alloc failed Line %d\n", __LINE__);
                    return -1;
                } else {
                    Info->CmpBuf->RPos = 0;
                    Info->CmpBuf->WPos = 0;
                }

                Info->DecInfo =  AllocateZeroPool (sizeof (struct DecryptInfo));
                if (!Info->DecInfo) {
                   printf ("Memory alloc failed Line %d\n", __LINE__);
                   return -1;
                } else {
                      Info->DecInfo->RByte = 0;
                      Info->DecInfo->Pos = 0;
                      Info->DecInfo->CMPLen = 0;
                      Info->DecInfo->TBytes = 0;
                }
                NumDecomPage = CMP_UNC_PAGES * NSlot;
#endif
        } else {
                NumDecomPage = NumPages;
        }
        for (Iter2 = 0; Iter2 < NumCompPages; Iter2++) {
                if (CheckSwapMapPage (*Offset)) {
                        (*Offset)++;
                        if (IS_COMPRESS (SwsuspHeader->Flags)) {
                                Info->NumPages++;
                        }
                }
                (*Offset)++;
#if HIBERNATION_SUPPORT_AES
                Authtags += Dp->Authsize;
                Count++;
#endif
        }
        for (Iter2 = 0; Iter2 < NumDecomPage; Iter2++) {
                DstPfn_z = KernelPfnIndexes[*PfnOffset];
                if (IS_ZERO_PFN (DstPfn_z)) {
                        Iter2--;
                }
                (*PfnOffset)++;
        }
#if HIBERNATION_SUPPORT_AES
        IncrementIV (IvGlb, sizeof (Dp->Iv), Count);
#endif
        return 0;
}

static INT32 RestoreSnapshotImage (VOID)
{
        INT32 Ret = 0, Iter1 = 0;
        UINT64 StartMs, Offset, PfnOffset = 0;
        RestoreInfo Info[NUM_CORES];
        struct BounceTableIterator *Bti = &TableIterator;
        Thread *T[NUM_CORES];
        CHAR8 *ThreadName;

        UINT64 NumCompPages = 0;
        UINT8 *BlkArr = NULL;
        INT32 BlkLen = 0, DataIndx = 0, j = 0;
#if HIBERNATION_SUPPORT_AES
        /* Parameters required for decompression */
        INT32 MetaIndx;
        UINT64 NumSilverPage = 0, NumGoldPage = 0;
        INT32 ExtraDataPage = 0;
        UINT32 SMPage = 0; UINT64 DstPfn_z;
#endif
        InitReadMultiThreadEnv ();
        Ret = EnableAllCores ();
        if (Ret < 0) {
            DEBUG ((EFI_D_ERROR, "EnableAllCores failed\n"));
            return Ret;
        }
        StartMs = GetTimerCountms ();
        Ret = ReadSwapInfoStruct ();
        if (Ret < 0) {
                return Ret;
        }

        if (InitAesDecrypt ()) {
                printf ("AES initialization failed\n");
                return -1;
        }

        /* IS_COMPRESS function verifies if the hibernation image
         * has been compressed using LZ4 algorithm.
         */
        if (IS_COMPRESS (SwsuspHeader->Flags)) {
#if HIBERNATION_SUPPORT_AES
                if (InitBlockArr (&BlkArr)) {
                    printf ("Block array initialization failed \n");
                    return -1;
                }

                /* Determining the count of indexes that need
                 * to be read for a compressed meta page.
                 */
                MetaIndx = (NrMetaPages % CMP_UNC_PAGES) == 0 ?
                        NrMetaPages / CMP_UNC_PAGES
                        :
                        NrMetaPages / CMP_UNC_PAGES + 1;

                for (j = 0; j < MetaIndx; j++) {
                    NumCompPages += BlkArr[j];
                }

                SMPage = (FIRST_PFN_INDEX_OFFSET - 1 + NumCompPages)
                                        /
                          PFN_INDEXES_PER_PAGE;
                CmpBuf.Buf = AllocatePages (NrMetaPages + CMP_UNC_PAGES);
                if (!CmpBuf.Buf) {
                        printf ("Memory alloc failed Line %d\n", __LINE__);
                        return -1;
                }
#endif
        } else {
                CmpBuf.Buf = NULL;
                NumCompPages = ENTRIES_PER_SWAPMAP_PAGE - 1;
        }

        KernelPfnIndexes = ReadKernelImagePfnIndexes (&Offset, &DecInfo,
                                                      &CmpBuf, NumCompPages);
        if (!KernelPfnIndexes) {
                return -1;
        }
        InitKernelPfnIterator (KernelPfnIndexes);

        ThreadName = AllocateZeroPool (sizeof (CHAR8) * 8);
        if (!ThreadName) {
                printf ("Error allocating memory\n");
                return -1;
        }

        if (IS_COMPRESS (SwsuspHeader->Flags)) {
#if HIBERNATION_SUPPORT_AES
                /* Incrementing the offset value in accordance with
                 * the compressed meta pages and the Swap Map page */
                Offset = FIRST_PFN_INDEX_OFFSET + NumCompPages + SMPage;
                /* While decompressing the final meta block unit, if any data
                 * pages are also decompressed simultaneously, we ensure that
                 * the PfnOffset is set correctly. Subsequently, once all the
                 * data pages have been decompressed, we will copy those data
                 * pages to their respective Pfn offsets.
                 */
                ExtraDataPage = (CmpBuf.WPos - CmpBuf.RPos) / PAGE_SIZE;

                for (Iter1 = 0; Iter1 < ExtraDataPage; Iter1++) {
                        DstPfn_z = KernelPfnIndexes[PfnOffset];
                        if (IS_ZERO_PFN (DstPfn_z)) {
                                Iter1--;
                        }
                        PfnOffset++;
                }

                BlkLen = (NrCopyPages + NrMetaPages) / CMP_UNC_PAGES + 1;
                DataIndx = j, NumCompPages = 0;

                for (j = 0; j < BlkLen; j++) {
                    NumCompPages += BlkArr[j];
                }

                NumSilverPage = (NumCompPages * 0.4) / (NUM_SILVER_CORES);
                NumGoldPage = (NumCompPages * 0.6)
                                        /
                              (NUM_CORES - NUM_SILVER_CORES);
#endif
        }
        for (Iter1 = 0; Iter1 < NUM_SILVER_CORES; Iter1++) {
                if (IS_COMPRESS (SwsuspHeader->Flags)) {
#if HIBERNATION_SUPPORT_AES
                        Ret = ThreadConstructor ((VOID *)&Info[Iter1], &Offset,
                                                  &PfnOffset, &DataIndx, BlkLen,
                                                  BlkArr, NumSilverPage
                                                );
#endif
                } else {
                        Ret = ThreadConstructor ((VOID *)&Info[Iter1], &Offset,
                                                  &PfnOffset, &DataIndx, BlkLen,
                                                  BlkArr,
                                                  NUM_PAGES_PER_SILVER_CORE
                                                );
                }
                if (Ret) {
                    return Ret;
                }
        }

        for (Iter1 = NUM_SILVER_CORES; Iter1 < NUM_CORES - 1; Iter1++) {
                if (IS_COMPRESS (SwsuspHeader->Flags)) {
#if HIBERNATION_SUPPORT_AES
                        Ret = ThreadConstructor ((VOID *)&Info[Iter1], &Offset,
                                                  &PfnOffset, &DataIndx, BlkLen,
                                                  BlkArr, NumGoldPage
                                                );
#endif
                } else {
                        Ret = ThreadConstructor ((VOID *)&Info[Iter1], &Offset,
                                                  &PfnOffset, &DataIndx, BlkLen,
                                                  BlkArr,
                                                  NUM_PAGES_PER_GOLD_CORE
                                                );
                }
                if (Ret) {
                    return Ret;
                }
        }

        /* Assign the remaining page to the last thread */
        if (IS_COMPRESS (SwsuspHeader->Flags)) {
                NumCompPages = 0;
                for (j = DataIndx; j < BlkLen; j++) {
                    NumCompPages += BlkArr[j];
                }
        }
        if (IS_COMPRESS (SwsuspHeader->Flags)) {
#if HIBERNATION_SUPPORT_AES
                Ret = ThreadConstructor ((VOID *)&Info[Iter1], &Offset,
                                &PfnOffset, &DataIndx, BlkLen, BlkArr,
                                NumCompPages
                                );
#endif
        } else {
                Ret = ThreadConstructor ((VOID *)&Info[Iter1], &Offset,
                                &PfnOffset, &DataIndx, BlkLen, BlkArr,
                                NrCopyPages - (4 * NUM_PAGES_PER_SILVER_CORE)
                                                            -
                                                (3 * NUM_PAGES_PER_GOLD_CORE)
                                );
        }
        if (Ret) {
                return Ret;
        }

        for (Iter1 = 0; Iter1 < NUM_CORES; Iter1++) {
                Info[Iter1].DiskReadBuffer = AllocatePages (DISK_BUFFER_PAGES);
                if (!Info[Iter1].DiskReadBuffer) {
                       printf ("Memory alloc failed Line %d\n", __LINE__);
                       return -1;
                } else {
                        printf ("Disk buffer alloction at 0x%p - 0x%p\n",
                                Info[Iter1].DiskReadBuffer,
                                Info[Iter1].DiskReadBuffer +
                                DISK_BUFFER_SIZE - 1);
                }
#if HIBERNATION_SUPPORT_AES
                Info[Iter1].ThreadId = Iter1;
                Info[Iter1].TempOut = TempOut[Iter1];
                Info[Iter1].AuthCur = AuthCur[Iter1];
#endif
                Info[Iter1].Sem = KernIntf->Sem->SemInit (Iter1, 0);

                AsciiSPrint (ThreadName, 8, "Thread%d", Iter1);
                if (!KernIntf ||
                    !KernIntf->Thread)
                        return -1;

                T[Iter1] = KernIntf->Thread->ThreadCreate (ThreadName,
                                 ReadDataPages, (VOID *)&Info[Iter1],
                                 UEFI_THREAD_PRIORITY, DEFAULT_STACK_SIZE);
                KernIntf->Thread->ThreadSetPinnedCpu (T[Iter1], Iter1);
                AllocateUnSafeStackPtr (T[Iter1]);
        }
        mx = KernIntf->Mutex->MutexInit (1);

        printf ("Mapping Regions:\n");
        Ret = UefiMapUnmapped ();
        if (Ret < 0) {
                printf ("Error mapping unmapped regions\n");
                Ret = -1;
                goto err;
        }

        /*
         * No dynamic allocation beyond this point. If not honored it will
         * result in corruption of pages.
         */
        Ret = GetConventionalMemoryRanges ();
        if (Ret < 0) {
                printf ("Error getting memory regions\n");
                goto err;
        }

        Ret = PreallocateFreeRanges ();
        if (Ret < 0) {
                printf ("Error allocating memory\n");
                goto err;
        }

        Bti->FirstTable = (struct BounceTable *)
                                (GetUnusedPfn () << PAGE_SHIFT);
        Bti->CurTable = Bti->FirstTable;

        /* assign unused pfn to zero page pfn */
        ZeroPagePfn = GetUnusedPfn ();
        SetMem ((UINT64*)(ZeroPagePfn << PAGE_SHIFT), PAGE_SIZE, 0);

        for (Iter1 = NUM_CORES - 1; Iter1 >= 0; Iter1--) {
                Ret = KernIntf->Thread->ThreadResume (T[Iter1]);
                DEBUG ((EFI_D_INFO, "Thread %d created with Status : %d\n",
                        Iter1, Ret));
                MicroSecondDelay (3000);
        }

        for (Iter1 = NUM_CORES - 1; Iter1 >= 0; Iter1--) {
                KernIntf->Sem->SemWait (Info[Iter1].Sem);
        }

        for (Iter1 = 0; Iter1 < NUM_CORES; Iter1++) {
                if (Info[Iter1].Status != 0) {
                        printf ("error in restore_snapshot_image\n");
                        Ret = -1;
                        goto err;
                }
        }

        if (IS_COMPRESS (SwsuspHeader->Flags)) {
                /* Copy data pages to their respective PfnOffsets, starting from
                 * 0, if any data pages are decompressed during the
                 * decompression of the final meta page unit.
                 */
#if HIBERNATION_SUPPORT_AES
                if ((CmpBuf.WPos - CmpBuf.RPos) / PAGE_SIZE) {
                        UINT64 SrcPfn1, DstPfn1;
                        INT32 Loop = 0, Ind = 0;
                        SrcPfn1 = (UINT64) (CmpBuf.Buf + CmpBuf.RPos)
                                                        >> PAGE_SHIFT;
                        Loop = (CmpBuf.WPos - CmpBuf.RPos) / PAGE_SIZE;
                        while (Loop > 0) {
                                DstPfn1 = KernelPfnIndexes[Ind++];
                                if (IS_ZERO_PFN (DstPfn1)) {
                                       CopyZeroPageToDst (DstPfn1);
                                } else {
                                       CopyPageToDst (SrcPfn1, DstPfn1);
                                       SrcPfn1++;
                                       Loop--;
                                }
                        }
                        CmpBuf.RPos = 0, CmpBuf.WPos = 0;
                }
#endif
        }
        BootStatsSetTimeStamp (BS_KERNEL_LOAD_BOOT_END);

        printf ("Image size = %lu MBs\n",
                (NrCopyPages * PAGE_SIZE) / (1024 * 1024));
        printf ("Time loading image (excluding bounce buffers) = %lu msecs\n",
                (GetTimerCountms () - StartMs));
        printf ("Image restore Completed...\n");
        printf ("Total bounced Pages = %d (%lu MBs)\n",
                BouncedPages, (BouncedPages * PAGE_SIZE)/(1024 * 1024));
err:
        for (Iter1 = 0; Iter1 < NUM_CORES; Iter1++) {
                FreePages (Info[Iter1].DiskReadBuffer, DISK_BUFFER_PAGES);
        }
        return Ret;
}

STATIC VOID
SetLinuxBootCpu (UINT32 BootCpu)
{
  EFI_STATUS Status;
  Status = gRT->SetVariable (L"DestinationCore",
      &gQcomTokenSpaceGuid,
      (EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_NON_VOLATILE |
       EFI_VARIABLE_RUNTIME_ACCESS),
       sizeof (UINT32),
       (VOID*)(UINT32*)&BootCpu);

  if (Status != EFI_SUCCESS) {
       DEBUG ((EFI_D_ERROR, "Error: Failed to set Linux boot cpu:%d\n",
                BootCpu));
   } else if (Status == EFI_SUCCESS) {
       DEBUG ((EFI_D_INFO, "Switching to physical CPU:%d for Booting Linux\n",
                BootCpu));
   }

  return;
}

#ifdef TARGET_LINUX_BOOT_CPU_ID
#define BootCpuId TARGET_LINUX_BOOT_CPU_ID
STATIC BOOLEAN
BootCpuSelectionEnabled (VOID)
{
  return TRUE;
}
#else
#define BootCpuId 0
STATIC BOOLEAN
BootCpuSelectionEnabled (VOID)
{
  return FALSE;
}
#endif

static VOID CopyBounceAndBootKernel ()
{
        INT32 Status;
        struct BounceTableIterator *Bti = &TableIterator;
        UINT64 CpuResume = (UINT64) ResumeHdr->PhysReEnterKernel;
        UINT64 Ttbr0;
        UINT64 StackPointer;

        /*
         * After copying the bounce pages, there is a chance of corrupting
         * old stack which might be kernel memory.
         * To avoid kernel memory corruption, Use a free page for the stack.
         */
        StackPointer = GetUnusedPfn () << PAGE_SHIFT;
        StackPointer = StackPointer + PAGE_SIZE - 16;

        /*
         * The restore routine "JumpToKernel" copies the bounced pages after
         * iterating through the bounce entry table and passes control to
         * hibernated kernel after calling _PreparePlatformHarware
         *
         * Disclaimer: JumpToKernel.s is less than PAGE_SIZE
         */
        gBS->CopyMem ((VOID*)RelocateAddress, (VOID*)&JumpToKernel, PAGE_SIZE);
        Ttbr0 = CopyPageTables ();

        BootStatsSetTimeStamp (BS_BL_END);

        printf ("Disable UEFI Boot services\n");
        printf ("Kernel entry point = 0x%lx\n", CpuResume);
        printf ("Relocation code at = 0x%lx\n", RelocateAddress);

        if (BootCpuSelectionEnabled ()) {
          SetLinuxBootCpu (BootCpuId);
        }

        /* Shut down UEFI boot services */
        Status = ShutdownUefiBootServices ();
        if (EFI_ERROR (Status)) {
                printf ("ERROR: Can not shutdown UEFI boot services."
                        " Status=0x%X\n", Status);
                return;
        }

        asm __volatile__ (
                "mov    x18, %[ttbr_reg]\n"
                "msr    ttbr0_el1, x18\n"
                "dsb    sy\n"
                "isb\n"
                "ic     iallu\n"
                "dsb    sy\n"
                "isb\n"
                "tlbi   vmalle1\n"
                "dsb    sy\n"
                "isb\n"
                :
                :[ttbr_reg] "r" (Ttbr0)
                :"x18", "memory");

        asm __volatile__ (
                "mov x18, %[table_base]\n"
                "mov x19, %[count]\n"
                "mov x21, %[resume]\n"
                "mov sp, %[sp]\n"
                "mov x22, %[relocate_code]\n"
                "br x22"
                :
                :[table_base] "r" (Bti->FirstTable),
                [count] "r" (BouncedPages),
                [resume] "r" (CpuResume),
                [sp] "r" (StackPointer),
                [relocate_code] "r" (RelocateAddress)
                :"x18", "x19", "x21", "x22", "memory");
}

static INT32 CheckForValidHeader (VOID)
{
        SwsuspHeader = AllocatePages (1);
        if (!SwsuspHeader) {
                printf ("Memory alloc failed Line %d\n", __LINE__);
                return -1;
        }

        if (VerifySwapPartition ()) {
                printf ("Failled VerifySwapPartition\n");
                goto read_image_error;
        }

        if (ReadImage (0, SwsuspHeader, 1)) {
                printf ("Disk read failed Line %d\n", __LINE__);
                goto read_image_error;
        }

        if (MemCmp (HIBERNATE_SIG, SwsuspHeader->Sig, 10)) {
                printf ("Signature not found. Aborting hibernation\n");
                goto read_image_error;
        }

        printf ("Image slot at 0x%lx\n", SwsuspHeader->Image);
        if (SwsuspHeader->Image != 1) {
                printf ("Invalid swap slot. Aborting hibernation!");
                goto read_image_error;
        }

        printf ("Signature found. Proceeding with disk read...\n");
        return 0;

read_image_error:
        FreePages (SwsuspHeader, 1);
        return -1;
}

static VOID EraseSwapSignature (VOID)
{
        EFI_STATUS Status;
        EFI_BLOCK_IO_PROTOCOL *BlockIo = SwapDetails.BlockIo;

        SwsuspHeader->Sig[0] = ' ';
        Status = BlockIo->WriteBlocks (BlockIo, BlockIo->Media->MediaId, 0,
                        EFI_PAGE_SIZE, (VOID*)SwsuspHeader);
        if (Status != EFI_SUCCESS) {
                printf ("Failed to erase swap signature\n");
        }
}

VOID BootIntoHibernationImage (BootInfo *Info,
                               BOOLEAN *SetRotAndBootStateAndVBH)
{
        INT32 Ret;
        EFI_STATUS Status = EFI_SUCCESS;
        printf ("Entrying Hibernation restore\n");

        if (CheckForValidHeader () < 0) {
                return;
        }

        if (!SetRotAndBootStateAndVBH) {
                printf ("SetRotAndBootStateAndVBH cannot be NULL.\n");
                goto err;
        }

        Status = LoadImageAndAuth (Info, TRUE, FALSE
#ifndef USE_DUMMY_BCC
                                   , &BccParamsRecvdFromAVB
#endif
                                  );
        if (Status != EFI_SUCCESS) {
                printf ("Failed to set ROT and Bootstate : %r\n", Status);
                goto err;
        }

        /* ROT, BootState and VBH are set only once per boot.
         * set variable to TRUE to Avoid setting second time
         * incase hbernation resume fails at restore snapshot
         * stage.
         */
        *SetRotAndBootStateAndVBH = TRUE;
#if !HIBERNATION_TZ_ENCRYPTION
        Status = KeyMasterFbeSetSeed ();
        if (Status != EFI_SUCCESS) {
                printf ("Failed to set seed for fbe : %r\n", Status);
                goto err;
        }
#endif
        Ret = RestoreSnapshotImage ();
        if (Ret) {
                printf ("Failed restore_snapshot_image \n");
                goto err;
        }

        RelocateAddress = GetUnusedPfn () << PAGE_SHIFT;

        /* Reset swap signature now */
        if (!IsSnapshotGolden ()) {
                EraseSwapSignature ();
        }

        CopyBounceAndBootKernel ();
        /* Control should not reach here */

err:    /*
         * Erase swap signature to avoid kernel restoring the
         * hibernation image
         */
        if (!IsSnapshotGolden ()) {
                EraseSwapSignature ();
        }
        return;
}
#endif
