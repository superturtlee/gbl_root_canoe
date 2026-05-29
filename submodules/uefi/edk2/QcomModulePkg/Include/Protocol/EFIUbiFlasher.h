/* Copyright (c) 2017, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * * Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above
 *   copyright notice, this list of conditions and the following
 *   disclaimer in the documentation and/or other materials provided
 *   with the distribution.
 * * Neither the name of The Linux Foundation nor the names of its
 *   contributors may be used to endorse or promote products derived
 *   from this software without specific prior written permission.
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


#ifndef __EFI_UBI_FLASHER_H__
#define __EFI_UBI_FLASHER_H__

/*===========================================================================
  MACRO DECLARATIONS
===========================================================================*/
/** @cond */
typedef struct _EFI_UBI_FLASHER_PROTOCOL EFI_UBI_FLASHER_PROTOCOL;
/** @endcond */

/** @addtogroup efi_ubi_flasher_constants
@{ */
/** Protocol version. */
#define EFI_UBI_FLASHER_PROTOCOL_REVISION 0x000000010000000
/** @} */ /* end_addtogroup efi_ubi_flasher_constants */

/* Protocol GUID definition */
/** @ingroup efi_ubi_flasher_protocol */
#define EFI_UBI_FLASHER_PROTOCOL_GUID \
 {0xE3EEF434, 0x22C9, 0xE33B, {0x8F, 0x5D, 0x0E, 0x81, 0x68, 0x6A, 0x68, 0xCB}}

/** @cond */

typedef VOID* UBI_FLASHER_HANDLE;

/*===========================================================================
  EXTERNAL VARIABLES
===========================================================================*/
/**
  External reference to the EFIUbiFlasher Protocol GUID.
 */
/** @ingroup efi_ubi_flasher_protocol */
extern EFI_GUID gEfiUbiFlasherProtocolGuid;

/*===========================================================================
  TYPE DEFINITIONS
===========================================================================*/

/*===========================================================================
  FUNCTION DEFINITIONS
===========================================================================*/
/*
  @par Summary
  Open an UBI-partition or an UBIFS-volume in the nand-flash to write an
  UBI or UBIFS image.

  Like shown in the image below, UBIFS has 2 distinct layers UBI and UBIFS
  where UBIFS is the file system module and UBI is the module that provides
  nand-block wear-leveling service (like FTL) to the UBIFS layer.


                 |-----------------------|
                 |                       |
                 |        UBIFS          | UBIFS is the file system layer
                 |                       |
                 |-----------------------|
                 |                       | UBI provides FTL (Flash Translation
                 |        UBI            | Layer) service by wear-leveling
                 |                       | the nand blocks.
                 |-----------------------|

  Each file system will have its own UBIFS-image and several such UBIFS-images
  can be grouped and stored together insid one single UBI-image. The common
  nomenclature here is to call each UBIFS-image as one UBI-volume.



  @param PartitionName [IN]
    PartitionName must be null terminated ASCII string.

    While flashing an UBI-image, the UBI-image will be written directly onto an
    nand-partition. So in this case there must exist an nand-partition in the
    nand-flash matching the partition-name supplied by the user.

    Since UBIFS is an logical entity inside an UBI-partition, so while flashing
    an UBIFS-volume, the partition-name supplied by the user will be used to
    locate the volume in the "system" UBI-partition.

    Note: Please note only UBI-partition names can be found in the
    nand-partition-table and UBIFS-volume-name is an logical entity inside an
    UBI-partition and hence will not be present in the nand-partition-table.

    PartitionName pointer will be stored and used until the handle given
    out in this API is closed.

  @param UbiFlasherHandle [OUT]
    Upon succesful completion, a handle to the UBI-image-flasher will be
    returned. This handle must be used in the below APIs to flash both
    UBI and UBIFS images.

  @param NandFlashPageSizeInBytes [OUT]
    Upon succesful completion, the nand-page-size in bytes of then nand-flsh
    used in the device will be returned in this parameter.

  @param NandFlashBlockSizeInBytes [OUT]
    Upon succesful completion, the nand-block-size in bytes of then nand-flsh
    used in the device will be returned in this parameter.

    Note: The buffers used for UBI/UBIFS image flashing must be aligned to this
    nand-flash block-size.

  @retval     EFI_SUCCESS       Operation succeeded
  @retval     Others            Operation failed
*/
typedef
EFI_STATUS
(EFIAPI *EFI_UBI_FLASHER_OPEN)(
  IN  CONST CHAR8               *PartitionName,
  OUT UBI_FLASHER_HANDLE        *UbiFlasherHandle,
  OUT UINT32                    *NandFlashPageSizeInBytes,
  OUT UINT32                    *NandFlashBlockSizeInBytes
);

