/* Copyright (c) 2015-2017, 2020-2021, The Linux Foundation.
 * All rights reserved.
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
 * Copyright (c) 2022-2023 Qualcomm Innovation Center, Inc. All rights reserved.
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

#ifndef __UPDATEDEVICETREE_H__
#define __UPDATEDEVICETREE_H__

#include "libfdt.h"
#include <Library/Board.h>
#include <Library/DebugLib.h>
#include <Library/Debug.h>
#include <Library/LinuxLoaderLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Protocol/EFILimits.h>
#include <Protocol/EFIRamPartition.h>
#include <Uefi.h>

#define DDR_MAX_RANKS 2

#define DTB_MAX_SUBNODE 128

#define MSMCOBALT_PGOOD_FUSE 0x78013C
#define MSMCOBALT_PGOOD_SUBBIN_FUSE 0x780324
#define DTB_OFFSET_LOCATION_IN_ARCH32_KERNEL_HDR 0x2C

#define PARTIAL_GOOD_GOLD_DISABLE 0x1

/* Return True if integer overflow will occur */
#define CHECK_ADD64(a, b) ((MAX_UINT64 - b < a) ? TRUE : FALSE)

/* DDR region information have the layout like below, each item have 32bit:
 * region = < StartAddr_high StartAddr_low
 *                RegionsSize_high RegionsSize_low
 *                SegmentsStartOffset_high SegmentsStartOffset_low
 *                SegmentsStartIndex_high SegmentsStartIndex_low
 *                GranuleSize_high GranuleSize_low
*/
#define MAX_DDR_REGION_PROP_MEM 10

/* Look up table for fstab node */
struct FstabNode {
  CONST CHAR8 *ParentNode; /* Parent Node name */
  CONST CHAR8 *Property;   /* Property Name */
  CONST CHAR8 *DevicePathId;
};

struct CarveoutMemRegion {
  UINT64 StartAddr;
  UINT64 Size;
};

struct DisplaySplashBufferInfo {
  /* Version number used to track changes to the structure */
  UINT32 uVersion;
  /* Physical address of the frame buffer */
  UINT32 uFrameAddr;
  /* Frame buffer size */
  UINT32 uFrameSize;
};

#pragma pack(push)
#pragma pack(1)
/* Display demura parameters */
struct DisplayDemuraInfoType {
  UINT32 Version;                /* Version info of this structure */
  UINT64 Demura0PanelID;         /* Demura 0 panel ID              */
  UINT32 Demura0HFCAddr;         /* Demura 0 HFC data address      */
  UINT32 Demura0HFCSize;         /* Demura 0 HFC data size         */
  UINT64 Demura1PanelID;         /* Demura 1 panel ID              */
  UINT32 Demura1HFCAddr;         /* Demura 1 HFC data address      */
  UINT32 Demura1HFCSize;         /* Demura 1 HFC data size         */
};
#pragma pack(pop)

#define DDR_REGIONS_MASK 32
#define DDR_REGIONS_LOW_MASK 0xffffffff
#define DDR_REGION_NAME_LEN 8
#define DDR_REGION_NAME_SUFFIX 2

INT32
dev_tree_add_mem_info (VOID *fdt, UINT32 offset, UINT32 addr, UINT32 size);

INT32
dev_tree_add_mem_infoV64 (VOID *fdt, UINT32 offset, UINT64 addr, UINT64 size);

EFI_STATUS
UpdateDeviceTree (VOID *DeviceTreeLoadAddr,
                  CONST CHAR8 *CmdLine,
                  VOID *RamDiskLoadAddr,
                  UINT32 RamDiskSize,
                  BOOLEAN BootingWith32BitKernel);

EFI_STATUS
UpdateFstabNode (VOID *fdt);

UINT32
fdt_check_header_ext (VOID *fdt);

EFI_STATUS
GetUpdatedRamPartitions (VOID *Fdt,
                         RamPartitionEntry *RamPartitions,
                         UINT32 NumPartitions,
                         RamPartitionEntry *UpdatedRamPartitions,
                         UINT32 *NumUpdPartitions);

BOOLEAN
IsCarveoutRemovalEnabled (VOID);
#endif
