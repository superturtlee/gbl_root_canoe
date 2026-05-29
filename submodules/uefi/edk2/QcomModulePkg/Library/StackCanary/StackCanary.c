/* Copyright (c) 2016-2018, The Linux Foundation. All rights reserved.
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

#include "LinuxLoaderLib.h"
#include <Library/StackCanary.h>
#include <Protocol/EFIRng.h>

UINTN __stack_chk_guard = 0xc0c0c0c0;

VOID StackGuardChkSetup (VOID)
{
  EFI_QCOM_RNG_PROTOCOL *RngIf;
  EFI_STATUS Status;

  Status = gBS->LocateProtocol (&gQcomRngProtocolGuid, NULL, (VOID **)&RngIf);
  if (Status != EFI_SUCCESS) {
    return;
  }

  Status = RngIf->GetRNG (RngIf, &gEfiRNGAlgRawGuid, sizeof (UINTN),
                          (UINT8 *)&__stack_chk_guard);
  if (Status != EFI_SUCCESS) {
    return;
  }
}

/*
 * Callback if stack cananry is corrupted
 */
void
__stack_chk_fail (void)
{
  volatile UINT32 i = 1;
  /*
   * Loop forever in case of stack overflow. Avoid
   * calling into another api in case of stack corruption/overflow
   */
  while (i)
    ;
}