/*
  @par Summary
  Writes the supplied buffer containing an UBI-image to the UBI-partition
  or an UBIFS-volume.

  @param UbiFlasherHandle [IN]
    Handle to the UBI-flasher obtained using the API (EFI_UBI_FLASHER_OPEN).

  @param FrameNumber [IN]
    If the device does not have enough RAM to store the whole incoming
    UBI-image, then the UBI-image can be splittedin smaller sized buffers and
    flash them one after the other.

    Since we can split the incoming UBI-image in smaller sized buffers, this
    parameter is to assign a number to each incoming buffer to help ensure
    that the incoming UBI-image is written in order.

    This FrameNumber parameter must start with an value of 1 and must be
    incremented by 1 each time this API is called.

  @param Buffer [IN]
    Buffer containing the payload of the UBI or UBIFS-image to be flashed.

    Plesae see the next Parameter (BufferSize  )for more details on how big
    this buffer should be.

  @param BufferSize [IN]
    This BufferSize parameter species the size of above Buffer in bytes.

    The BufferSize must be aligned to the nand-flash block-size in the device.
    The nand-flash block-size of the device can be learned from the above
    API (EFI_UBI_FLASHER_OPEN) NandFlashBlockSizeInBytes parameter.

    The BufferSize must be aligned to nand-block size must be atleast
    1-nand-block. The last buffer which is less than 1-nand-block indicates
    end of UBI or UBIFS image.

  @retval     EFI_SUCCESS       Operation succeeded
  @retval     Others            Operation failed
*/
typedef
EFI_STATUS
(EFIAPI *EFI_UBI_FLASHER_WRITE)(
  IN  UBI_FLASHER_HANDLE        UbiFlasherHandle,
  IN  UINT32                    FrameNumber,
  IN  VOID                      *Buffer,
  IN  UINT32                    BufferSize
);

/*
  @par Summary
  Closes the UBI-flasher handle and concludes the flashing of the
  UBI/UBI/UBIFS-image

  @param UbiFlasherHandle [IN]
    Handle to the UBI-flasher obtained using the API (EFI_UBI_FLASHER_OPEN).

  @retval     EFI_SUCCESS       Operation succeeded
  @retval     Others            Operation failed
*/
typedef
EFI_STATUS
(EFIAPI *EFI_UBI_FLASHER_CLOSE)(
  IN  UBI_FLASHER_HANDLE        UbiFlasherHandle
);



/*===========================================================================
  PROTOCOL INTERFACE
===========================================================================*/
/** @ingroup efi_ubi_flasher_protocol
  @par Summary
  This protocol provides an interface to access the UBI Flasher driver.

  @par Parameters
  @inputprotoparams{ubi_flasher_params.tex}
*/
struct _EFI_UBI_FLASHER_PROTOCOL {
  UINTN                   Revision;
  EFI_UBI_FLASHER_OPEN    UbiFlasherOpen;
  EFI_UBI_FLASHER_WRITE   UbiFlasherWrite;
  EFI_UBI_FLASHER_CLOSE   UbiFlasherClose;
};

#endif /* __EFI_UBI_FLASHER_H__ */
