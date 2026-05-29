/* Copyright (c) 2018, The Linux Foundation. All rights reserved.
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

#ifndef __EFI_NAND_PARTI_GUID_PROTOCOL_H__
#define __EFI_NAND_PARTI_GUID_PROTOCOL_H__

/*===========================================================================
  MACRO DECLARATIONS
===========================================================================*/
/** @cond */
typedef struct _EFI_NAND_PARTI_GUID_PROTOCOL EFI_NAND_PARTI_GUID_PROTOCOL;
/** @endcond */

/** @addtogroup efi_nand_parti_guid_constants
@{ */
/** Protocol version. */
#define EFI_NAND_PARTI_GUID_PROTOCOL_REVISION 0x000000010000000
/** @} */ /* end_addtogroup efi_nand_io_constants */

/* Protocol GUID definition */
/** @ingroup efi_nand_parti_guid_protocol */
#define EFI_QCOM_NAND_PARTI_GUID_PROTOCOL_GUID \
   { 0xd68edce2, 0xa314, 0x457b, { 0x96, 0x2a, 0x1d, 0x99, 0xbb, 0xfc, 0xbb, \
     0xfb }}

/** @cond */

/*===========================================================================
  EXTERNAL VARIABLES
===========================================================================*/
/**
  External reference to the EFINandIo Protocol GUID.
 */
/** @ingroup efi_nand_io_protocol */
extern EFI_GUID gEfiNandPartiGuidProtocolGuid;

/*===========================================================================
  TYPE DEFINITIONS
===========================================================================*/

/*===========================================================================
  FUNCTION DEFINITIONS
===========================================================================*/

/*
  @par Summary
  Generate NAND partition GUID hashed with NAND partition name.

  @param[in]   This               Pointer to the EFI_NAND_PARTI_GUID_PROTOCOL
                                  instance.
  @param[in]   *PartiName         Pointer to partition name's data buffer.
  @param[in]   PartiNameLen       Partition name string size.
  @param[Out]  *PartiGuid         Pointer to partition name's GUID.

  @retval     EFI_SUCCESS         Partition GUID was successfully generated.
  @retval     EFI_DEVICE_ERROR    Partition GUID generation failed.

  @dependencies  none.
*/
typedef
EFI_STATUS
(EFIAPI *EFI_NAND_PARTI_GUID_GEN_GUID) (
   IN EFI_NAND_PARTI_GUID_PROTOCOL      *This,
   IN CONST CHAR16                      *PartiName,
   IN CONST UINT32                      PartiNameLen,
   OUT EFI_GUID                         *PartiGuid
);

/*===========================================================================
  PROTOCOL INTERFACE
===========================================================================*/
/** @ingroup efi_nand_parti_guid_protocol
  @par Summary
  This protocol provides a method to generate NAND partition GUID.

  @par Parameters
  @inputprotoparams{nand_parti_guid_params.tex}
*/
struct _EFI_NAND_PARTI_GUID_PROTOCOL {
  UINTN                          Revision;
  EFI_NAND_PARTI_GUID_GEN_GUID   GenGuid;
};

#endif //ifndef __EFI_NAND_PARTI_GUID_PROTOCOL_H__

