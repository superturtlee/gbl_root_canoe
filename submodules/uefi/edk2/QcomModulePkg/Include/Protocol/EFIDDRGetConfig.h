/* Copyright (c) 2018-2021, The Linux Foundation. All rights reserved.
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

#ifndef __EFIDDRGETINFO_H__
#define __EFIDDRGETINFO_H__

#include "DDRDetails.h"

/** @cond */
typedef struct _EFI_DDRGETINFO_PROTOCOL EFI_DDRGETINFO_PROTOCOL;
/** @endcond */

/** @addtogroup efi_ddrgetinfo_constants
@{ */
/**
  Protocol version.
*/
#define EFI_DDRGETINFO_PROTOCOL_REVISION_5 0x0000000000050000
#define EFI_DDRGETINFO_PROTOCOL_REVISION 0x0000000000070000

/*Both the protocl version and the structure version should be same */
#if (EFI_DDRGETINFO_PROTOCOL_REVISION != DDR_DETAILS_STRUCT_VERSION)
ASSERT (FALSE);
#endif

/** @} */ /* end_addtogroup efi_ddrgetinfo_constants */

/*  Protocol GUID definition */
/** @ingroup efi_ddrgetinfo_protocol */
#define EFI_DDRGETINFO_PROTOCOL_GUID                                           \
  {                                                                            \
    0x1a7c0eb8, 0x5646, 0x45f7,                                                \
    {                                                                          \
      0xab, 0x20, 0xea, 0xe5, 0xda, 0x46, 0x40, 0xa2                           \
    }                                                                          \
  }

/** @cond */
/**
  External reference to the DDRGetInfo Protocol GUID defined
  in the .dec file.
*/
extern EFI_GUID gEfiDDRGetInfoProtocolGuid;
/** @endcond */

/** @} */ /* end_addtogroup efi_ddrgetinfo_data_types */

/*==============================================================================

                             API IMPLEMENTATION

==============================================================================*/

/* ============================================================================
**  Function : EFI_DDRGETINFO_GETDDRDETAILS
** ============================================================================
*/
/** @ingroup efi_ddrgetinfo_GETDDRDETAILS
  @par Summary
  Gets the DDR Details

  @param[in]   This            Pointer to the EFI_DDRGETINFO_PROTOCOL instance.
  @param[out]  DetailsEntry    Pointer to DDR Details

  @return
  EFI_SUCCESS        -- Function completed successfully. \n
  EFI_PROTOCOL_ERROR -- Error occurred during the operation.
*/
typedef EFI_STATUS (EFIAPI *EFI_DDRGETINFO_GETDDRDETAILS) (
    IN EFI_DDRGETINFO_PROTOCOL *This,
    OUT struct ddr_details_entry_info *DetailsEntry);

/* ============================================================================
**  Function : EFI_DDRGetInfo_GetDDRFreqTable
** ============================================================================
*/
/** @ingroup EFI_DDRGetInfo_GetDDRFreqTable
  @par Summary
  Gets the DDR Clock plan table

  @param[in]   This            Pointer to the EFI_DDRGETINFO_PROTOCOL instance.
  @param[out]  ClkPlanTable    Pointer to DDR clock plan Table

  @return
  EFI_SUCCESS        -- Function completed successfully. \n
  EFI_PROTOCOL_ERROR -- Error occurred during the operation.
*/
typedef
EFI_STATUS
(EFIAPI *EFI_DDRGETINFO_GETDDRFREQTABLE)(
   IN EFI_DDRGETINFO_PROTOCOL *This,
   OUT struct ddr_freq_plan_entry_info   *clk_plan_tbl
   );

/* ============================================================================
**  Function : EFI_DDRGetInfo_GetDDRFreq
** ============================================================================
*/
/** @ingroup EFI_DDRGetInfo_GetDDRFreq
  @par Summary
  Gets the Current DDR Freq

  @param[in]   This            Pointer to the EFI_DDRGETINFO_PROTOCOL instance.
  @param[out]  ddr_freq        Pointer to Current DDR clock frequency

  @return
  EFI_SUCCESS        -- Function completed successfully. \n
  EFI_PROTOCOL_ERROR -- Error occurred during the operation.
*/
typedef
EFI_STATUS
(EFIAPI *EFI_DDRGETINFO_GETDDRFREQ)(
   IN EFI_DDRGETINFO_PROTOCOL *This,
   OUT UINT32                 *ddr_freq
   );

