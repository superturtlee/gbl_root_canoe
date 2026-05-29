/* Copyright (c) 2015-2016,2019 The Linux Foundation. All rights reserved.
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
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted (subject to the limitations in the
 *  disclaimer below) provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 *
 *   * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 *  GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 *  HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
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

#ifndef __EFICARDINFO_H__
#define __EFICARDINFO_H__

/*===========================================================================
  INCLUDE FILES
===========================================================================*/

/*===========================================================================
  MACRO DECLARATIONS
===========================================================================*/
/** @addtogroup efi_cardInfo_constants
@{ */
/**
  Protocol version.
*/
#define EFI_MEM_CARD_INFO_PROTOCOL_REVISION_4 0x0000000000010004
#define EFI_MEM_CARD_INFO_PROTOCOL_REVISION_3 0x0000000000010003
#define EFI_MEM_CARD_INFO_PROTOCOL_REVISION EFI_MEM_CARD_INFO_PROTOCOL_REVISION_4

/** @} */ /* end_addtogroup efi_cardInfo_constants */

/*  Protocol GUID definition */
/** @ingroup efi_cardInfo_protocol */
#define EFI_CARD_INFO_PROTOCOL_GUID \
   { 0x85C1F7D2, 0xBCE6, 0x4F31, { 0x8F, 0x4D, 0xD3, 0x7E, 0x03, 0xD0, 0x5E, 0xAA } }

/** @cond */
/*===========================================================================
  EXTERNAL VARIABLES
===========================================================================*/
/**
  External reference to the EFICardInfo Protocol GUID.
 */
extern EFI_GUID gEfiMemCardInfoProtocolGuid;

/*===========================================================================
  TYPE DEFINITIONS
===========================================================================*/
/**
  Protocol declaration.
 */
typedef struct _EFI_MEM_CARDINFO_PROTOCOL EFI_MEM_CARDINFO_PROTOCOL;
/** @endcond */

/** @addtogroup efi_cardInfo_data_types
@{ */

/**
 SDCC/UFS unified Card information.
*/
typedef struct _CARD_INFO {
  UINT16 mfr_id;                 /**< Manufacturer ID. */
  UINT16 oem_id;                 /**< Original equipment manufacturer ID. */
  UINT8 mfr_date[8];             /**< Manufacture date. */
  UINT8 product_serial_num[252]; /**< Product serial number. */
  UINT32 serial_num_len;         /**< Product serial number length. */
  UINT8 inquiry_str[29];         /**< Output from SCSI inquiry command. */
  UINT32 rpmb_size_in_byte;      /**< Replay protected memory block partition
                                      size in bytes. */
  UINT32 reliable_write_count;   /**< Reliable write count. */
  UINT8 card_type[4];            /**< Type of the card. 'UFS' or 'MMC' */
} /** @cond */ MEM_CARD_INFO /** @endcond */;
/** @} */ /* end_addtogroup efi_cardInfo_data_types */

/** @ingroup efi_mem_card_media
  @par Summary
  MEM Card media information

  @par Parameters
*/
typedef struct
{
  // The curent media Id. If the media changes, this value is changed.
  UINT32     MediaId;
  // TRUE if the media is removable; otherwise, FALSE.
  //"RemovableMedia" : Media that supports hotplug or runtime removal/insertion.
  BOOLEAN    RemovableMedia;
}EFI_MEM_CARD_MEDIA;

/*===========================================================================
  FUNCTION DEFINITIONS
===========================================================================*/
/* GET_MEM_CARD_INFO */
/** @ingroup efi_cardInfo_get_card_info
  @par Summary
  Gets the card information.

  @param[in]   This              Pointer to the
                                 EFI_MEM_CARDINFO_PROTOCOL
                                 instance.
  @param[out]  card_info         Pointer to a variable that the driver returns
                                 with the retrieved card information; see
                                 #_MEM_CARD_INFO for details.

  @return
  EFI_SUCCESS           -- Function completed successfully. \n
  EFI_INVALID_PARAMETER -- Parameter is invalid.
*/
typedef EFI_STATUS (EFIAPI *GET_MEM_CARD_INFO) (
    IN EFI_MEM_CARDINFO_PROTOCOL *This,
    OUT MEM_CARD_INFO *card_info);

