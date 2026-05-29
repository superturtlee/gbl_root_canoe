/* Copyright (c) 2015-2020, The Linux Foundation. All rights reserved.
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

#ifndef __EFIPMICVERSION_H__
#define __EFIPMICVERSION_H__

/*===========================================================================
  MACRO DECLARATIONS
===========================================================================*/
/** @ingroup efi_pmicVersion_constants
  Protocol version.
*/
#define PMIC_VERSION_REVISION 0x0000000000010004
#define PMIC_EXT_VERSION 0x0000000000010003
/** @} */ /* end_addtogroup efi_pmicVersion_constants */

/* Protocol GUID definition */
/** @ingroup efi_pmicVersion_protocol */
#define EFI_PMIC_VERSION_PROTOCOL_GUID                                         \
  {                                                                            \
    0x4684800a, 0x2755, 0x4edc,                                                \
    {                                                                          \
      0xb4, 0x43, 0x7f, 0x8c, 0xeb, 0x32, 0x39, 0xd3                           \
    }                                                                          \
  }

/** @cond */
/**
  External reference to the PMIC Version Protocol GUID.
*/
extern EFI_GUID gQcomPmicVersionProtocolGuid;

/*===========================================================================
  TYPE DEFINITIONS
===========================================================================*/
/**
  PMIC VERSION UEFI typedefs
*/
typedef struct _EFI_QCOM_PMIC_VERSION_PROTOCOL EFI_QCOM_PMIC_VERSION_PROTOCOL;
/** @endcond */

