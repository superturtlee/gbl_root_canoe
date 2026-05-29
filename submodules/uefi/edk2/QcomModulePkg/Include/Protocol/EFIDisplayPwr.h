/* Copyright (c) 2020, The Linux Foundation. All rights reserved.
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

#ifndef __EFIDISPLAYPWR_H__
#define __EFIDISPLAYPWR_H__

/*===========================================================================
  INCLUDE FILES
===========================================================================*/

/*===========================================================================
  MACRO DECLARATIONS
===========================================================================*/
/** @ingroup efi_displayPwr_constants
  Protocol version.
*/
#define EFI_DISPLAY_POWER_PROTOCOL_REVISION    0x0000000000010000

/** @} */ /* end_addtogroup efi_displayPwr_constants */

/*  Protocol GUID definition */
/** @ingroup efi_displayPwr_protocol */
#define EFI_DISPLAY_POWER_PROTOCOL_GUID  \
    {0xf352021d, 0x9593, 0x4432, {0xbf, 0x4, 0x67, 0xb9, 0xf3, 0xb7, 0x60, 0x8}}

/** @cond */
/*===========================================================================
  EXTERNAL VARIABLES
===========================================================================*/
/**
  External reference to the Display Power Protocol GUID.
*/
extern EFI_GUID gEfiDisplayPowerStateProtocolGuid;

/*===========================================================================
  TYPE DEFINITIONS
===========================================================================*/
/**
  Protocol declaration.
*/
typedef struct _EFI_DISPLAY_POWER_PROTOCOL  EFI_DISPLAY_POWER_PROTOCOL;
/** @endcond */

/** @ingroup efi_displayPwr_data_types
  Display state.
*/
typedef enum /** @cond */_EFI_DISPLAY_POWER_STATE/** @endcond */ {
  EfiDisplayPowerStateUnknown = 0,
  /**< Indicates the power state has not yet been initialized. */
  EfiDisplayPowerStateOff,
  /**< Power off both the backlight and display. */
  EfiDisplayPowerStateOn,
  /**< Power on both the backlight and display. */
} EFI_DISPLAY_POWER_STATE;
/** @} */ /* end_addtogroup efi_displayPwr_data_types */

/*===========================================================================
  FUNCTION DEFINITIONS
===========================================================================*/
/* EFI_DISPLAY_POWER_SETDISPLAYPOWERSTATE */
/** @ingroup efi_displayPwr_set_display_power_state
  @par Summary
  Modifies the power state of the display and backlight.

  @param[in]  This        Pointer to the EFI_DISPLAY_POWER_PROTOCOL instance.
  @param[in]  PowerState  Display state; see #EFI_DISPLAY_POWER_STATE for
                          details.

  @return
  EFI_SUCCESS           -- Function returned successfully.
  EFI_INVALID_PARAMETER -- Parameter passed is incorrect.
  EFI_DEVICE_ERROR      -- Physical device reported an error.
*/
typedef
EFI_STATUS
(EFIAPI *EFI_DISPLAY_POWER_SETDISPLAYPOWERSTATE)(
  IN EFI_DISPLAY_POWER_PROTOCOL             *This,
  IN EFI_DISPLAY_POWER_STATE                PowerState
  );

/* EFI_DISPLAY_POWER_GETDISPLAYPOWERSTATE */
/** @ingroup efi_displayPwr_get_display_power_state
  @par Summary
  Retrieves the current power state of the display and backlight.

  @param[in]  This        Pointer to the EFI_DISPLAY_POWER_PROTOCOL instance.
  @param[out] PowerState  Display state; see #EFI_DISPLAY_POWER_STATE for
                          details.

  @return
  EFI_SUCCESS           -- Function returned successfully.
  EFI_INVALID_PARAMETER -- Parameter passed is incorrect.
  EFI_DEVICE_ERROR      -- Physical device reported an error.
*/
typedef
EFI_STATUS
(EFIAPI * EFI_DISPLAY_POWER_GETDISPLAYPOWERSTATE)(
  IN EFI_DISPLAY_POWER_PROTOCOL             *This,
  OUT EFI_DISPLAY_POWER_STATE               *PowerState
  );

/*===========================================================================
  PROTOCOL INTERFACE
===========================================================================*/
/** @ingroup efi_displayPwr_protocol
  @par Summary
  This protocol allows modification and query of the display power state.

  @par Parameters
  @inputprotoparams{display_power_proto_params.tex}
*/

typedef struct _EFI_DISPLAY_POWER_PROTOCOL {
  UINT32                                        Revision;
  EFI_DISPLAY_POWER_SETDISPLAYPOWERSTATE        SetDisplayPowerState;
  EFI_DISPLAY_POWER_GETDISPLAYPOWERSTATE        GetDisplayPowerState;
} EFI_DISPLAY_POWER_PROTOCOL;

#endif  /* __EFIDISPLAYPWR_H__ */
