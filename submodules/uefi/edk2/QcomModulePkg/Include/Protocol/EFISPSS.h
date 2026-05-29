/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef __EFISPSS_H__
#define __EFISPSS_H__

/*===========================================================================
  INCLUDE FILES
===========================================================================*/

#include <Uefi.h>
#include "KeymasterClient.h"

/*===========================================================================
  MACRO DECLARATIONS
===========================================================================*/

/*  Protocol GUID definition */
/** @ingroup efi_spss_protocol */
#define EFI_SPSS_PROTOCOL_GUID                                                 \
  {                                                                            \
    0xa322ff2c, 0x6d1a, 0x44de,                                                \
    {                                                                          \
      0xa4, 0x70, 0xc0, 0xa8, 0x9e, 0x48, 0xc2, 0xe6                           \
    }                                                                          \
  }

#define SHA256_SIZE 32

/*===========================================================================
  EXTERNAL VARIABLES
===========================================================================*/

/** @ingroup */
extern EFI_GUID gEfiSPSSProtocolGuid;

/*===========================================================================
  TYPE DEFINITIONS
===========================================================================*/

typedef struct KeymintSharedInfoStruct_t {
  KMSetRotReq RootOfTrust;
  KMSetBootStateReq BootInfo;
  KMSetVbhReq Vbh;
} __attribute__ ((packed)) KeymintSharedInfoStruct;

/*===========================================================================
  FUNCTION DEFINITIONS
===========================================================================*/

/**
* Share Keymint info with SPSS Driver
*
* @param KeymintSharedInfoStruct  *KeymintSharedInfo
*   Keymint shared info
*
* @return int
*   Status:
*     0 - Success
*     Negative value indicates failure.
*/
typedef
EFI_STATUS
(EFIAPI *SPSS_SHARE_KEYMINT_INFO)
(
  IN  KeymintSharedInfoStruct  *KeymintSharedInfo
);

/*===========================================================================
  PROTOCOL INTERFACE
===========================================================================*/

/** @ingroup
  @par Summary
    Android SPSS Protocol interface.

  @par Parameters
  @inputprotoparams
*/
typedef struct _SPSS_PROTOCOL {
  SPSS_SHARE_KEYMINT_INFO      SPSSDxe_ShareKeyMintInfo;
} SpssProtocol;

#endif /* __EFISPSS_H__ */