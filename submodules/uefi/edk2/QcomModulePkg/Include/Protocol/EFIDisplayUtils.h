/* Copyright (c) 2016-2019, The Linux Foundation. All rights reserved.
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

#ifndef __EFIDISPLAYUTILS_H__
#define __EFIDISPLAYUTILS_H__

/*===========================================================================
  INCLUDE FILES
===========================================================================*/

/*===========================================================================
  MACRO DECLARATIONS
===========================================================================*/
/** @addtogroup efi_displayUtils_constants
@{ */
/**
  Protocol version.
*/
#define DISPLAY_UTILS_REVISION 0x0000000000020000

/** @} */ /* end_addtogroup efi_displayUtils_constants */

/*  Protocol GUID definition */
/** @ingroup efi_displayUtils_protocol */

// {c0dd69ac-76ba-11e6-ab24-1fc7f5575f19}
#define EFI_DISPLAYUTILS_PROTOCOL_GUID \
   { 0xc0dd69ac, 0x76ba, 0x11e6, {0xab, 0x24, 0x1f, 0xc7, 0xf5, 0x57, 0x5f, \
    0x19 }}


/** @cond */
/*===========================================================================
  EXTERNAL VARIABLES
===========================================================================*/
/**
  External reference to the Display Utils Protocol GUID.
*/
extern EFI_GUID gQcomDisplayUtilsProtocolGuid;


/*===========================================================================
  TYPE DEFINITIONS
===========================================================================*/
/**
  Protocol declaration.
*/
typedef struct _EFI_QCOM_DISPLAY_UTILS_PROTOCOL EfiQcomDisplayUtilsProtocol;
/** @endcond */

/** @addtogroup efi_displayUtils_data_types
@{ */
/**
  Display Utils property types
*/
typedef enum
{
  EFI_DISPLAY_UTILS_DEVICE_TREE_ADDR = 0x0,
  /**< Device tree base address: (Type: VOID*) */
  EFI_DISPLAY_UTILS_PANEL_OVERRIDE,
  /**< Panel override string (Type: CHAR8*) */
  EFI_DISPLAY_UTILS_SUPPORTED_PANELS,
  /**< Newline separated list of supported panels (Type: CHAR16*) */
  EFI_DISPLAY_UTILS_PANEL_CONFIG,
  /**< Panel raw configuration */
} EFI_DISPLAY_UTILS_PROPERTY_TYPE;

/**
  Parameter structure for EFI_DISPLAY_UTILS_PANEL_LIST
*/
typedef struct
{
  UINT32   PanelCount;      /**< Number of panels in PanelList */
  UINT32   Length;          /**< String length of PanelList */
  CHAR8   *PanelList;       /**< Newline separated list of supported panels */
} EFI_DISPLAY_UTILS_PANEL_LIST_PARAM;

/**
  Display Utils render logo flags
*/
typedef enum
{
  DISPLAY_UTILS_RENDERLOGO_RENDER_SPLASH_SCREEN  = 0x00000001,
  /**< Render Splash Screen flag */
  DISPLAY_UTILS_RENDERLOGO_RENDER_BGRT_IMAGE     = 0x00000002,
  /**< Render BGRT Image flag */
  DISPLAY_UTILS_RENDERLOGO_VALIDATE_SPLASH_IMAGE = 0x00000004,
  /**< Validate Splash Image flag */
  DISPLAY_UTILS_RENDERLOGO_CLEARSCREEN           = 0x00000008,
  /**< Clear Screen flag */
  DISPLAY_UTILS_RENDERLOGO_POSITION_OVERRIDE     = 0x00000010,
  /**< Position Override flag */
} EFI_DISPLAY_UTILS_RENDERLOGO_FLAGS;

