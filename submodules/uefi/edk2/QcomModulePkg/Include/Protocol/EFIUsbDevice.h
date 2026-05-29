/* Copyright (c) 2015-2016, The Linux Foundation. All rights reserved.
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

#ifndef __USB_DEVICE_PROTOCOL_H__
#define __USB_DEVICE_PROTOCOL_H__

#include <IndustryStandard/Usb.h>
#include <Protocol/EFIUsbEx.h>
#include <Protocol/UsbIo.h>

/*  Protocol GUID definition */
/** @ingroup efi_usb_device_protocol */
extern EFI_GUID gEfiUsbDeviceProtocolGuid;

/** @addtogroup efi_usb_dev_constants
@{ */
/** Protocol version. */
#define EFI_USB_DEVICE_PROTOCOL_REVISION 0x000000010000001
/** @} */ /* end_addtogroup efi_usbfn_constants */

/** USB string descriptor **/
typedef EFI_USB_STRING_DESCRIPTOR USB_STRING_DESCRIPTOR;

/** USB Device Event */
typedef enum {
  UsbDeviceEventNoEvent,
  UsbDeviceEventDeviceStateChange,
  UsbDeviceEventTransferNotification,
  UsbDeviceEventOemEvent
} USB_DEVICE_EVENT;

/** USB Device Transfer Status */
typedef enum {
  UsbDeviceTransferStatusActive,
  UsbDeviceTransferStatusCompleteOK,
  UsbDeviceTransferStatusCancelled,
  UsbDeviceTransferStatusCompleteError,
} USB_DEVICE_TRANSFER_STATUS;

/** USB Device State */
typedef enum {
  UsbDeviceStateConnected,
  UsbDeviceStateDisconnected
} USB_DEVICE_STATE;

/** USB TRANSFER OUTCOME */
typedef struct {
  USB_DEVICE_TRANSFER_STATUS Status;
  UINT8 EndpointIndex;
  UINTN BytesCompleted;
  VOID *DataBuffer;
} USB_DEVICE_TRANSFER_OUTCOME;

/** OEM data */
// Every OEM DATA must start with this header information
// OEM data are used for furture extension when the cleint
// requires new information from USB Device
// The caller needs to free the context buffer with FreePool
typedef struct {
  EFI_GUID DataGuid;   // The GUID which identifies the OEM data
  UINTN VersionNumber; // Version Number
  UINTN Size; // Size, in bytes, which is equal to the header + the OEM defined
              // data
  VOID *Context; // Context information for the particular OEM data
} USB_DEVICE_OEM_DATA;

/** Data associated with the USB DEVICE Event. */
typedef union {
  USB_DEVICE_STATE DeviceState;
  USB_DEVICE_TRANSFER_OUTCOME TransferOutcome;
  USB_DEVICE_OEM_DATA OemData;
} USB_DEVICE_EVENT_DATA;

/*
   The set of USB descriptors and associated data structure
   DeviceDescriptor          Pointer to device descriptor

   Descriptors               Array of pointers to buffers, where
                             Descriptors[n] contains the response to a
                             GET_DESCRIPTOR request for configuration n. From
                             USB 2 Spec section 9.4.3:
                             "The first interface descriptor follows the
                             configuration descriptor. The endpoint
                             descriptors for the first interface follow the
                             first interface descriptor. If there are
                             additional interfaces, their interface
                             descriptor and endpoint descriptors follow the
                             first interface's endpoint descriptors".
                             The size of each buffer is the TotalLength
                             member of the Configuration Descriptor.
                             The size of the array is
                             DeviceDescriptor->NumConfigurations.

   SSDeviceDescriptor        Pointer to SuperSpeed device descriptor

   SSDescriptors             The descriptor layout of SSDescriptors follows
                             Descriptors. However, each Endpoint Descriptor
                             in SSDescriptors is followed by an endpoint
                             companion descriptor.

   DeviceQualifierDescriptor Pointer to device qualifier descriptor

   BinaryDeviceOjectStore    Pointer to binary object device store.

   StringDescriptorCount     Number of string descriptors in StringDescritors

   StringDescritors          Array of pointers to string descriptors, where
                             StringDescritors[n] contains the pointer to the
                             string descriptor n.
                             StringDescritors can be set to NULL. If it is not
set to NULL,
                             StringDescriptorCount must be greater than zero.
**/
typedef struct _USB_DEVICE_DESCRIPTOR_SET {
  EFI_USB_DEVICE_DESCRIPTOR *DeviceDescriptor;
  VOID **Descriptors;
  EFI_USB_DEVICE_DESCRIPTOR *SSDeviceDescriptor;
  VOID **SSDescriptors;
  EFI_USB_DEVICE_QUALIFIER_DESCRIPTOR *DeviceQualifierDescriptor;
  VOID *BinaryDeviceOjectStore;
  UINT8 StringDescriptorCount;
  EFI_USB_STRING_DESCRIPTOR **StringDescritors;
} USB_DEVICE_DESCRIPTOR_SET;