/* ============================================================================
**  Function : EFI_DDRGetInfo_GetDDRMaxEnabledFreq
** ============================================================================
*/
/** @ingroup EFI_DDRGetInfo_GetDDRMaxEnabledFreq
  @par Summary
  Gets the DDR Max Enabled Freq

  @param[in]   This             Pointer to the EFI_DDRGETINFO_PROTOCOL instance.
  @param[out]  ddr_freq         Pointer to DDR Max Enabled frequency

  @return
  EFI_SUCCESS        -- Function completed successfully. \n
  EFI_PROTOCOL_ERROR -- Error occurred during the operation.
*/
typedef
EFI_STATUS
(EFIAPI *EFI_DDRGETINFO_GETDDRMAXENABLEDFREQ)(
   IN EFI_DDRGETINFO_PROTOCOL *This,
   OUT UINT32                 *ddr_freq
   );


/* ============================================================================
**  Function : EFI_DDRGetInfo_SetDDRFreq
** ============================================================================
*/
/** @ingroup EFI_DDRGETINFO_SETDDRFREQ
  @par Summary
  Gets the DDR Details

  @param[in]   This            Pointer to the EFI_DDRGETINFO_PROTOCOL instance.
  @param[in]   ddr_freq        DDR freq to be set in the system.

  @return
  EFI_SUCCESS        -- Function completed successfully. \n
  EFI_PROTOCOL_ERROR -- Error occurred during the operation.
*/
typedef
EFI_STATUS
(EFIAPI *EFI_DDRGETINFO_SETDDRFREQ)(
   IN EFI_DDRGETINFO_PROTOCOL *This,
   IN UINT32                   ddr_freq
   );

/* ============================================================================
**  Function : EFI_DDRGETINFO_SETMAXNOMINALDDRFREQ
** ============================================================================
*/
/** @ingroup EFI_DDRGETINFO_SETMAXNOMINALDDRFREQ
  @par Summary
  Gets the DDR Details

  @param[in]   This            Pointer to the EFI_DDRGETINFO_PROTOCOL instance.

  @return
  EFI_SUCCESS        -- Function completed successfully. \n
  EFI_PROTOCOL_ERROR -- Error occurred during the operation.
*/
typedef
EFI_STATUS
(EFIAPI *EFI_DDRGETINFO_SETMAXNOMINALDDRFREQ)(
   IN EFI_DDRGETINFO_PROTOCOL *This
   );

/* ============================================================================
**  Function : EFI_DDRGETINFO_SETLOWESTDDRFREQ
** ============================================================================
*/
/** @ingroup EFI_DDRGETINFO_SETLOWESTDDRFREQ
  @par Summary
  Gets the DDR Details

  @param[in]   This            Pointer to the EFI_DDRGETINFO_PROTOCOL instance.

  @return
  EFI_SUCCESS        -- Function completed successfully. \n
  EFI_PROTOCOL_ERROR -- Error occurred during the operation.
*/
typedef
EFI_STATUS
(EFIAPI *EFI_DDRGETINFO_SETLOWESTDDRFREQ)(
   IN EFI_DDRGETINFO_PROTOCOL *This
   );

/* ============================================================================
**  Function : EFI_DDRGETINFO_GETDDRREGIONSDATA
** ============================================================================
*/
/** @ingroup EFI_DDRGETINFO_GETDDRREGIONSDATA
  @par Summary
  Gets the DDR Details

  @param[in]   This            Pointer to the EFI_DDRGETINFO_PROTOCOL instance.
  @param[out]  DetailsEntry    Pointer to DDR Regions Data

  @return
  EFI_SUCCESS        -- Function completed successfully. \n
  EFI_PROTOCOL_ERROR -- Error occurred during the operation.
*/
typedef
EFI_STATUS
(EFIAPI *EFI_DDRGETINFO_GETDDRMAPPEDREGIONS)(
   IN EFI_DDRGETINFO_PROTOCOL *This,
   OUT struct ddr_regions_data_info   *GetDDRMappedRegions
   );

/* ============================================================================
**  Function : EFI_DDRGetInfo_GetSHUBFreqTable
** ============================================================================
*/
/** @ingroup EFI_DDRGetInfo_GetSHUBFreqTable
  @par Summary
  Gets the SHUB Clock plan table

  @param[in]   This            Pointer to the EFI_DDRGETINFO_PROTOCOL instance.
  @param[out]  ClkPlanTable    Pointer to SHUB clock plan Table

  @return
  EFI_SUCCESS        -- Function completed successfully. \n
  EFI_PROTOCOL_ERROR -- Error occurred during the operation.
*/
typedef
EFI_STATUS
(EFIAPI *EFI_DDRGETINFO_GETSHUBFREQTABLE)(
   IN EFI_DDRGETINFO_PROTOCOL             *This,
   OUT struct shub_freq_plan_entry_info   *clk_plan_tbl
   );

