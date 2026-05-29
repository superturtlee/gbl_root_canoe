/* Copyright (c) 2016, The Linux Foundation. All rights reserved.
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
#ifndef __EFISCM_MS_H__
#define __EFISCM_MS_H__

/*===========================================================================
  INCLUDE FILES
===========================================================================*/
#include <Uefi.h>

/*===========================================================================
  MACRO DECLARATIONS
===========================================================================*/
/** @ingroup efi_scm_mode_swith_constants
  Protocol version.
*/
#define QCOM_SCM_MODE_SWITCH_PROTOCOL_REVISION 0x0000000000000001

/*===========================================================================
  EXTERNAL VARIABLES
===========================================================================*/
/** @ingroup efi_scm_mode_swith_protocol */
extern EFI_GUID gQcomScmModeSwithProtocolGuid;

/*===========================================================================
  TYPE DEFINITIONS
===========================================================================*/
typedef struct _EFI_HLOS_BOOT_ARGS EFI_HLOS_BOOT_ARGS;

struct _EFI_HLOS_BOOT_ARGS {
  UINT64 el1_x0;
  UINT64 el1_x1;
  UINT64 el1_x2;
  UINT64 el1_x3;
  UINT64 el1_x4;
  UINT64 el1_x5;
  UINT64 el1_x6;
  UINT64 el1_x7;
  UINT64 el1_x8;
  UINT64 el1_elr;
} __attribute__ ((packed));

/* QCOM_SCM_SYS_CALL */
/** @ingroup efi_scm_mode_switch_sys_call
  @par Summary
  Call to perform mode switch. This function does not return upon success

  @param[in]     HlosBootInfo  Structure containing entry point, mode value and
                                 arguments to entry point

  @return
  Error values  -- In case of failure; Does not return upon succes \n
*/

typedef EFI_STATUS (EFIAPI *QCOM_SWITCH_TO_32BIT_MODE) (
    IN EFI_HLOS_BOOT_ARGS HlosBootInfo);

/*===========================================================================
  PROTOCOL INTERFACE
===========================================================================*/
/** @ingroup efi_scm_mode_switch_sys_call
  @par Summary
  Qualcomm Technologies Inc Mode Switch Protocol interface.

  @par Parameters
  @inputprotoparams{scm_proto_params.tex}
*/
typedef struct _QCOM_SCM_MODE_SWITCH_PROTOCOL {
  QCOM_SWITCH_TO_32BIT_MODE SwitchTo32bitMode;
} QCOM_SCM_MODE_SWITCH_PROTOCOL;

#endif /* __EFISCM_H__ */