/*
  @par Summary
  Put data in the Tx or Rx buffer to be transfered.

  @par
  Don't call this function again until the previous transfer on the same
  endpoint
  has finished.

  @param[in]EndpointIndex    Endpoint index, as specified in endpoint
  descriptors, of
                             the endpoint to send the data from.
  @param[in]Size             Size in bytes of data.
  @param[in]Buffer           Pointer to data.

  @retval EFI_SUCCESS           The data was queued successfully.
  @retval EFI_INVALID_PARAMETER There was an error sending the data.
*/
typedef EFI_STATUS (*USB_DEVICE_SEND) (IN UINT8 EndpointIndex,
                                       IN UINTN Size,
                                       IN VOID *Buffer);

/*
  @par Summary
  Abort the transfer on EndpointIndex

  @par
  The function aborts the current transfer issued on EndpointIndex

  @param[in]EndpointIndex       Endpoint index, as specified in endpoint
  descriptors, of
                                the endpoint to send the data from.

  @retval EFI_SUCCESS           The data was queued successfully.
  @retval EFI_INVALID_PARAMETER There was an error sending the data.
*/
typedef EFI_STATUS (*USB_DEVICE_ABORT_XFER) (IN UINT8 EndpointIndex);

/*
  @par Summary
  Handle USB events. The function needs to be called periodically to handle
  the USB events by the client.

  @see EFIUsbfnIo.h

  @param[out]    Event       USB device event
  @param[out]    DataSize    Size of the event data
  @param[out]    EventData   Data associated with the event

  @retval EFI_SUCCESS           The data was queued successfully.
  @retval EFI_INVALID_PARAMETER There was an error sending the data.
*/
typedef EFI_STATUS (EFIAPI *USB_DEVICE_HANDLE_EVENT) (
    OUT USB_DEVICE_EVENT *Event,
    OUT UINTN *DataSize,
    OUT USB_DEVICE_EVENT_DATA *EventData);

/*
  @par Summary
  Allocates a memory region of Size bytes and returns the address of the
  allocated memory that satisfies the underlying controller requirements in
  the location referenced by Buffer.
  @par
  The allocated transfer buffer must be freed using a matching call to
  the USB_DEVICE_FREE_TRANSFER_BUFFER function.

  @param[in]  Size              Number of bytes to allocate for the transfer
                                buffer.
  @param[out] Buffer            Pointer to a pointer to the allocated buffer
                                if the call succeeds; otherwise undefined.

  @retval     EFI_SUCCESS           Function completed successfully.
  @retval     EFI_INVALID_PARAMETER Parameter is invalid.
  @retval     EFI_OUT_OF_RESOURCES  Requested transfer buffer could not be
  allocated.
*/
typedef EFI_STATUS (EFIAPI *USB_DEVICE_ALLOCATE_TRANSFER_BUFFER) (
    IN UINTN Size,
    OUT VOID **Buffer);

/*
  @par Summary
  Deallocates the memory specified by Buffer. The buffer that is freed
  must have been allocated by USB_DEVICE_ALLOCATE_TRANSFER_BUFFER.

  @param[in]  Buffer            Pointer to the transfer buffer to deallocate.

  @retval EFI_SUCCESS           Function completed successfully.
  @retval EFI_INVALID_PARAMETER Parameter is invalid.
*/
typedef EFI_STATUS (EFIAPI *USB_DEVICE_FREE_TRANSFER_BUFFER) (IN VOID *Buffer);