/* ============================================================================
**  Function : EFI_DDRGetInfo_SetSHUBFreq
** ============================================================================
*/
/** @ingroup EFI_DDRGetInfo_SetSHUBFreq
  @par Summary
  Sets the SHUB clock frequency

  @param[in]   This         Pointer to the EFI_DDRGETINFO_PROTOCOL instance.
  @param[out]  shub_freq    SHUB freq to be set in the system.

  @return
  EFI_SUCCESS        -- Function completed successfully. \n
  EFI_PROTOCOL_ERROR -- Error occurred during the operation.
*/
typedef
EFI_STATUS
(EFIAPI *EFI_DDRGETINFO_SETSHUBFREQ)(
   IN EFI_DDRGETINFO_PROTOCOL    *This,
   IN UINT32                     shub_freq
   );

/* ============================================================================
**  Function : EFI_DDRGetInfo_GetSHUBFreq
** ============================================================================
*/
/** @ingroup EFI_DDRGetInfo_GetSHUBFreq
  @par Summary
  Gets the Current SHUB Freq

  @param[in]   This             Pointer to the EFI_DDRGETINFO_PROTOCOL instance.
  @param[out]  shub_freq        Pointer to Current SHUB clock frequency

  @return
  EFI_SUCCESS        -- Function completed successfully. \n
  EFI_PROTOCOL_ERROR -- Error occurred during the operation.
*/
typedef
EFI_STATUS
(EFIAPI *EFI_DDRGETINFO_GETSHUBFREQ)(
   IN EFI_DDRGETINFO_PROTOCOL   *This,
   OUT UINT32                   *shub_freq
   );

/* ============================================================================
**  Function : EFI_DDRGETINFO_GETDDRMISCINFO_SMEM_DATA
** ============================================================================
*/
/** @ingroup EFI_DDRGETINFO_GETDDRMISCINFO_SMEM_DATA
  @par Summary
  Gets the loader to uefi smem data

  @param[in]   This             Pointer to the EFI_DDRGETINFO_PROTOCOL instance.
  @param[out]  ddr_misc_data    Pointer to loader to uefi smem data

  @return
  EFI_SUCCESS        -- Function completed successfully. \n
  EFI_PROTOCOL_ERROR -- Error occurred during the operation.
*/
typedef
EFI_STATUS
(EFIAPI *EFI_DDRGETINFO_GETDDRMISCINFO_SMEM_DATA)(
   IN EFI_DDRGETINFO_PROTOCOL   *This,
   OUT ddr_misc_info  *ddr_misc_data
   );

/* ============================================================================
**  Function : EFI_DDRGetInfo_GetDDRSCTConfig
** ============================================================================
*/
/** @ingroup EFI_DDRGetInfo_GetDDRSCTConfig
  @par Summary
  Gets the DDR SCT Config Information

  @param[in]   This            Pointer to the EFI_DDRGETINFO_PROTOCOL instance.
  @param[out]  sct_config      Pointer to the SCT Configuration Information.

  @return
  EFI_SUCCESS        -- Function completed successfully. \n
  EFI_PROTOCOL_ERROR -- Error occurred during the operation.
*/
typedef
EFI_STATUS
(EFIAPI *EFI_DDRGETINFO_GETDDRSCTCONFIG)(
   IN EFI_DDRGETINFO_PROTOCOL *This,
   OUT UINT8                  *sct_config
   );

/*===========================================================================
  PROTOCOL INTERFACE
===========================================================================*/
/** @ingroup efi_ddrgetinfo_protocol
  @par Summary
  DDR Get Info Protocol interface.

  @par Parameters
*/
struct _EFI_DDRGETINFO_PROTOCOL {
   UINT64                                Revision;
   EFI_DDRGETINFO_GETDDRDETAILS          GetDDRDetails;
   EFI_DDRGETINFO_GETDDRFREQTABLE        GetDDRFreqTable;
   EFI_DDRGETINFO_GETDDRFREQ             GetDDRFreq;
   EFI_DDRGETINFO_SETDDRFREQ             SetDDRFreq;
   EFI_DDRGETINFO_GETDDRMAXENABLEDFREQ   GetDDRMaxEnabledFreq;
   EFI_DDRGETINFO_SETMAXNOMINALDDRFREQ   SetMaxNominalDDRFreq;
   EFI_DDRGETINFO_SETLOWESTDDRFREQ       SetLowestDDRFreq;
   EFI_DDRGETINFO_GETDDRMAPPEDREGIONS    GetDDRMappedRegions;
   EFI_DDRGETINFO_GETSHUBFREQTABLE       GetSHUBFreqTable;
   EFI_DDRGETINFO_SETSHUBFREQ            SetSHUBFreq;
   EFI_DDRGETINFO_GETSHUBFREQ            GetSHUBFreq;
   EFI_DDRGETINFO_GETDDRMISCINFO_SMEM_DATA  GetDDRmiscinfoSmemData;
   EFI_DDRGETINFO_GETDDRSCTCONFIG        GetDDRSCTConfig;
};

#endif /* __EFIDDRGETINFO_H__ */
