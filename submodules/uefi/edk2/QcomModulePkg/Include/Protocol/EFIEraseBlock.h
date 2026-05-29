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
#ifndef __EFI_ERASE_BLOCK_H__
#define __EFI_ERASE_BLOCK_H__

#include <Protocol/BlockIo.h>

/** @cond */
typedef struct _EFI_ERASE_BLOCK_PROTOCOL EFI_ERASE_BLOCK_PROTOCOL;
/** @endcond */

/** @ingroup efi_EraseBlock_constants
  Protocol version.
*/
#define EFI_ERASE_BLOCK_PROTOCOL_REVISION ((2 << 16) | (60))

/* Protocol GUID definition */
/** @ingroup efi_EraseBlock_protocol */
#define EFI_ERASE_BLOCK_PROTOCOL_GUID                                          \
  {                                                                            \
    0x95A9A93E, 0xA86E, 0x4926,                                                \
    {                                                                          \
      0xaa, 0xef, 0x99, 0x18, 0xe7, 0x72, 0xd9, 0x87                           \
    }                                                                          \
  }

typedef struct {
  EFI_EVENT Event;
  EFI_STATUS TransactionStatus;
} EFI_ERASE_BLOCK_TOKEN;

/* External reference to the Emmc Erase Protocol GUID */
/** @cond */
extern EFI_GUID gEfiEraseBlockProtocolGuid;
/** @endcond */

/*===========================================================================
  FUNCTION DEFINITIONS
===========================================================================*/

/* EFI_ERASE_BLOCKS */
/**@ingroup efi_Erase_Block_block_erase
  @par Summary
  Performs the erase operation on the specified sectors.

  @param[in] This         Indicates a pointer to the calling context.
                    Type is defined in the EFI_ERASE_BLOCK_PROTOCOL
                  description
  @param[in] MediaId      The media ID that the erase request is for
  @param[in] LBA          Start LBA for the erase operation
  @param[in] Token        A pointer to the token associated with the transaction
  @param[in] Size         The size in bytes to be erased. This must be a
  multiple
                          of the physical block size of the device

  @par Description
  This function erases the blocks specified by the LBA and
  Size parameters.  If the parameters are valid, this
  function sends the commands to the storage device to apply the erase operation
  on the specified blocks.

  @return
  EFI_SUCCESS           -- Function completed successfully. \n
  EFI_INVALID_PARAMETER -- Parameter is invalid, including an invalid
                           alignment of the start and count values. \n
  EFI_WRITE_PROTECTED   -- Device cannot be erased due to write protection. \n
  EFI_DEVICE_ERROR      -- Physical device reported an error.
*/
typedef EFI_STATUS (EFIAPI *EFI_BLOCK_ERASE) (IN EFI_BLOCK_IO_PROTOCOL *This,
                                              IN UINT32 MediaId,
                                              IN EFI_LBA LBA,
                                              IN OUT
                                                  EFI_ERASE_BLOCK_TOKEN *Token,
                                              IN UINTN Size);

/*===========================================================================
  PROTOCOL INTERFACE
===========================================================================*/
/** @ingroup efi_erase_block_protocol
  @par Summary
  Erase Block Protocol interface.

  @par Parameters
  @inputprotoparams{erase_block_proto_params.tex}
*/
struct _EFI_ERASE_BLOCK_PROTOCOL {
  UINT64 Revision;
  UINT32 EraseLengthGranularity;
  EFI_BLOCK_ERASE EraseBlocks;
};

#endif /* __EFI_ERASE_BLOCK_H__ */
