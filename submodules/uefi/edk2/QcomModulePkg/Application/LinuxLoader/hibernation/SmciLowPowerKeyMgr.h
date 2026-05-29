/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef __LOWPOWER_KEY_MANAGER_H__
#define __LOWPOWER_KEY_MANAGER_H__

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>

#define CLOWPOWERKEYMANAGER_UID 0x13B

#define ILOWPOWERKEYMANAGER_HIBERNATE 1
#define ILOWPOWERKEYMANAGER_HIBERNATE_WITH_ENCRYPTION 2

#define ILOWPOWERKEYMANAGER_ERROR_INVALID_EVENT 10
#define ILOWPOWERKEYMANAGER_ERROR_INVALID_OPERATION 11
#define ILOWPOWERKEYMANAGER_ERROR_INVALID_KEYSIZE 12
#define ILOWPOWERKEYMANAGER_ERROR_KEY_GENERATION 13
#define ILOWPOWERKEYMANAGER_ERROR_RPMB_OPERATION 14

#define ILOWPOWERKEYMANAGER_OP_GETKEY 0
#define ILOWPOWERKEYMANAGER_OP_PREPARE 1
#define ILOWPOWERKEYMANAGER_OP_RESERVED 2

static inline INT32
ILowPowerKeyManagerGetKey (Object Self,
                uint32_t Event,
                VOID *OutputPtr,
                size_t OutputLen,
                size_t *OutputLenout)
{
  ObjectArg ArgA[2] = {{{0, 0}}};
  ArgA[0].ArgB = (ObjectBuf) {&Event, sizeof(uint32_t)};
  ArgA[1].ArgB = (ObjectBuf) {OutputPtr, OutputLen * 1};

  INT32 Result = ObjectInvokeFunc (Self, ILOWPOWERKEYMANAGER_OP_GETKEY, ArgA,
                                     ObjectCounts_pack (1, 1, 0, 0));

  *OutputLenout = ArgA[1].ArgB.Size / 1;

  return Result;
}

#endif // __LOWPOWER_KEY_MANAGER_H__

