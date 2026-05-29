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
 *
 *@file  EFIChargerEx.h
 *@brief Charger Extended Protocol for UEFI open source.
 */
#ifndef __EFI_PMIC_PON_H__
#define __EFI_PMIC_PON_H__

/** @file  EFIPmicPon.h
    @brief PMIC-PON MODULE RELATED DECLARATION

     This file contains functions and variable declarations to support
     the PMIC PON module.
 */

/** @ingroup efi_pmicPON_constants
  Protocol version.
*/
#define PMIC_PON_REVISION 0x0000000000010001
#define EFI_QCOM_PMIC_PON_PROTOCOL_GUID                                        \
  {                                                                            \
    0x97044b58, 0xfea4, 0x4ad0,                                                \
    {                                                                          \
      0x9d, 0xb, 0xe4, 0x17, 0xd6, 0xf, 0x11, 0xa1                             \
    }                                                                          \
  }
/* PMIC PON typedefs */
/** @cond */
typedef struct _EFI_QCOM_PMIC_PON_PROTOCOL EFI_QCOM_PMIC_PON_PROTOCOL;
/** @endcond */

/* PMIC PON Procotol GUID */
/** @ingroup efi_pmicPON_protocol */
extern EFI_GUID gQcomPmicPonProtocolGuid;

/** @} */
/* end_addtogroup efi_pmicPon_data_types */

typedef enum {
  EFI_PM_PON_PERPH_SPARE,
  EFI_PM_PON_DVDD_SPARE,
  EFI_PM_PON_XVDD_SPARE,
  EFI_PM_PON_SOFT_SPARE,
  EFI_PM_PON_SPARE_INVALID
} EFI_PM_PON_SPARE_REG_TYPE;

/*! \struct pm_pon_reason_type
   \brief Power On reason type.
 */
typedef struct {
  UINT32 HARD_RESET : 1; /* Hard reset event trigger */
  UINT32 SMPL : 1;       /* SMPL trigger */
  UINT32 RTC : 1;        /* RTC trigger */
  UINT32 DC_CHG : 1;     /* DC Charger trigger */
  UINT32 USB_CHG : 1;    /* USB Charger trigger */
  UINT32 PON1 : 1;       /* PON1 trigger */
  UINT32 CBLPWR : 1;     /* CBL_PWR1_N trigger */
  UINT32 KPDPWR : 1;     /* KPDPWR_N trigger */
} EFI_PM_PON_REASON_TYPE;

/*===========================================================================
                          FUNCTION DEFINITIONS
===========================================================================*/

/* EFI_PM_PON_GET_SPARE_REG_DATA */
/** @ingroup
  @par Summary
  Reads the PON spare register

  @param[in]  PmicDeviceIndex  Primary: 0.
  @param[in]  SpareReg         Spare register type See
  #EFI_PM_PON_SPARE_REG_TYPE for details.
  @param[out] DataPtr          Data Read

  @return
  EFI_SUCCESS        -- Function completed successfully. \n
  EFI_PROTOCOL_ERROR -- Error occurred during the operation.
*/
typedef EFI_STATUS (EFIAPI *EFI_PM_PON_GET_SPARE_REG) (
    IN UINT32 PmicDeviceIndex,
    IN EFI_PM_PON_SPARE_REG_TYPE SpareReg,
    OUT UINT8 *DataPtr);

/* EFI_PM_PON_SET_SPARE_REG*/
/** @ingroup
  @par Summary
  Sets the PON spare registers

  @param[in]  PmicDeviceIndex  Primary: 0.
  @param[in]  SpareReg         Spare register type See
  #EFI_PM_PON_SPARE_REG_TYPE for details.
  @param[in]  SetValue         Data to write
  @param[in]  Mask             Mask bit

  @return
  EFI_SUCCESS        -- Function completed successfully. \n
  EFI_PROTOCOL_ERROR -- Error occurred during the operation.
*/
typedef EFI_STATUS (EFIAPI *EFI_PM_PON_SET_SPARE_REG) (
    IN UINT32 PmicDeviceIndex,
    IN EFI_PM_PON_SPARE_REG_TYPE SpareReg,
    IN UINT8 SetValue,
    IN UINT8 Mask);

/**
 * @brief Returns reason for Power On
 *
 * @param[in] PmicDeviceIndex. Primary PMIC: 0 Secondary PMIC: 1
 * @param[in] Reason:
 *                PON reason type. Refer struct
 *                EFI_PM_PON_GET_PON_REASON.
 *
 *
 * @return  pm_err_flag_type
 *          PM_ERR_FLAG__INVALID_POINTER = Null pointer passed
 *          in.
 *          PM_ERR_FLAG__PAR1_OUT_OF_RANGE = Device Index out of
 *          range.
 *          PM_ERR_FLAG__SUCCESS = SUCCESS.
 */
typedef EFI_STATUS (EFIAPI *EFI_PM_PON_GET_PON_REASON) (
    IN UINT32 PmicDeviceIndex,
    OUT EFI_PM_PON_REASON_TYPE *Reason);

/**
 * @brief Returns Warm reset occured satus
 *
 * @param[in] PmicDeviceIndex. Primary PMIC: 0 Secondary PMIC: 1
 * @param[in] WarmReset:
 *                If boot up was warm reset then then TRUE
 *                otherwise FALSE
 *
 *
 * @return  pm_err_flag_type
 *          EFI_DEVICE_ERROR = FAIL
 *          EFI_SUCCESS = SUCCESS.
 */
typedef EFI_STATUS (EFIAPI *EFI_PM_PON_WARM_RESET_STATUS) (
    IN UINT32 PmicDeviceIndex,
    OUT BOOLEAN *WarmReset);

/*===========================================================================
  PROTOCOL INTERFACE
===========================================================================*/
/** @ingroup efi_pmicPon_protocol
  @par Summary
  Qualcomm Technologies inc. PMIC Pon Protocol interface.

  @par Parameters
  @inputprotoparams{pmic_pon_proto_params.tex}
*/
struct _EFI_QCOM_PMIC_PON_PROTOCOL {
  UINT64 Revision;
  EFI_PM_PON_GET_SPARE_REG GetSpareReg;
  EFI_PM_PON_SET_SPARE_REG SetSpareReg;
  EFI_PM_PON_GET_PON_REASON GetPonReason;
  EFI_PM_PON_WARM_RESET_STATUS WarmResetStatus;
};

#endif /* __EFI_PMIC_PON_H__ */
