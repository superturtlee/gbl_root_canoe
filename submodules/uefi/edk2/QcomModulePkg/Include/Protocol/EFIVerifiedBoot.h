/* Copyright (c) 2015-2016, The Linux Foundation. All rights reserved.
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
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
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

#ifndef __EFIVERIFIEDBOOT_H__
#define __EFIVERIFIEDBOOT_H__

/*===========================================================================
  INCLUDE FILES
===========================================================================*/
#include <Uefi.h>

/*===========================================================================
  MACRO DECLARATIONS
===========================================================================*/
/** @ingroup
  Protocol version.
*/
#define QCOM_VERIFIEDBOOT_PROTOCOL_REVISION 0x0000000000010003
#define QCOM_VERIFIEDBOOT_PROTOCOL_FIRST_REVISION 0x0000000000010001

#define MAX_PNAME_LENGTH 32
#define MAX_VERSION_LEN 64

/*  Protocol GUID definition */
/** @ingroup efi_verifiedboot_protocol */
#define EFI_VERIFIEDBOOT_PROTOCOL_GUID                                         \
  {                                                                            \
    0x8e5eff91, 0x21b6, 0x47d3,                                                \
    {                                                                          \
      0xaf, 0x2b, 0xc1, 0x5a, 0x1, 0xe0, 0x20, 0xec                            \
    }                                                                          \
  }

/*===========================================================================
  EXTERNAL VARIABLES
===========================================================================*/
/** @ingroup */
extern EFI_GUID gEfiQcomVerifiedBootProtocolGuid;

/*===========================================================================
  TYPE DEFINITIONS
===========================================================================*/

/** @cond */
/* Protocol declaration.  */
typedef struct _QCOM_VERIFIEDBOOT_PROTOCOL QCOM_VERIFIEDBOOT_PROTOCOL;
/** @endcond */

/*
 * Device State operations
 */
typedef enum _vb_device_state_op_t {
  READ_CONFIG,
  WRITE_CONFIG,
  DEVICE_STATE_MAX = (int)0xFFFFFFFFULL,
} vb_device_state_op_t;

typedef enum _boot_state_t {
  GREEN,
  ORANGE,
  YELLOW,
  RED,
  BOOT_STATE_MAX = (int)0xFFFFFFFFULL,
} boot_state_t;

typedef struct _device_info_vb_t {
  BOOLEAN is_unlocked;
  BOOLEAN is_unlock_critical;
} device_info_vb_t;

/*===========================================================================
  FUNCTION DEFINITIONS
===========================================================================*/
/**
* Read/ Write Device state info for VB
* For Write: The in_buf has the data to be written
* and the size of the data is provided in in_buf_len.
* out_buf will be ignored
* For Read: The api will return data read in out_buf
* and the size of the data is provided in out_buf_len.
* in_buf will be ignored
*
* @param vb_device_state_op_t    op
*   Enum to select read or write.
*
* @param void *buf
*   Pointer to the buffer for read and write
*
* @param uint32_t *buf_len
*   Size of the buffer
*
* @return int
*   Success:	handle to be used for all calls to
*   			Secure app. Always greater than zero.
*   Failure:	Error code (negative only).
*/

typedef EFI_STATUS (EFIAPI *QCOM_VB_RW_DEVICE_STATE) (
    IN QCOM_VERIFIEDBOOT_PROTOCOL *This,
    IN vb_device_state_op_t op,
    IN OUT UINT8 *buf,
    IN UINT32 buf_len);

/**
* Initialize Verified Boot
*
* @param device_info_vb_t  *devinfo
*   Device info data
*
* @return int
*   Success:	handle to be used for all calls to
*   			Secure app. Always greater than zero.
*   Failure:	Error code (negative only).
*/

typedef EFI_STATUS (EFIAPI *QCOM_VB_DEVICE_INIT) (
    IN QCOM_VERIFIEDBOOT_PROTOCOL *This,
    IN device_info_vb_t *devinfo);

/**
* Send Milestone to TZ
* API will send end milestone command to TZ
* @return int
*   Status:
*     0 - Success
*     Negative value indicates failure.
*/
typedef EFI_STATUS (EFIAPI *QCOM_VB_SEND_MILESTONE) (
    IN QCOM_VERIFIEDBOOT_PROTOCOL *This);