/** @addtogroup efi_pmicVersion_data_types
@{ */
/* Specifies which PMIC we are using */
/**
  PMIC model; version resource ID.
*/
typedef enum
{
   EFI_PMIC_IS_UNKNOWN   = 0x0,                /**< UNKNOWN  */
   EFI_PMIC_IS_PM8941    = 0x1,                /**< PM8941   */
   EFI_PMIC_IS_PM8841    = 0x2,                /**< PM8841   */
   EFI_PMIC_IS_PM8019    = 0x3,                /**< PM8019   */
   EFI_PMIC_IS_PM8026    = 0x4,                /**< PM8026   */
   EFI_PMIC_IS_PM8110    = 0x5,                /**< PM8110   */
   EFI_PMIC_IS_PMA8084   = 0x6,                /**< PMA8084  */
   EFI_PMIC_IS_PMI8962   = 0x7,                /**< PMI8962  */
   EFI_PMIC_IS_PMD9635   = 0x8,                /**< PMD9635  */
   EFI_PMIC_IS_PM8994    = 0x9,                /**< PM8994   */
   EFI_PMIC_IS_PMI8994   = 0xA,                /**< PMI8994  */
   EFI_PMIC_IS_PM8916    = 0xB,                /**< PM8916   */
   EFI_PMIC_IS_PM8004    = 0xC,                /**< PM8004   */
   EFI_PMIC_IS_PMF2432   = EFI_PMIC_IS_PMA8084,/**< PMF2432  */
   EFI_PMIC_IS_PM2433    = 0xE,                /**< PM2433   */
   EFI_PMIC_IS_PM8950    = 0x10,               /**< PM8950   */
   EFI_PMIC_IS_PMK8001   = 0x12,               /**< PMK8001  */
   EFI_PMIC_IS_PMI8996   = 0x13,               /**< PMI8996  */
   EFI_PMIC_IS_PM8998    = 0x14,               /**< PM8998   */
   EFI_PMIC_IS_PMI8998   = 0x15,               /**< PMI8998  */
   EFI_PMIC_IS_PM8005    = 0x18,               /**< PM8005   */
   EFI_PMIC_IS_PM660L    = 0x1A,               /**< PM660L   */
   EFI_PMIC_IS_PM660     = 0x1B,               /**< PM660    */
   EFI_PMIC_IS_SMB1355   = 0x1C,               /**< SMB1355  */
   EFI_PMIC_IS_PM855     = 0x1E,               /**< PM855    */
   EFI_PMIC_IS_PM8150    = EFI_PMIC_IS_PM855,  /**< PM8150   */
   EFI_PMIC_IS_PM8250    = EFI_PMIC_IS_PM855,  /**< PM8250   */
   EFI_PMIC_IS_PM855L    = 0x1F,               /**< PM855L   */
   EFI_PMIC_IS_PM8150L   = EFI_PMIC_IS_PM855L, /**< PM8150L  */
   EFI_PMIC_IS_PM855B    = 0x20,               /**< PM855B   */
   EFI_PMIC_IS_PM8150B   = EFI_PMIC_IS_PM855B, /**< PM8150B  */
   EFI_PMIC_IS_PMK8002   = 0x21,               /**< PMK8002  */
   EFI_PMIC_IS_PMX24     = 0x22,               /**< PMX24    */
   EFI_PMIC_IS_SMB1390   = 0x23,               /**< SMB1390  */
   EFI_PMIC_IS_PM8009    = 0x24,               /**< PM8009   */
   EFI_PMIC_IS_PMI632    = 0x25,               /**< PMI632   */
   EFI_PMIC_IS_PM855P    = 0x26,               /**< PM855P   */
   EFI_PMIC_IS_PM855A    = 0x27,               /**< PM855A   */
   EFI_PMIC_IS_PM8150A   = EFI_PMIC_IS_PM855A, /**< PM8150A  */
   EFI_PMIC_IS_SMB2351   = 0x29,               /**< SMB2351  */
   EFI_PMIC_IS_PMX55     = 0x2A,               /**< PMX55   */
   EFI_PMIC_IS_PMR8255   = 0x2B,               /**< PMR8255 */
   EFI_PMIC_IS_PM8008    = 0x2C,               /**< PM8008  */
   EFI_PMIC_IS_PM6125    = 0x2D,               /**< PM6125  */
   EFI_PMIC_IS_PM7250B   = 0x2E,               /**< PM7250B */
   EFI_PMIC_IS_PMK8350   = 0x2F,               /**< PMK8350 */
   EFI_PMIC_IS_PM8350    = 0x30,               /**< PM8350  */
   EFI_PMIC_IS_PM8350C   = 0x31,               /**< PM8350C */
   EFI_PMIC_IS_PM8350B   = 0x32,               /**< PM8350B */
   EFI_PMIC_IS_PMR735A   = 0x33,               /**< PMR735A */
   EFI_PMIC_IS_PMR735B   = 0x34,               /**< PMR735B */
   EFI_PMIC_IS_SMB1398   = 0x35,               /**< SMB1398 */
   EFI_PMIC_IS_PM6350    = 0x36,               /**< PM6350  */
   EFI_PMIC_IS_PM2XX     = 0x37,               /**< PM2XX   */
   EFI_PMIC_IS_PMX60     = 0x38,               /**< PMX60   */
   EFI_PMIC_IS_SMB1394   = 0x39,               /**< SMB1394 */
   EFI_PMIC_IS_PM8450    = 0x3A,               /**< PM8450  */
   EFI_PMIC_IS_PMX65     = 0x3B,               /**< PMX65   */
   EFI_PMIC_IS_INVALID   = 0x7FFFFFFF,         /**< INVALID  */
} EFI_PM_MODEL_TYPE;

/**
  PMIC device information.
*/
typedef struct {
  EFI_PM_MODEL_TYPE PmicModel;
  /**< Model type; see #EFI_PM_MODEL_TYPE for details. */
  UINT32 PmicAllLayerRevision;
  /**< All layer revision number. */
  UINT32 PmicMetalRevision;
  /**< Metal revision number. */
  UINT32 SlaveCount;
  /**< Slave Count on PmicModel */
} EFI_PM_DEVICE_INFO_TYPE;
/** @} */ /* end_addtogroup efi_pmicVersion_data_types */