/**
  Display Utils render logo configurations structure
*/
typedef struct
{
  UINT32                uDisplayId;        /**< Display ID */
  UINT32                uFlags;            /**< Render Logo Override flags */

  /* Override Logo Position Parameters */
  UINT32                xPosition;         /**< Override x Position of Logo */
  UINT32                yPosition;         /**< Override y Position of Logo */

  /* Render Splash Screen Parameter */
  VOID                 *BGRTTable;         /**< BGRT Table */

  /* Render BGRT Image Parameters */
  UINT8                *BGRTImage;         /**< BGRT Source image buffer */
  UINT32                BGRTImageSize;     /**< BGRT image buffer size */

  /* Validate Splash Image Parameters */
  EFI_DISK_IO_PROTOCOL *pDiskIo;           /**< Disk Handle */
  UINT32                MediaId;           /**< Media ID */
  UINT32                SplashImageAddr; /**< Splash img offset in partition */
  UINT32                SplashImageMaxSize; /**< Splash image max size */
} EFI_DISPLAY_UTILS_RENDERLOGO_CONFIG;

/**
  Parameter structure for EFI_DISPLAY_UTILS_PANEL_CONFIG
*/
typedef struct
{
  UINT32   uPanelIndex;                 /**< Panel index */
  CHAR8   *pPanelConfig;                /**< Panel configuration */
  UINT32   uPanelConfigLength;          /**< Panel configuration length */
} EFI_DISPLAY_UTILS_PANEL_CONFIG_PARAMS;


/*===========================================================================
  FUNCTION DEFINITIONS
===========================================================================*/
/* EFI_DISPLAY_UTILS_SET_PROPERTY */
/** @ingroup efi_displayUtils_control
  @par Summary
  Set a property on Display UEFI

  @param[in] Type                Property type
  @param[in] Data                Pointer to data structure
  @param[in] DataSize            Size of data in bytes

  @return
  EFI_SUCCESS              : Function returned successfully.
  EFI_INVALID_PARAMETER    : Parameter passed is incorrect.
  EFI_UNSUPPORTED          : Parameter passed is not supported.
  EFI_DEVICE_ERROR         : Physical device reported an error.
*/
typedef
EFI_STATUS (EFIAPI *EFI_DISPLAY_UTILS_SET_PROPERTY)(
  IN EFI_DISPLAY_UTILS_PROPERTY_TYPE   Type,
  IN VOID                             *Data,
  IN UINT32                            DataSize
);

/* EFI_DISPLAY_UTILS_GET_PROPERTY */
/** @ingroup efi_displayUtils_control
  @par Summary
  Gets the value of a property from Display UEFI.
  If Data is NULL or DataSize is smaller then function
  returns EFI_BUFFER_TOO_SMALL with DataSize set to
  the required buffer size

  @param[in] Type                Property type
  @param[out] Data               Pointer to data structure
  @param[in/out] DataSize        [in] Max size of data
                                 [out] Actual size of data

  @return
  EFI_SUCCESS              : Function returned successfully.
  EFI_INVALID_PARAMETER    : Parameter passed is incorrect.
  EFI_UNSUPPORTED          : Parameter passed is not supported.
  EFI_DEVICE_ERROR         : Physical device reported an error.
  EFI_BUFFER_TOO_SMALL     : Buffer to small to copy data into
*/
typedef
EFI_STATUS (EFIAPI *EFI_DISPLAY_UTILS_GET_PROPERTY)(
  IN     EFI_DISPLAY_UTILS_PROPERTY_TYPE   Type,
  OUT    VOID                             *Data,
  IN OUT UINT32                           *DataSize
);

/* EFI_DISPLAY_UTILS_RENDER_LOGO */
/** @ingroup efi_displayUtils_control
  @par Summary
  Render logo function implementation.

  @param[in] psRenderLogoConfig Config of clearing screen & overriding position.

  @return
  EFI_SUCCESS              : Function returned successfully.
  EFI_INVALID_PARAMETER    : Parameter passed is incorrect.
  EFI_UNSUPPORTED          : Parameter passed is not supported.
  EFI_DEVICE_ERROR         : Physical device reported an error.
*/
typedef
EFI_STATUS (EFIAPI *EFI_DISPLAY_UTILS_RENDER_LOGO)(
  IN     EFI_DISPLAY_UTILS_RENDERLOGO_CONFIG *psRenderLogoConfig
);