/*
  @par Summary
  Start the USB peripheral controller and respond to enumeration.

  @param[in] DeviceDescriptor           Pointer to device descriptor
  @param[in] Descriptors                Array of pointers to buffers, where
                                        Descriptors[n] contains the response to
  a
                                        GET_DESCRIPTOR request for configuration
  n. From
                                        USB Spec section 9.4.3:
                                        "The first interface descriptor follows
  the
                                        configuration descriptor. The endpoint
                                        descriptors for the first interface
  follow the
                                        first interface descriptor. If there are
                                        additional interfaces, their interface
                                        descriptor and endpoint descriptors
  follow the
                                        first interface's endpoint descriptors".

                                        The size of each buffer is the
  TotalLength
                                        member of the Configuration Descriptor.

                                        The size of the array is
                                        DeviceDescriptor->NumConfigurations.
  @param[in] DeviceQualifierDescriptor  Pointer to device qualifier descriptor
  @param[in] BinaryDeviceOjectStore     Pointer to binary object device store
   (optional) Default to NULL. If the pointer is NULL, the
  DeviceDescriptor.bcdUSB
              must be version less than 2.1
  @param[in] StringDescriptorCount      Number of string descriptors in
  StringDescritors
  @param[in] StringDescritors           Array of pointers to string descriptors,
  where
                                        StringDescritors[n] contains the pointer
  to the
                                        string descriptor n.
   (optional) Default to NULL. If not NULL, StringDescriptorCount must be
  greater
              than zero.

  @retval EFI_SUCCESS           Function completed successfully.
  @retval Others                Operation failed.
*/
typedef EFI_STATUS (*USB_DEVICE_START) (
    IN USB_DEVICE_DESCRIPTOR *DeviceDescriptor,
    IN VOID **Descriptors,
    IN USB_DEVICE_QUALIFIER_DESCRIPTOR *DeviceQualifierDescriptor,
    IN VOID *BinaryDeviceOjectStore,
    OPTIONAL IN UINT8 StringDescriptorCount,
    IN USB_STRING_DESCRIPTOR **StringDescritors OPTIONAL);

/*
  @par Summary
  Start the USB peripheral controller and respond to enumeration. This routine
  extends the functionality of USB_DEVICE_START by providing an additional
  SSDescriptors parameters to support USB 3.0 device configuration.

  @param[in]                    Descriptors needed for USB 3.0 configuration.

  @retval EFI_SUCCESS           Function completed successfully.
  @retval Others                Operation failed.
*/
typedef EFI_STATUS (*USB_DEVICE_START_EX) (
    IN USB_DEVICE_DESCRIPTOR_SET *UsbDevDescSet);

/*
  @par Summary
  Sets or clears the Stall state on the specified endpoint.

  @param[in]  EndpointIndex     Endpoint index, as specified in endpoint
  descriptors, of
                                the endpoint to send the data from.
  @param[in]  State             Requested Stall state on the specified endpoint.
                                TRUE causes the endpoint to stall; FALSE clears
                                an existing stall.

  @retval     EFI_SUCCESS       Operation succeeded
  @retval     Others            Operation failed

  @dependencies
  USB_DEVICE_START() must have already been called.
*/
typedef EFI_STATUS (EFIAPI *USB_DEVICE_SET_ENDPOINT_STALL_STATE) (
    IN UINT8 EndpointIndex,
    IN BOOLEAN State);

/*
  @par Summary
  Stop the USB controller and clean up the resources allocated.

  @par
  After calling the function, no more access to the USB Device Protocol is
  allowed.

  @retval     EFI_SUCCESS           Function completed successfully.
  @retval     Others                Operation failed.
*/
typedef EFI_STATUS (*USB_DEVICE_STOP) (VOID);

/*===========================================================================
  PROTOCOL INTERFACE
===========================================================================*/
/** @ingroup efi_usb_device_protocol
  @par Summary
  This protocol enables a lightweight communication between the host and
  device over the USB in a preboot environment. The client of the
  EFI_USB_DEVICE_PROTOCOL
  is responsible to handle any non-standard USB control transfer and transfers
  on non-control endpoint.

  @par Parameters
  @inputprotoparams{usb_dev_proto_params.tex}
*/
struct _EFI_USB_DEVICE_PROTOCOL {
  UINTN Revision;
  USB_DEVICE_START Start;
  USB_DEVICE_SEND Send;
  USB_DEVICE_HANDLE_EVENT HandleEvent;
  USB_DEVICE_ALLOCATE_TRANSFER_BUFFER AllocateTransferBuffer;
  USB_DEVICE_FREE_TRANSFER_BUFFER FreeTransferBuffer;
  USB_DEVICE_STOP Stop;
  USB_DEVICE_ABORT_XFER AbortXfer;
  USB_DEVICE_SET_ENDPOINT_STALL_STATE SetEndpointStallState;
  USB_DEVICE_START_EX StartEx;
};
typedef struct _EFI_USB_DEVICE_PROTOCOL EFI_USB_DEVICE_PROTOCOL;

#endif // ifndef __USB_DEVICE_PROTOCOL_H__