/* GET_BOOT_LU */
/** @ingroup efi_cardInfo_get_boot_lu
  @par Summary
  Gets the BOOT LU

  @param[in]   This              Pointer to the
                                 EFI_MEM_CARDINFO_PROTOCOL
                                 instance.
  @param[out]  BootLU            Pointer to a variable
                                 that indicates the BOOT LU
                 (0 = disable BOOT LU, 1 = BOOT for A,
                  2 = BOOT for B)

  @return
  EFI_SUCCESS           -- Function completed successfully. \n
  EFI_INVALID_PARAMETER -- Parameter is invalid.
*/
typedef EFI_STATUS (EFIAPI *GET_BOOT_LU) (IN EFI_MEM_CARDINFO_PROTOCOL *This,
                                          OUT UINT32 *BootLU);
/* SET_BOOT_LU */
/** @ingroup efi_cardInfo_set_boot_lu
  @par Summary
  Set the BOOT LU.

  @param[in]   This              Pointer to the
                                 EFI_MEM_CARDINFO_PROTOCOL
                                 instance.
  @param[in]   BootLU            Boot LU to switch to
                 (0 = disable BOOT LU, 1 = BOOT for A,
                  2 = BOOT for B)

  @return
  EFI_SUCCESS           -- Function completed successfully. \n
  EFI_INVALID_PARAMETER -- Parameter is invalid.
*/
typedef EFI_STATUS (EFIAPI *SET_BOOT_LU) (IN EFI_MEM_CARDINFO_PROTOCOL *This,
                                          IN UINT32 BootLU);

/* GET_AVAIL_BUFFER_SIZE */
/** @ingroup efi_card_info_protocol_apis
  @par Summary
  Get the available buffer size.

  @param[in]   This              Pointer to the
                                 EFI_MEM_CARDINFO_PROTOCOL
                                 instance.
  @param[in]   AvailBufferSize   Write booster available buffer size

  @return
  EFI_SUCCESS           -- Function completed successfully. \n
  EFI_INVALID_PARAMETER -- Parameter is invalid.
*/
typedef
EFI_STATUS
(EFIAPI *GET_AVAIL_BUFFER_SIZE)(
  IN  EFI_MEM_CARDINFO_PROTOCOL *This,
  OUT UINT32                    *AvailBufferSize
);

/* GET_CUR_BUFFER_SIZE */
/** @ingroup efi_card_info_protocol_apis
  @par Summary
  Get the current buffer size.

  @param[in]   This              Pointer to the
                                 EFI_MEM_CARDINFO_PROTOCOL
                                 instance.
  @param[in]   CurBufferSize   Write booster current buffer size

  @return
  EFI_SUCCESS           -- Function completed successfully. \n
  EFI_INVALID_PARAMETER -- Parameter is invalid.
*/
typedef
EFI_STATUS
(EFIAPI *GET_CUR_BUFFER_SIZE)(
  IN  EFI_MEM_CARDINFO_PROTOCOL *This,
  OUT UINT32                    *CurBufferSize
);

/*===========================================================================
  PROTOCOL INTERFACE
===========================================================================*/
/** @ingroup efi_cardInfo_protocol
  @par Summary
  Secure Digital Card Controller (SDCC) Card/Universal Flash Storage (UFS) Card
  Information Protocol interface.

  @par Parameters
  @inputprotoparams{card_info_proto_params.tex}
*/
struct _EFI_MEM_CARDINFO_PROTOCOL {
  UINT64 Revision;
  GET_MEM_CARD_INFO GetCardInfo;
  GET_BOOT_LU GetBootLU;
  SET_BOOT_LU SetBootLU;
  GET_AVAIL_BUFFER_SIZE  GetAvailBufferSize;
  GET_CUR_BUFFER_SIZE    GetCurBufferSize;
  EFI_MEM_CARD_MEDIA     *Media;              /* Added in Revision 1004 */
};

#endif /* __EFICARDINFO_H__ */