/* EFI_DISPLAY_UTILS_SET_MODE */
/** @ingroup efi_displayUtils_control
  @par Summary
  Set display mode can be used for primary and externl

  @param[in] DisplayId     Primary(0) or external (1)
  @param[in] ModeNum       display mode index
  @param[in] Flags         extra info

  @return
  EFI_SUCCESS              : Function returned successfully.
  EFI_INVALID_PARAMETER    : Parameter passed is incorrect.
  EFI_UNSUPPORTED          : Parameter passed is not supported.
  EFI_DEVICE_ERROR         : Physical device reported an error.
*/
typedef
EFI_STATUS (EFIAPI *EFI_DISPLAY_UTILS_SET_MODE)(
  IN UINT32                            DisplayId,
  IN UINT32                            ModeNum,
  IN UINT32                            Flags
);


/* EFI_DISPLAY_UTILS_SET_VARIABLE */
/** @ingroup efi_displayUtils_control
  @par Summary
  Variable set function implementation.

  @param[in]  pVariableName              The variable name that need to set.
  @param[in]  pVariableValue             The variable value.
  @param[in]  uDataSize                  The size of the variable value.
  @param[in]  uFlags                     The flag to set variable.

  @return
  EFI_SUCCESS              : Function returned successfully.
  EFI_INVALID_PARAMETER    : Parameter passed is incorrect.
  EFI_UNSUPPORTED          : Parameter passed is not supported.
  EFI_DEVICE_ERROR         : Physical device reported an error.
*/
typedef
EFI_STATUS (EFIAPI *EFI_DISPLAY_UTILS_SET_VARIABLE)(
  CHAR16                          *pVariableName,
  UINT8                           *pVariableValue,
  UINTN                            uDataSize,
  UINTN                            uFlags
);


/* EFI_DISPLAY_UTILS_GET_VARIABLE */
/** @ingroup efi_displayUtils_control
  @par Summary
  Variable get function implementation.

  @param[in]   pVariableName              The variable name that need to get.
  @param[out]  pVariableValue             The variable value.
  @param[out]  pDataSize                  The size of the variable value.
  @param[in]   uFlags                     The flag to get variable.

  @return
  EFI_SUCCESS              : Function returned successfully.
  EFI_INVALID_PARAMETER    : Parameter passed is incorrect.
  EFI_UNSUPPORTED          : Parameter passed is not supported.
  EFI_DEVICE_ERROR         : Physical device reported an error.
*/
typedef
EFI_STATUS (EFIAPI *EFI_DISPLAY_UTILS_GET_VARIABLE)(
  CHAR16                          *pVariableName,
  UINT8                           *pVariableValue,
  UINTN                           *pDataSize,
  UINTN                            uFlags
);


/*===========================================================================
  PROTOCOL INTERFACE
===========================================================================*/
/** @ingroup efi_displayUtils_protocol
  @par Summary
  Qualcomm Display Utils Protocol interface.

  @par Parameters
  @inputprotoparams{}
*/
struct _EFI_QCOM_DISPLAY_UTILS_PROTOCOL
{
  UINT64                                           Revision;
  EFI_DISPLAY_UTILS_SET_PROPERTY                   DisplayUtilsSetProperty;
  EFI_DISPLAY_UTILS_GET_PROPERTY                   DisplayUtilsGetProperty;
  EFI_DISPLAY_UTILS_RENDER_LOGO                    DisplayUtilsRenderLogo;
  EFI_DISPLAY_UTILS_SET_MODE                       DisplayUtilsSetMode;
  EFI_DISPLAY_UTILS_SET_VARIABLE                   DisplayUtilsSetVariable;
  EFI_DISPLAY_UTILS_GET_VARIABLE                   DisplayUtilsGetVariable;
};

#endif  /* __EFIDISPLAYUTILS_H__ */
