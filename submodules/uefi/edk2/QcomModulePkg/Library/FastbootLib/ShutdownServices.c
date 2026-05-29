/** @file
 *
 *  Copyright (c) 2011-2015, ARM Limited. All rights reserved.
 *
 *  This program and the accompanying materials
 *  are licensed and made available under the terms and conditions of the BSD
 *License
 *  which accompanies this distribution.  The full text of the license may be
 *found at
 *  http://opensource.org/licenses/bsd-license.php
 *
 *  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR
 *IMPLIED.
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

#include "ShutdownServices.h"

#include <FastbootLib/FastbootCmds.h>
#include <Guid/ArmMpCoreInfo.h>
#include <Guid/FileInfo.h>
#include <Guid/GlobalVariable.h>
#include <Library/ArmLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/HobLib.h>
#include <Library/LinuxLoaderLib.h>
#include <Library/PrintLib.h>
#include <Library/SerialPortLib.h>
#include <Library/TimerLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

EFI_STATUS ShutdownUefiBootServices (VOID)
{
  EFI_STATUS Status;
  UINTN MemoryMapSize;
  EFI_MEMORY_DESCRIPTOR *MemoryMap;
  UINTN MapKey;
  UINTN DescriptorSize;
  UINT32 DescriptorVersion;
  UINTN Pages;

  WaitForFlashFinished ();

  MemoryMap = NULL;
  MemoryMapSize = 0;
  Pages = 0;

  do {
    Status = gBS->GetMemoryMap (&MemoryMapSize, MemoryMap, &MapKey,
                                &DescriptorSize, &DescriptorVersion);
    if (Status == EFI_BUFFER_TOO_SMALL) {

      Pages = EFI_SIZE_TO_PAGES (MemoryMapSize) + 1;
      MemoryMap = AllocatePages (Pages);
      if (!MemoryMap) {
        DEBUG ((EFI_D_ERROR, "Failed to allocate pages for memory map\n"));
        return EFI_OUT_OF_RESOURCES;
      }

      //
      // Get System MemoryMap
      //
      Status = gBS->GetMemoryMap (&MemoryMapSize, MemoryMap, &MapKey,
                                  &DescriptorSize, &DescriptorVersion);
    }

    // Don't do anything between the GetMemoryMap() and ExitBootServices()
    if (!EFI_ERROR (Status)) {
      Status = gBS->ExitBootServices (gImageHandle, MapKey);
      if (EFI_ERROR (Status)) {
        FreePages (MemoryMap, Pages);
        MemoryMap = NULL;
        MemoryMapSize = 0;
      }
    }
  } while (EFI_ERROR (Status));

  return Status;
}

#ifdef DISABLE_KERNEL_PROTOCOL
EFI_STATUS PreparePlatformHardware (VOID)
{
  ArmDisableBranchPrediction ();

  /* ArmDisableAllExceptions */
  ArmDisableInterrupts ();
  ArmDisableAsynchronousAbort ();

  ArmCleanInvalidateDataCache ();
  ArmCleanDataCache ();
  ArmInvalidateInstructionCache ();

  ArmDisableDataCache ();
  ArmDisableInstructionCache ();
  ArmDisableMmu ();
  ArmInvalidateTlb ();
  return EFI_SUCCESS;
}
#else
EFI_STATUS PreparePlatformHardware (EFI_KERNEL_PROTOCOL *KernIntf,
    VOID *KernelLoadAddr, UINTN KernelSizeActual, VOID *RamdiskLoadAddr,
    UINTN RamdiskSizeActual, VOID *DeviceTreeLoadAddr,
    UINTN DeviceTreeSizeActual, VOID *CallerStackCurrent, UINTN CallerStackBase)
{
  Thread *ThreadNum;
  VOID *StackBase;
  VOID **StackCurrent;

  if (KernIntf->Version >= EFI_KERNEL_PROTOCOL_VERSION) {
    ThreadNum = KernIntf->Thread->GetCurrentThread ();
    StackCurrent = KernIntf->Thread->ThreadGetUnsafeSPCurrent (ThreadNum);
    StackBase = KernIntf->Thread->ThreadGetUnsafeSPBase (ThreadNum);
  }

  ArmDisableBranchPrediction ();

  /* ArmDisableAllExceptions */
  ArmDisableInterrupts ();
  ArmDisableAsynchronousAbort ();

  // Clean, invalidate, disable data cache
  WriteBackInvalidateDataCacheRange (KernelLoadAddr, KernelSizeActual);
  WriteBackInvalidateDataCacheRange (RamdiskLoadAddr, RamdiskSizeActual);
  WriteBackInvalidateDataCacheRange (DeviceTreeLoadAddr, DeviceTreeSizeActual);
  if (KernIntf->Version >= EFI_KERNEL_PROTOCOL_VERSION) {
    WriteBackInvalidateDataCacheRange ((VOID *)StackCurrent,
                  (UINTN)StackBase - (UINTN)StackCurrent);
    WriteBackInvalidateDataCacheRange (CallerStackCurrent,
                  CallerStackBase - (UINTN)CallerStackCurrent);
  }

  ArmCleanDataCache ();
  ArmInvalidateInstructionCache ();

  ArmDisableDataCache ();
  ArmDisableInstructionCache ();
  ArmDisableMmu ();
  ArmInvalidateTlb ();
  return EFI_SUCCESS;
}
#endif

VOID
RebootDevice (UINT8 RebootReason)
{
  ResetDataType ResetData;
  EFI_STATUS Status = EFI_INVALID_PARAMETER;

  WaitForFlashFinished ();

  StrnCpyS (ResetData.DataBuffer, ARRAY_SIZE (ResetData.DataBuffer),
            (CONST CHAR16 *)STR_RESET_PARAM, ARRAY_SIZE (STR_RESET_PARAM) - 1);
  ResetData.Bdata = RebootReason;
  if (RebootReason == NORMAL_MODE)
    Status = EFI_SUCCESS;

  if (RebootReason == EMERGENCY_DLOAD)
    gRT->ResetSystem (EfiResetPlatformSpecific, EFI_SUCCESS,
                      StrSize ((CONST CHAR16 *)STR_RESET_PLAT_SPECIFIC_EDL),
                      STR_RESET_PLAT_SPECIFIC_EDL);

  gRT->ResetSystem (EfiResetCold, Status, sizeof (ResetDataType),
                    (VOID *)&ResetData);
}

VOID ShutdownDevice (VOID)
{
  EFI_STATUS Status = EFI_INVALID_PARAMETER;

  WaitForFlashFinished ();

  gRT->ResetSystem (EfiResetShutdown, Status, 0, NULL);

  /* Flow never comes here and is fatal if it comes here.*/
  ASSERT (0);
}
