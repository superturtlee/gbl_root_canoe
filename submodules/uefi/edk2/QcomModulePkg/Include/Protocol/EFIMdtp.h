/* Copyright (c) 2016-2017, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of The Linux Foundation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
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

#ifndef __EFIMDTP_H__
#define __EFIMDTP_H__

/*===========================================================================
  INCLUDE FILES
===========================================================================*/

#include <Protocol/EFIVerifiedBoot.h>
#include <Uefi.h>

/*===========================================================================
  MACRO DECLARATIONS
===========================================================================*/
/** @ingroup
  Protocol version.
*/
#define QCOM_MDTP_PROTOCOL_REVISION 0x0000000000010001
#define MAX_PARTITION_NAME_LEN 100

/*  Protocol GUID definition */
/** @ingroup efi_mdtp_protocol */
#define EFI_MDTP_PROTOCOL_GUID                                                 \
  {                                                                            \
    0x71746E63, 0x65F9, 0x41EC,                                                \
    {                                                                          \
      0xAC, 0x08, 0xCD, 0xD1, 0xF2, 0xD0, 0x22, 0x98                           \
    }                                                                          \
  }

/*===========================================================================
  EXTERNAL VARIABLES
===========================================================================*/
/** @ingroup*/
extern EFI_GUID gQcomMdtpProtocolGuid;

/*===========================================================================
  TYPE DEFINITIONS
===========================================================================*/

/** @cond */
/* Protocol declaration.  */
typedef struct _QCOM_MDTP_PROTOCOL QCOM_MDTP_PROTOCOL;
/** @endcond */

typedef struct {
  BOOLEAN VbEnabled;
  CHAR8 PartitionName[MAX_PARTITION_NAME_LEN];
  VOID *ImageBuffer;
  UINT32 ImageSize;
  boot_state_t BootState;
  device_info_vb_t DevInfo;
} MDTP_VB_EXTERNAL_PARTITION;

typedef BOOLEAN MDTP_ISV_PIN;

typedef enum {
  MDTP_STATE_DISABLED, /* MDTP is disabled on the device. */
  MDTP_STATE_INACTIVE, /* MDTP is not activated on the device. */
  MDTP_STATE_ACTIVE,   /* MDTP is activated on the device. */
  MDTP_STATE_TAMPERED, /* MDTP is tampered on the device. */
  MDTP_STATE_INVALID,  /* MDTP is invalid on the device. */
  MDTP_STATE_SIZE = 0x7FFFFFFF
} MDTP_SYSTEM_STATE;

/*===========================================================================
  FUNCTION DEFINITIONS
===========================================================================*/

/**
* Get MDTP state.
*
* @param MDTP_SYSTEM_STATE *MdtpState
*   Set to MDTP state.
*
* @return int
*   Status:
*     0 - Success
*     Negative value indicates failure.
*/
typedef EFI_STATUS (EFIAPI *QCOM_MDTP_GET_STATE) (
    IN QCOM_MDTP_PROTOCOL *This,
    OUT MDTP_SYSTEM_STATE *MdtpState);

/**
* Set MDTP BootState.
* @param MDTP_VB_EXTERNAL_PARTITION *ExternalPartition
*   Pointer to verified boot external partition
*
* @return int
*   Status:
*     0 - Success
*     Negative value indicates failure.
*/
typedef EFI_STATUS (EFIAPI *QCOM_MDTP_SET_BOOT_STATE) (
    IN QCOM_MDTP_PROTOCOL *This,
    IN MDTP_VB_EXTERNAL_PARTITION *ExternalPartition);

/**
* Run MDTP Local Deactivation, displaying Recovery UI
*
* @param MDTP_ISV_PIN *MasterPIN
*   Indicates if this is ISV PIN or not
*
* @return int
*   Status:
*     0 - Success
*     Negative value indicates failure.
*/
typedef EFI_STATUS (EFIAPI *QCOM_MDTP_DEACTIVATE) (IN QCOM_MDTP_PROTOCOL *This,
                                                   OUT MDTP_ISV_PIN MasterPIN);

/*===========================================================================
  PROTOCOL INTERFACE
===========================================================================*/
/** @ingroup
  @par Summary
  Qualcomm MDTP Protocol interface.

  @par Parameters
  @inputprotoparams
*/
struct _QCOM_MDTP_PROTOCOL {
  UINT64 Revision;
  QCOM_MDTP_GET_STATE MdtpGetState;
  QCOM_MDTP_SET_BOOT_STATE MdtpBootState;
  QCOM_MDTP_DEACTIVATE MdtpDeactivate;
};

#endif /* __EFIMDTP_H__ */
