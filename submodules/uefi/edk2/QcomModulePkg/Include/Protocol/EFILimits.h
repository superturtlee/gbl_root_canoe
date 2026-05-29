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
#ifndef __EFILIMITS_H__
#define __EFILIMITS_H__

/*===========================================================================
  INCLUDE FILES
===========================================================================*/

/*===========================================================================
  MACRO DECLARATIONS
===========================================================================*/

/** @ingroup efi_limits_constants
  Protocol version.
*/
#define EFI_LIMITS_PROTOCOL_REVISION 0x0000000000010000
/** @} */ /* end_addtogroup efi_limits_constants */

/*  Protocol GUID definition */
/** @ingroup efi_limits_protocol */
#define EFI_LIMITS_PROTOCOL_GUID                                               \
  {                                                                            \
    0x79d6c879, 0x725e, 0x489e,                                                \
    {                                                                          \
      0xa0, 0xa9, 0x27, 0xef, 0xa5, 0xdf, 0xcb, 0x35                           \
    }                                                                          \
  }

/** @cond */
/*===========================================================================
  EXTERNAL VARIABLES
===========================================================================*/
/**
  External reference to the Limits Protocol GUID.
 */
extern EFI_GUID gEfiLimitsProtocolGuid;
/** @endcond */

/*===========================================================================
  TYPE DEFINITIONS
===========================================================================*/
/** @cond */
typedef struct _EFI_LIMITS_PROTOCOL EFI_LIMITS_PROTOCOL;
/** @endcond */

/* Limits Throttle Level */
typedef enum {
  EFI_LIMITS_THROTTLE_NONE = 0,
  EFI_LIMITS_THROTTLE_MIN,
  EFI_LIMITS_THROTTLE_QUARTER,
  EFI_LIMITS_THROTTLE_HALF,
  EFI_LIMITS_THROTTLE_THREE_QUARTER,
  EFI_LIMITS_THROTTLE_MAX,
  EFI_LIMITS_THROTTLE_MAX_DISABLE,
  EFI_LIMITS_THROTTLE_VALUE,
  EFI_LIMITS_THROTTLE_INVALID,
  EFI_LIMITS_THROTTLE_32BITS = 0x7FFFFFFF,
} EFI_LIMITS_THROTTLE_TYPE;

/* Limits Subsystem Identifier */
typedef enum {
  EFI_LIMITS_SUBSYS_APC0 = 0,
  EFI_LIMITS_SUBSYS_APC1,
  EFI_LIMITS_SUBSYS_GPU,
  EFI_LIMITS_SUBSYS_INVALID,
  EFI_LIMITS_SUBSYS_32BITS = 0x7FFFFFFF,
} EFI_LIMITS_SUBSYS_ID;

/*===========================================================================
  FUNCTION DEFINITIONS
===========================================================================*/
/* EFI_LIMITS_SUBSYS_THROTTLE */
/** @ingroup efi_limits_subsys_throttle
  @par Summary
  Input is the Subsys Id. Returns recommended throttle value. This value is an
  indication to HLOS that a particular subsys should be max operating point
  limited by SW during normal operating conditions. Interface may return
  Throttle value of EFI_LIMITS_THROTTLE_VALUE at which point the caller can
  check Value for precise throttling recommendation.

  @param[in]   This       Pointer to the EFI_LIMITS_PROTOCOL instance.
  @param[in]   SubsysId   Pointer to the EFI_LIMITS_SUBSYS_ID which is the
                          SOC subsytem caller would like to recieve a
                          recommendation from Limits driver for.
  @param[out]  Throttle   Pointer to the EFI_LIMITS_THROTTLE_TYPE
                          recommendation.
  @param[out]  Value      Pointer to precise recommendation value. Only valid
                          if Throttle recommendation returned is
                          EFI_LIMITS_THROTTLE_VALUE.

  @return
  EFI_SUCCESS -- Function completed successfully.
  EFI_INVALID_PARAMETER -- Invalid parameter.
 */
typedef EFI_STATUS (EFIAPI *EFI_LIMITS_SUBSYS_THROTTLE) (
    IN EFI_LIMITS_PROTOCOL *This,
    IN EFI_LIMITS_SUBSYS_ID SubsysId,
    OUT EFI_LIMITS_THROTTLE_TYPE *Throttle,
    OUT UINT32 *Value);

/*===========================================================================
  PROTOCOL INTERFACE
===========================================================================*/
/** @ingroup efi_limits_protocol
  @par Summary
  Limits Driver Protocol interface.

  @par Parameters
  @inputprotoparams{limits_proto_params.tex}
*/
struct _EFI_LIMITS_PROTOCOL {
  UINT64 Revision;
  EFI_LIMITS_SUBSYS_THROTTLE SubSysThrottle;
};

#endif /* __EFILIMITS_H__ */
