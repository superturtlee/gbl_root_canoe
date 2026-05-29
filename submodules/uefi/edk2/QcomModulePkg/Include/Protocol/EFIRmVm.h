/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef __EFIRMVM_H__
#define __EFIRMVM_H__

/** @cond */
typedef struct _EFI_RMVMINTF_PROTOCOL RmVmProtocol;
/** @endcond */

/** @addtogroup efi_rmvm_constants
@{ */
/**
  Protocol version.
*/
#define EFI_RMVM_PROTOCOL_REVISION 0x0000000000010000
/** @} */ /* end_addtogroup efi_rmvm_constants */

/*  Protocol GUID definition */
/** @ingroup efi_rmvm_protocol */
/*{0F987C66-6694-4663-9363-106B57B4FDBD}*/

#define EFI_RMVM_PROTOCOL_GUID \
  { 0X0F987C66, \
    0X6694, \
    0X4663, \
    { 0X93, 0X63, 0X10, 0X6B, 0X57, 0XB4, 0XFD, 0XBD } \
  }


/** @cond */
/**
  External reference to the Platform BDS Protocol GUID defined
  in the .dec file.
*/
extern EFI_GUID gEfiRmVmProtocolGuid;
/** @endcond */

/*=============================================================================
  TYPE DEFINITIONS
=============================================================================*/

#define RM_MEM_TYPE_NORMAL_MEMORY             0
#define RM_MEM_TYPE_IO_MEMORY                 1

#define RM_MEM_SHARE_FLAG_SANITIZE            0x1
#define RM_MEM_SHARE_FLAG_PARCEL_NOT_FINAL    0x2

#define RM_ACL_PERM_EXEC  0x1
#define RM_ACL_PERM_WRITE 0x2
#define RM_ACL_PERM_READ  0x4

#define RM_MEM_ATTRIB_NO_RESTRICT             0x0
#define RM_MEM_ATTRIB_MEM_MAPPED              0x1
#define RM_MEM_ATTRIB_MEM_NON_CACHED          0x2
#define RM_MEM_ATTRIB_MEM_CACHE_COHERENT      0x3

#define RM_VM_AUTH_ANDROID_PVM                0x2

#define MAX_RPC_BUFF_SIZE_BYTES               240



/**
  Access Control List defines
*/
typedef struct __attribute__ ((packed)) _RmMemAclEntry
{
  /* Participant VMID to whom memparcel is shared/donated */
  UINT16 Vmid;
  /* Participant VMID access rights read/write/execute */
  UINT8  Rights;
  UINT8 Reserved;
} RmMemAclEntry;

typedef struct __attribute__ ((packed)) _RmMemAcl
{
  UINT16 AclEntriesCount;   /**< Total number of ACL entries*/
  UINT16 Reserved;
  RmMemAclEntry AclEntries[]; /**< Start of entries */
} RmMemAcl;

/**
  Scatter Gather list defines
*/
typedef struct __attribute__ ((packed)) _RmMemSglEntry
{
  UINT64  BaseAddr;   /**< Memory region Base Address */
  UINT64  Size;       /**< Memory region Size */
} RmMemSglEntry;

typedef struct __attribute__ ((packed)) _RmMemSgl
{
  UINT16 SglEntriesCount;   /**< Total number of SGL entries*/
  UINT16 Reserved;
  RmMemSglEntry SglEntries[]; /**< Start of entries */
} RmMemSgl;

/**
  Memory Attributes list
*/
typedef struct __attribute__ ((packed)) _RmMemAttributeEntry
{
  /* Memory region attributes */
  UINT16 Attributes;
  /* Participant VMID affected by attribute entry */
  UINT16 AttrVmid;
} RmMemAttributeEntry;

typedef struct __attribute__ ((packed)) _RmMemAttributes
{
  /* Total number of entries */
  UINT16 AttrEntriesCount;
  /* Start of entries */
  UINT16 Reserved;
  RmMemAttributeEntry AttrEntries[];
} RmMemAttributes;


/*==============================================================================

                             API IMPLEMENTATION

==============================================================================*/

