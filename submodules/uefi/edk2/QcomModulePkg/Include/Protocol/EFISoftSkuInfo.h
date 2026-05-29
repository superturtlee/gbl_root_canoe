/*
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
*/
#ifndef __EFISOFTSKU_H__
#define __EFISOFTSKU_H__

/*===========================================================================
  INCLUDE FILES
===========================================================================*/

/*===========================================================================
  MACRO DECLARATIONS
===========================================================================*/
/** @addtogroup efi_soft_sku_constants
@{ */
/**
  Protocol version.
*/
#define SOFT_SKU_REVISION 0x0000000000010001

/**
  Max number of soft SKU modules supported for each chip family
*/
#define SOFT_SKU_MODULE_MAX_NUM        16


/**
  Max number of dependent EFIs that Soft SKU needs for each chip family
*/
#define SOFT_SKU_DEPENDENCY_MAX_NUM    16

/**
  Max length in bytes for the soft SKU dependency name
*/
#define SOFT_SKU_DEPENDENCY_NAME_MAX_LEN    32

/** @} */ /* end_addtogroup efi_soft_sku_constants */

/*  Protocol GUID definition */
/** @ingroup efi_soft_sku_protocol */

#define EFI_SOFT_SKU_PROTOCOL_GUID                            \
    {                                                         \
      0xf917a796, 0x871b, 0x4595,                             \
      {                                                       \
        0x85, 0x5c, 0x95, 0x38, 0x87, 0x75, 0x2e, 0xa3        \
      }                                                       \
    }


/** @cond */
/*===========================================================================
  EXTERNAL VARIABLES
*/

/**
  Protocol declaration.
*/
typedef struct _EFI_QTI_SOFT_SKU_PROTOCOL  EFI_QTI_SOFT_SKU_PROTOCOL;


/*===========================================================================
  TYPE DEFINITIONS
===========================================================================*/

/* Soft SKU Status Type */
typedef enum
{
  SOFT_SKU_STATUS_UNINITIALIZED  = 0x0,  /* soft SKU not initialized */
  SOFT_SKU_STATUS_SUCCESS,               /* soft SKU Status Success */
  SOFT_SKU_STATUS_FAIL                   /* soft SKU Status Fail */
} EFI_SOFT_SKU_STATUS_TYPE;

/* Soft SKU ID Type */
typedef enum
{
  SOFT_SKU_ID_FP1 = 0x1,  /*soft SKU Feature Package 1 (Low SKU) */
  SOFT_SKU_ID_FP2,        /*soft SKU Feature Package 2 (Mid SKU) */
  SOFT_SKU_ID_FP3         /*soft SKU Feature Package 3 (High SKU) */
} EFI_SOFT_SKU_ID_TYPE;

/* record module name and execution status */
typedef struct
{
  EFI_SOFT_SKU_STATUS_TYPE  eStatus;
} EFI_SOFT_SKU_MODULE_INFO;

/* This structure record the modules that support soft SKU
 * uNumModules :  Number of valid moudles in sInfo
 * sInfo       :  Valid module table
*/
typedef struct
{
  UINT32                    uNumModules;
  EFI_SOFT_SKU_MODULE_INFO  sInfo[SOFT_SKU_MODULE_MAX_NUM];
} EFI_SOFT_SKU_STATUS_INFO;


/* This structure record soft SKU status */
typedef struct
{
  EFI_SOFT_SKU_STATUS_TYPE  eTALoadStatus;   /* TA load status */
  EFI_SOFT_SKU_STATUS_INFO  sTAExeStatus;   /* TA command execution status */
  EFI_SOFT_SKU_STATUS_TYPE  eTAUnloadStatus; /* TA unload status */
} EFI_SOFT_SKU_STATUS;

/* This structure record soft SKU ID */
typedef struct
{
  EFI_SOFT_SKU_ID_TYPE  eSoftSKUId;       /* Soft SKU ID */
} EFI_SOFT_SKU_ID;


/*==========================================================================
  FUNCTION DEFINITIONS
===========================================================================*/


/* EFI_SOFT_SKU_QUERY_STATUS */
/** @ingroup efi_soft_sku_protocol
  @par Summary
  query the TA status

  @return
  EFI_SUCCESS              : get TA status successfully

*/
typedef
EFI_STATUS (EFIAPI *EFI_SOFT_SKU_QUERY_STATUS)(
  EFI_SOFT_SKU_STATUS *pStatus
);

/* EFI_SOFT_SKU_QUERY_SKU_ID */
/** @ingroup efi_soft_sku_protocol
  @par Summary
  query the SKU ID (FP1, FP2, etc)

  @return
  EFI_SUCCESS              : get SKU ID successfully

*/
typedef
EFI_STATUS (EFIAPI *EFI_SOFT_SKU_QUERY_SKU_ID)(
  EFI_SOFT_SKU_ID *pSKUId
);

/*===========================================================================
  PROTOCOL INTERFACE
===========================================================================*/

/** @ingroup efi_soft_sku_protocol
  @par Summary
  QTI Enabler Protocol interface.

  @par Parameters
  @inputprotoparams{}
*/
struct _EFI_QTI_SOFT_SKU_PROTOCOL
{
  EFI_SOFT_SKU_QUERY_STATUS  SoftSKUQueryStatus;
  EFI_SOFT_SKU_QUERY_SKU_ID  SoftSKUQuerySKUId;
};


#endif  /* __EFISOFTSKU_H__ */