/**
  Extended PMIC device information with PMIC Variant info.
*/
typedef struct {
  EFI_PM_MODEL_TYPE PmicModel;
  /**< Model type; see #EFI_PM_MODEL_TYPE for details. */
  UINT32 PmicAllLayerRevision;
  /**< All layer revision number. */
  UINT32 PmicMetalRevision;
  /**< Metal revision number. */
  UINT32 PmicVariantRevision;
  /**< Variant revision number. */
} EFI_PM_DEVICE_INFO_EXT_TYPE;
/** @} */ /* end_addtogroup efi_pmicVersion_data_types */

/*===========================================================================
  FUNCTION DEFINITIONS
===========================================================================*/

/* EFI_PM_GET_PMIC_INFO */
/** @ingroup efi_pmicVersion_get_pmic_info
  @par Summary
  Retrieves information about the PMIC device for a specific device index.

  @param[in]  PmicDeviceIndex  Primary: 0. Secondary: 1.
  @param[out] PmicDeviceInfo   Variable with PMIC device information to return
                               to the caller; see #EFI_PM_DEVICE_INFO_TYPE for
                               details.

  @return
  EFI_SUCCESS        -- Function completed successfully. \n
  EFI_PROTOCOL_ERROR -- Error occurred during the operation.
*/
typedef EFI_STATUS (EFIAPI *EFI_PM_GET_PMIC_INFO) (
    IN UINT32 PmicDeviceIndex,
    OUT EFI_PM_DEVICE_INFO_TYPE *PmicDeviceInfo);

/* EFI_PM_GET_PRIMARY_PMIC_INDEX */
/** @ingroup efi_pmicversion_get_pmic_info
  @par Summary
 *Retrieves information about primary PMIC index
 *
 * @param[out] PrimaryPmicIndex Primary pmic index 0, 1, 2,
 *       ..etc
 *

  @return
  EFI_SUCCESS        -- Function completed successfully. \n
  EFI_PROTOCOL_ERROR -- Error occurred during the operation.
*/
typedef
EFI_STATUS (EFIAPI *EFI_PM_GET_PRIMARY_PMIC_INDEX)(
  OUT  UINT32    *PrimaryPmicIndex
);

/* EFI_PM_GET_PMIC_INFO_EXT */
/** @ingroup efi_pmicVersion_get_pmic_info
  @par Summary
  Retrieves information about the PMIC device for a specific device index.

  @param[in]  PmicDeviceIndex  Primary: 0. Secondary: 1.
  @param[out] PmicDeviceInfo   Variable with PMIC device information to return
                               to the caller; see #EFI_PM_DEVICE_INFO_EXT_TYPE
  for
                               details.

  @return
  EFI_SUCCESS        -- Function completed successfully. \n
  EFI_PROTOCOL_ERROR -- Error occurred during the operation.
*/
typedef EFI_STATUS (EFIAPI *EFI_PM_GET_PMIC_INFO_EXT) (
    IN UINT32 PmicDeviceIndex,
    OUT EFI_PM_DEVICE_INFO_EXT_TYPE *PmicDeviceInfo);

/*===========================================================================
  PROTOCOL INTERFACE
===========================================================================*/
/** @ingroup efi_pmicVersion_protocol
  @par Summary
  Qualcomm PMIC Version Protocol interface.

  @par Parameters
  @inputprotoparams{pmic_version_proto_params.tex}
*/
struct _EFI_QCOM_PMIC_VERSION_PROTOCOL {
  UINT64 Revision;
  EFI_PM_GET_PMIC_INFO GetPmicInfo;
  EFI_PM_GET_PRIMARY_PMIC_INDEX GetPrimaryPmicIndex;
  EFI_PM_GET_PMIC_INFO_EXT GetPmicInfoExt;
};

#endif /* __EFIPMICVERSION_H__ */