/* ============================================================================
**  Function : EFI_RM_VM_GET_ID
** ============================================================================
*/
/** @ingroup efi_rm_vm_get_id
  @par Summary
  Call into Resource Manager to get VM's identification

  @param[in]      This               Pointer to the RmVmProtocol instance.
  @param[in]      VmID               VmID to query
  @param[OUT]     DataBuff           Data buffer with response
  @param[OUT]     DataBuffSize       Buffer size

  @return
  EFI_SUCCESS           -- Function completed successfully.
  EFI_NOT_READY         -- Protocol Dependencies not satisfied.
*/
typedef
EFI_STATUS
(EFIAPI *EFI_RM_VM_GET_ID)(
   IN   RmVmProtocol   *This,
   IN   UINT16         VmID,
   OUT  VOID           **DataBuff,
   OUT  UINT32         *DataBuffSize
   );


/* ============================================================================
**  Function : EFI_RM_MEM_DONATE
** ============================================================================
*/
/** @ingroup efi_rm_mem_donate
  @par Summary
  Donate memory from one VM to another

  @param[in]      This        Pointer to the RmVmProtocol instance.
  @param[in]      MemType     Type of memory parcel, Normal Vs IO.
  @param[in]      Flags       Memparcel flags eg sanitize, finalize bits.
  @param[in]      Label       Value to be used as a label for memparcel
  @param[in]      AclPtr      Pointer to access control vmid list.
  @param[in]      SglPtr      Pointer to scatter gather list for memory regions.
  @param[in]      AttrPtr     Pointer to list of attributes for destn VMIDs.
  @param[in]      SrcVmid     Vmid of the source VM, eg HLOS VMID.
  @param[in]      DestVmid    VMID of the destination VM
                              to whom the parcel is donated.
  @param[OUT]     MemHandle   Pointer to Memory parcel handle.

  @return
  EFI_SUCCESS           -- Function completed successfully, Else error code.
*/
typedef
EFI_STATUS
(EFIAPI *EFI_RM_MEM_DONATE)(
   IN  RmVmProtocol      *This,
   IN  UINT8                  MemType,
   IN  UINT8                  Flags,
   IN  UINT32                 Label,
   IN  RmMemAcl               *AclPtr,
   IN  RmMemSgl               *SglPtr,
   IN  RmMemAttributes        *AttrPtr,
   IN  UINT16                 SrcVmid,
   IN  UINT16                 DestVmid,
   OUT UINT32                 *MemHandle
   );


/* ============================================================================
**  Function : EFI_RM_FW_SET_VM_FIRMWARE
** ============================================================================
*/
/** @ingroup efi_rm_fw_set_vm_firmware
  @par Summary
  Call into Resource Manager to set invoke message FW_SET_VM_FIRMWARE

  @param[in]      This          Pointer to the RmVmProtocol instance.
  @param[in]      Auth          Auth Mechanism: 2 = Android pVm.
  @param[in]      MemHandle     Handle to a memory parcel,
                                which must have previously been donated to RM.
  @param[in]      FwOffset      Firmware image's offset into the
                                specified memory parcel.
  @param[in]      FwSize        Size in bytes of the firmware image.


  @return
  EFI_SUCCESS           -- Function completed successfully, Else error code.
*/
typedef
EFI_STATUS
(EFIAPI *EFI_RM_FW_SET_VM_FIRMWARE)(
   IN RmVmProtocol       *This,
   IN  UINT16                 Auth,
   IN  UINT32                 MemHandle,
   IN  UINT64                 FwOffset,
   IN  UINT64                 FwSize
   );


/* ============================================================================
**  Function : EFI_RM_FW_MILESTONE
** ============================================================================
*/
/** @ingroup efi_rm_fw_milestone
  @par Summary
  Call into Resource Manager to set FW milestone

  @param[in]  This      Pointer to the RmVmProtocol instance.

  @return
  EFI_SUCCESS           -- Function completed successfully, Else error code. \n
*/
typedef
EFI_STATUS
(EFIAPI *EFI_RM_FW_MILESTONE)(
   IN RmVmProtocol       *This
   );



/*===========================================================================
  PROTOCOL INTERFACE
===========================================================================*/
/** @ingroup efi_rmvm_protocol
  @par Summary
  Qualcomm Resource Manager Protocol interface.

  @par Parameters
*/
typedef struct _EFI_RMVMINTF_PROTOCOL {
  UINT64                      Revision;
  EFI_RM_VM_GET_ID            VmGetID;
  EFI_RM_MEM_DONATE           MemDonate;
  EFI_RM_FW_SET_VM_FIRMWARE   FwSetVmFirmware;
  EFI_RM_FW_MILESTONE         SetFwMilestone;
} RmVmProtocol;
#endif /* __EFIRMVM_H__ */

