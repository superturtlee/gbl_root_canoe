/* Copyright (c) 2015-2016, 2018 The Linux Foundation. All rights reserved.
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

#ifndef __EFIRAMPARTITION_H__
#define __EFIRAMPARTITION_H__

/** @cond */
typedef struct _EFI_RAMPARTITION_PROTOCOL EFI_RAMPARTITION_PROTOCOL;
/** @endcond */

/** @addtogroup efi_ramPartition_constants
@{ */
/**
  Protocol version.
*/
#define EFI_RAMPARTITION_PROTOCOL_REVISION 0x0000000000010002
/** @} */ /* end_addtogroup efi_ramPartition_constants */

/*  Protocol GUID definition */
/** @ingroup efi_ramPartition_protocol */
#define EFI_RAMPARTITION_PROTOCOL_GUID                                         \
  {                                                                            \
    0x5172FFB5, 0x4253, 0x7D51,                                                \
    {                                                                          \
      0xC6, 0x41, 0xA7, 0x01, 0xF9, 0x73, 0x10, 0x3C                           \
    }                                                                          \
  }

/** @cond */
/**
  External reference to the RAMPARTITION Protocol GUID defined
  in the .dec file.
*/
extern EFI_GUID gEfiRamPartitionProtocolGuid;
/** @endcond */

typedef struct _RamPartition {
  UINT64 Base;
  UINT64 AvailableLength;
} RamPartitionEntry;

#define MAX_NAME_SZ 32

typedef struct _PreLoadedImg
{
  CHAR8  Name[MAX_NAME_SZ];
  UINT64 Base;
  UINT64 Size;
}PreLoadedImageEntry;

/** @} */ /* end_addtogroup efi_ramPartition_data_types */

/*==============================================================================

                             API IMPLEMENTATION

==============================================================================*/

/* ============================================================================
**  Function : EFI_RamPartition_GetRamPartitionVersion
** ============================================================================
*/
/** @ingroup efi_ramPartition_getRamVersion
  @par Summary
  Gets the RAM Partition table version.

  @param[in]   This            Pointer to the EFI_RAMPARTITION_PROTOCOL
  instance.
  @param[out]  MajorVersion    Pointer to UINT32 which returns RAM partition
  table version
  @param[out]  MinorVersion    Pointer to UINT32 which returns RAM partition
  table version



  @return
  EFI_SUCCESS        -- Function completed successfully. \n
  EFI_PROTOCOL_ERROR -- Error occurred during the operation.
*/
typedef EFI_STATUS (EFIAPI *EFI_RAMPARTITION_GETRAMPARTITIONVERSION) (
    IN EFI_RAMPARTITION_PROTOCOL *This,
    OUT UINT32 *MajorVersion,
    OUT UINT32 *MinorVersion);

/* ============================================================================
**  Function : EFI_RamPartition_GetHighestBankBit
** ============================================================================
*/
/** @ingroup efi_ramPartition_getHighestBankBit
  @par Summary
  Gets the RAM Partition table version.

  @param[in]   This            Pointer to the EFI_RAMPARTITION_PROTOCOL
  instance.
  @param[out]  HighestBankBit  Pointer to Highest Bank Bit

  @return
  EFI_SUCCESS        -- Function completed successfully. \n
  EFI_PROTOCOL_ERROR -- Error occurred during the operation.
*/
typedef EFI_STATUS (EFIAPI *EFI_RAMPARTITION_GETHIGHESTBANKBIT) (
    IN EFI_RAMPARTITION_PROTOCOL *This,
    OUT UINT32 *HighestBankBit);

/* ============================================================================
**  Function : EFI_RamPartition_GetMinPasrSize
** ============================================================================
*/
/** @ingroup EFI_RamPartition_GetMinPasrSize
  @par Summary
  Gets the MinPasrSize

  @param[in]   This         Pointer to the EFI_RAMPARTITION_PROTOCOL instance.
  @param[out]  MinPasrSize  Pointer to MinPasrSize

  @return
  EFI_SUCCESS        -- Function completed successfully.
  EFI_PROTOCOL_ERROR -- Error occurred during the operation.
*/
typedef
EFI_STATUS
(EFIAPI *EFI_RAMPARTITION_GETMINPASRSIZE)(
   IN EFI_RAMPARTITION_PROTOCOL *This,
   OUT UINT32                *MinPasrSize
   );

/* ============================================================================
**  Function : EFI_RamPartition_GetRamPartitions
** ============================================================================
*/
/** @ingroup efi_ramPartition_getRamPartitions
  @par Summary
  Gets the Ram version as read from the hardware register.

  @param[in]   This       Pointer to the EFI_RAMPARTITION_PROTOCOL instance.
  @param[out]  pnVersion  Pointer to a UINT32 passed by the caller that
                          will be populated by the driver.

  @return
  EFI_SUCCESS          -- Function completed successfully. \n
  EFI_BUFFER_TOO_SMALL -- Returns number of partitions available
  EFI_PROTOCOL_ERROR   -- Error occurred during the operation.
*/
typedef EFI_STATUS (EFIAPI *EFI_RAMPARTITION_GETRAMPARTITIONS) (
    IN EFI_RAMPARTITION_PROTOCOL *This,
    OUT RamPartitionEntry *RamPartitions,
    IN OUT UINT32 *NumPartition);

/* ============================================================================
**  Function : EFI_RamPartition_GetPreLoadedImageTable
** ============================================================================
*/
/** @ingroup efi_ramPartition_getRamPartitions
  @par Summary
  Gets the Ram version as read from the hardware register.

  @param[in]   This       Pointer to the EFI_RAMPARTITION_PROTOCOL instance.
  @param[out]  pnVersion  Pointer to a UINT32 passed by the caller that
                          will be populated by the driver.

  @return
  EFI_SUCCESS          -- Function completed successfully. \n
  EFI_BUFFER_TOO_SMALL -- Returns number of partitions available
  EFI_PROTOCOL_ERROR   -- Error occurred during the operation.
*/
typedef
EFI_STATUS
(EFIAPI *EFI_RAMPARTITION_GETPRELOADEDIMAGETABLE)(
   IN EFI_RAMPARTITION_PROTOCOL* This,
   OUT PreLoadedImageEntry       *PreLoadedImageTable,
   IN OUT UINT32                 *NumPreloadedImg
   );

/*===========================================================================
  PROTOCOL INTERFACE
===========================================================================*/
/** @ingroup efi_ramPartition_protocol
  @par Summary
  Ram Information Protocol interface.

  @par Parameters
*/
struct _EFI_RAMPARTITION_PROTOCOL {
   UINT64                                  Revision;
   EFI_RAMPARTITION_GETRAMPARTITIONVERSION GetRamPartitionVersion;
   EFI_RAMPARTITION_GETHIGHESTBANKBIT      GetHighestBankBit;
   EFI_RAMPARTITION_GETRAMPARTITIONS       GetRamPartitions;
   EFI_RAMPARTITION_GETMINPASRSIZE         GetMinPasrSize;
   EFI_RAMPARTITION_GETPRELOADEDIMAGETABLE GetPreLoadedImageTable;
};

#endif /* __EFIRAMPARTITION_H__ */