/**
* Return if the device is secure or not
* API will set the State flag to indicate if its a secure device
* @return int
*   Status:
*     0 - Success
*     Negative value indicates failure.
*/
typedef EFI_STATUS (EFIAPI *QCOM_VB_IS_DEVICE_SECURE) (
    IN QCOM_VERIFIEDBOOT_PROTOCOL *This,
    OUT BOOLEAN *State);

/**
* Send ROT to Keymaster
* API will construct the ROT payload and provide
* it to Keymaster TA
* @return int
*   Status:
*     0 - Success
*     Negative value indicates failure.
*/
typedef EFI_STATUS (EFIAPI *QCOM_VB_SEND_ROT) (
    IN QCOM_VERIFIEDBOOT_PROTOCOL *This);

/**
* Authenticate Linux Image based on VB
*
* @param UINT8 pname[MAX_PNAME_LENGTH]
*   Pname of image to be authenticated
*
* @param UINT32*img
*   Pointer to the img loaded from "pname" partition
*
* @param uint32_t img_len
*   Size of the img
*
* @param boot_state bootstate
*   Enum of the boot state of the device.
*
* @return int
*   Status:
*     0 - Success
*     Negative value indicates failure.
*/
typedef EFI_STATUS (EFIAPI *QCOM_VB_VERIFY_IMAGE) (
    IN QCOM_VERIFIEDBOOT_PROTOCOL *This,
    IN UINT8 pname[MAX_PNAME_LENGTH],
    IN UINT8 *img,
    IN UINT32 img_len,
    OUT boot_state_t *bootstate);

/**
* Send Delete CMD to Keymaster
*
* @return int
*   Status:
*     0 - Success
*     Negative value indicates failure.
*/
typedef EFI_STATUS (EFIAPI *QCOM_VB_RESET_STATE) (
    IN QCOM_VERIFIEDBOOT_PROTOCOL *This);

/**
* Get boot state
*
* @return boot_state_t
* In case of success returns 0.
*
*
*/
typedef EFI_STATUS (EFIAPI *QCOM_VB_GET_BOOT_STATE) (
    IN QCOM_VERIFIEDBOOT_PROTOCOL *This,
    OUT boot_state_t *bootstate);
/**
*  Get hash of certificate in Yellow boot state
*  @return *buf
*  In case of success return 0.
*
*/
typedef EFI_STATUS (EFIAPI *QCOM_VB_GET_CERT_FINGERPRINT) (
    IN QCOM_VERIFIEDBOOT_PROTOCOL *This,
    OUT UINT8 *buf,
    IN UINTN buf_len,
    OUT UINTN *out_len);

/**
* Check if Keymaster is enabled.
*
* @return int
* In case of success returns 0.
*
*
*/
typedef EFI_STATUS (EFIAPI *QCOM_VB_IS_KEYMASTER_ENABLED ) (
    IN  QCOM_VERIFIEDBOOT_PROTOCOL   *This,
    OUT BOOLEAN                      *KeymasterEnabled);

/*===========================================================================
  PROTOCOL INTERFACE
===========================================================================*/
/** @ingroup
  @par Summary
    Android VERIFIEDBOOT Protocol interface.

  @par Parameters
  @inputprotoparams
*/
struct _QCOM_VERIFIEDBOOT_PROTOCOL {
  UINT64 Revision;
  QCOM_VB_RW_DEVICE_STATE VBRwDeviceState;
  QCOM_VB_DEVICE_INIT VBDeviceInit;
  QCOM_VB_SEND_ROT VBSendRot;
  QCOM_VB_SEND_MILESTONE VBSendMilestone;
  QCOM_VB_VERIFY_IMAGE VBVerifyImage;
  QCOM_VB_RESET_STATE VBDeviceResetState;
  QCOM_VB_IS_DEVICE_SECURE VBIsDeviceSecure;
  QCOM_VB_GET_BOOT_STATE VBGetBootState;
  QCOM_VB_GET_CERT_FINGERPRINT VBGetCertFingerPrint;
  QCOM_VB_IS_KEYMASTER_ENABLED VBIsKeymasterEnabled;
};
#endif /* __EFIVERIFIEDBOOT_H__ */
