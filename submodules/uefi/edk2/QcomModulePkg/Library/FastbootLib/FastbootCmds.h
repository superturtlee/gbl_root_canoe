/*
 * Copyright (c) 2009, Google Inc.
 * All rights reserved.
 *
 * Copyright (c) 2015-2018, 2020-2021, The Linux Foundation.
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

#ifndef __FASTBOOT_CMDS_H__
#define __FASTBOOT_CMDS_H__

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/Debug.h>
#include <Library/LinuxLoaderLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PartitionTableUpdate.h>
#include <Protocol/EFIKernelInterface.h>

#include "Board.h"

#define ENDPOINT_IN 0x01
#define ENDPOINT_OUT 0x81

#define MAX_WRITE_SIZE (1024 * 1024)
#define MAX_RSP_SIZE 64
#define ERASE_BUFF_SIZE 256 * 1024
#define ERASE_BUFF_BLOCKS 256 * 2

#define USB_BUFFER_SIZE \
  (1024 * 1024 * 16)

#define VERSION_STR_LEN 96
#define FASTBOOT_STRING_MAX_LENGTH 256
#define FASTBOOT_COMMAND_MAX_LENGTH 64
#define MAX_GET_VAR_NAME_SIZE 36
#define ASCII_HEX_STRING_MAX_LENGTH 8
#define SIGACTUAL 4096
#define SLOT_SUFFIX_ARRAY_SIZE 10
#define SLOT_ATTR_SIZE 32
#define ATTR_RESP_SIZE 4
#define MAX_FASTBOOT_COMMAND_SIZE 64
#define RECOVERY_WIPE_DATA                                                     \
  "recovery\n--wipe_data\n--reason=MasterClearConfirm\n--locale=en_US\n"

/* Fs detection macros  and definitions */
#define RAW_FS_STR "raw"
#define EXT_FS_STR "ext4"
#define F2FS_FS_STR "f2fs"
#define FS_SUPERBLOCK_OFFSET 0x400
#define EXT_MAGIC_OFFSET_SB 0x38
#define EXT_FS_MAGIC 0xEF53
#define F2FS_MAGIC_OFFSET_SB 0x0
#define F2FS_FS_MAGIC 0xF2F52010

/* Divide allocatable free memory by 3/4ths or 85/100ths */
#define EFI_FREE_MEM_DIVISOR(BYTES) \
(((BYTES) * 3) / 4)

/* 64MB */
#define MIN_BUFFER_SIZE (67108864)
/* 1.5GB */
#define MAX_BUFFER_SIZE (1610612736)

typedef enum FsSignature {
  EXT_FS_SIGNATURE = 1,
  F2FS_FS_SIGNATURE,
  UNKNOWN_FS_SIGNATURE
} FS_SIGNATURE;

typedef void (*fastboot_cmd_fn) (const char *, void *, unsigned);

/* Fastboot Command descriptor */
struct FastbootCmdDesc {
  CHAR8 *name;
  fastboot_cmd_fn cb;
};

/* Fastboot Variable list */
typedef struct _FASTBOOT_VAR {
  struct _FASTBOOT_VAR *next;
  CONST CHAR8 *name;
  CONST CHAR8 *value;
} FASTBOOT_VAR;

/* Partition info fastboot variable */
struct GetVarPartitionInfo {
  const CHAR8 part_name[MAX_GET_VAR_NAME_SIZE];
  CHAR8 getvar_size_str[MAX_GET_VAR_NAME_SIZE];
  CHAR8 getvar_type_str[MAX_GET_VAR_NAME_SIZE];
  CHAR8 size_response[MAX_RSP_SIZE];
  CHAR8 type_response[MAX_RSP_SIZE];
};

/* Fastboot State */
typedef enum {
  ExpectCmdState,
  ExpectDataState,
  FastbootStateMax
} ANDROID_FASTBOOT_STATE;

/* Data structure to store the command list */
typedef struct _FASTBOOT_CMD {
  struct _FASTBOOT_CMD *next;
  CONST CHAR8 *prefix;
  UINT32 prefix_len;
  VOID (*handle) (CONST CHAR8 *arg, VOID *data, UINT32 sz);
} FASTBOOT_CMD;

/* Returns the number of bytes left in the
 * download. You must be expecting a download to
 * call this  function
 */
UINTN GetXfrSize (VOID);

/* Registers commands and publishes Variables */
EFI_STATUS
FastbootEnvSetup (VOID *xfer_buffer, UINT32 max);

/* register a command handler
 * - command handlers will be called if their prefix matches
 * - they are expected to call fastboot_okay() or fastboot_fail()
 *   to indicate success/failure before returning
 */
VOID
FastbootRegister (CONST CHAR8 *prefix,
                  VOID (*handle) (CONST CHAR8 *arg, VOID *data, UINT32 size));

/* Only callable from within a command handler
 * One of thse functions must be called to be a valid command
 */
VOID
FastbootOkay (CONST CHAR8 *result);
VOID
FastbootFail (CONST CHAR8 *reason);
VOID
FastbootInfo (CONST CHAR8 *Info);

/* Initializes the Fastboot App */
EFI_STATUS
FastbootCmdsInit (VOID);

/* Uninitializes the Fastboot App */
EFI_STATUS
FastbootCmdsUnInit (VOID);

/* Called when a message/download data passed to the app */
VOID
DataReady (IN UINT64 Size, IN VOID *Data);

BOOLEAN FastbootFatal (VOID);
VOID PartitionDump (VOID);

VOID *FastbootDloadBuffer (VOID);

ANDROID_FASTBOOT_STATE FastbootCurrentState (VOID);

EFI_STATUS
UpdateDevInfo (CHAR16 *Pname, CHAR8 *ImgVersion);
VOID
GetDevInfo (DeviceInfo **DevinfoPtr);
BOOLEAN IsFlashSplitNeeded (VOID);
BOOLEAN FlashComplete (VOID);
BOOLEAN IsDisableParallelDownloadFlash (VOID);
BOOLEAN IsUseMThreadParallel (VOID);
VOID ThreadSleep (TimeDuration Delay);
VOID WaitForFlashFinished (VOID);
#endif
