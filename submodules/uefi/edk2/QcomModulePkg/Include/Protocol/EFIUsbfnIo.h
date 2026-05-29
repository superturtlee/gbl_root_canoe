/* Copyright (c) 2018, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *  * Redistributions of source code must retain the above copyright
 *  notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following
 * disclaimer in the documentation and/or other materials provided
 *  with the distribution.
 *  * Neither the name of The Linux Foundation nor the names of its
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

#ifndef __EFIUSBFNIO__
#define __EFIUSBFNIO__

#include <Protocol/UsbIo.h>
#include <Protocol/EFIUsbEx.h>

/*  Protocol GUID definition */
/** @ingroup efi_usbfnIo_protocol */
#define EFI_USBFN_IO_PROTOCOL_GUID \
 { \
   0x32d2963a, 0xfe5d, 0x4f30, {0xb6, 0x33, 0x6e, 0x5d, 0xc5, 0x58,  0x3, 0xcc}\
 }

/** @cond */
///
/// Forward reference for pure ANSI compatibility.
///
typedef struct _EFI_USBFN_IO_PROTOCOL EFI_USBFN_IO_PROTOCOL;
/** @endcond */

//
// Related Definitions
//
/** @addtogroup efi_usbfn_data_types
@{ */
/** Port type. */
typedef enum _EFI_USBFN_PORT_TYPE {
  EfiUsbUnknownPort = 0,
  /**< Driver internal default port type; this is never returned by the driver
    with a success status code. */
  EfiUsbStandardDownstreamPort,
  /**< Standard USB host;
    refer to Section 1.4 of @xhyperref{R2,[R2]} for details. */
  EfiUsbChargingDownstreamPort,
  /**< Standard USB host (charging downstream);
    refer to Section 1.4 of @xhyperref{R2,[R2]} for details. */
  EfiUsbDedicatedChargingPort,
  /**< Wall charger, not a USB host;
    refer to Section 1.4 of @xhyperref{R2,[R2]} for details. */
  EfiUsbInvalidDedicatedChargingPort
  /**< Definitely not the USB host; but at the same time, this is not a
    dedicated charging port. */
} /** @cond */EFI_USBFN_PORT_TYPE/** @endcond */;

/**
  Policy supported on an endpoint.
*/
typedef enum _EFI_USBFN_POLICY_TYPE {
  EfiUsbPolicyUndefined = 0,
  /**< Undefined policy, which should not be used by the client.  */
  EfiUsbPolicyMaxTransactionSize,
  /**< Query the maximum transaction size supported by the hardware. */
  EfiUsbPolicyZeroLengthTerminationSupport,
  /**< Whether the hardware can support appending a zero length buffer
       automatically. */
  EfiUsbPolicyZeroLengthTermination
  /**< Enable/disable the hardware support of appending a zero length
       buffer. @newpage */
} /** @cond */EFI_USBFN_POLICY_TYPE/** @endcond */;

/** Device information ID. */
typedef enum _EFI_USBFN_DEVICE_INFO_ID {
  EfiUsbDeviceInfoUnknown = 0,
  /**< Unknown. */
  EfiUsbDeviceInfoSerialNumber,
  /**< Serial number. */
  EfiUsbDeviceInfoManufacturerName,
  /**< Manufacturer name. */
  EfiUsbDeviceInfoProductName
  /**< Product name. */
} /** @cond */EFI_USBFN_DEVICE_INFO_ID/** @endcond */;

/** Endpoint direction. */
typedef enum _EFI_USBFN_ENDPOINT_DIRECTION {
  EfiUsbEndpointDirectionHostOut = 0,
  /**< Host out. */
  EfiUsbEndpointDirectionHostIn,
  /**< Host in. */
  EfiUsbEndpointDirectionDeviceTx = EfiUsbEndpointDirectionHostIn,
  /**< Device Tx. */
  EfiUsbEndpointDirectionDeviceRx = EfiUsbEndpointDirectionHostOut
  /**< Device Rx. */
} /** @cond */EFI_USBFN_ENDPOINT_DIRECTION/** @endcond */;

/** Event message. */
typedef enum _EFI_USBFN_MESSAGE
{
  EfiUsbMsgNone = 0,
  /**< No messages. */
  EfiUsbMsgSetupPacket,
  /**< Setup packet was received. */
  EfiUsbMsgEndpointStatusChangedRx,
  /**< Some of the requested data has been received from the host. It is the
    responsibility of the class driver to determine if it needs to wait for
    any remaining data. The payload is #_EFI_USBFN_TRANSFER_RESULT;
    the Buffer supplied to the EFI_USBFN_IO_TRANSFER() function must be the
    same as the Buffer field of the payload. */
  EfiUsbMsgEndpointStatusChangedTx,
  /**< Some of the requested data has been transmitted to the host. It is the
    responsibility of the class driver to determine if any remaining data
    needs to be present. The payload is #_EFI_USBFN_TRANSFER_RESULT;
    the Buffer supplied to the EFI_USBFN_IO_TRANSFER() function must be the
    same as the Buffer field of the payload. */
  EfiUsbMsgBusEventDetach,
  /**< Detach Bus event was signaled. */
  EfiUsbMsgBusEventAttach,
  /**< Attach Bus event was signaled. */
  EfiUsbMsgBusEventReset,
  /**< Reset Bus event was signaled. */
  EfiUsbMsgBusEventSuspend,
  /**< Suspend Bus event was signaled. */
  EfiUsbMsgBusEventResume,
  /**< Resume Bus event was signaled. */
  EfiUsbMsgBusEventSpeed
  /**< Bus Speed event was signaled. The payload is #_EFI_USB_BUS_SPEED.
     @newpage */
} /** @cond */EFI_USBFN_MESSAGE/** @endcond */;

/** Transfer status. */
typedef enum _EFI_USBFN_TRANSFER_STATUS
{
  UsbTransferStatusUnknown = 0,
  /**< Unknown. */
  UsbTransferStatusComplete,
  /**< Complete. */
  UsbTransferStatusAborted,
  /**< Aborted. */
  UsbTransferStatusActive,
  /**< Active. */
  UsbTransferStatusNone
  /**< No status. */
} /** @cond */EFI_USBFN_TRANSFER_STATUS/** @endcond */;

/** Bus speed. */
typedef enum _EFI_USB_BUS_SPEED
{
  UsbBusSpeedUnknown = 0,
  /**< Unknown. */
  UsbBusSpeedLow,
  /**< Low. */
  UsbBusSpeedFull,
  /**< Full. */
  UsbBusSpeedHigh,
  /**< High. */
  UsbBusSpeedSuper,
  /**< Super. */
  UsbBusSpeedSuperPlus,
  /**< Super Plus. */
  UsbBusSpeedMaximum = UsbBusSpeedSuperPlus
  /**< Maximum. */
} /** @cond */EFI_USB_BUS_SPEED/** @endcond */;

/** Endpoint type. */
typedef enum _EFI_USB_ENDPOINT_TYPE
{
  UsbEndpointControl = 0,
  /**< Control. */
  UsbEndpointIsochronous,
  /**< Isochronous. */
  UsbEndpointBulk,
  /**< Bulk. */
  UsbEndpointInterrupt
  /**< Interrupt. */
} /** @cond */EFI_USB_ENDPOINT_TYPE/** @endcond */;
/** @} */ /* end_addtogroup efi_usbfn_data_types */

/** @cond */
typedef struct
{
  EFI_USB_INTERFACE_DESCRIPTOR  *InterfaceDescriptor;
  EFI_USB_ENDPOINT_DESCRIPTOR   **EndpointDescriptorTable;
} EFI_USB_INTERFACE_INFO;

typedef struct
{
  EFI_USB_CONFIG_DESCRIPTOR  *ConfigDescriptor;
  EFI_USB_INTERFACE_INFO     **InterfaceInfoTable;
} EFI_USB_CONFIG_INFO;

typedef EFI_USB_SS_ENDPOINT_COMPANION_DESCRIPTOR
          EFI_USB_SUPERSPEED_ENDPOINT_COMPANION_DESCRIPTOR;

typedef struct
{
  EFI_USB_ENDPOINT_DESCRIPTOR                      *EndpointDescriptor;
  EFI_USB_SUPERSPEED_ENDPOINT_COMPANION_DESCRIPTOR *EndpointCompanionDescriptor;
} EFI_USB_SUPERSPEED_ENDPOINT_DESCRIPTOR;

typedef struct
{
  EFI_USB_INTERFACE_DESCRIPTOR            *InterfaceDescriptor;
  EFI_USB_SUPERSPEED_ENDPOINT_DESCRIPTOR  **EndpointDescriptorTable;
} EFI_USB_SUPERSPEED_INTERFACE_INFO;

typedef struct
{
  EFI_USB_CONFIG_DESCRIPTOR          *ConfigDescriptor;
  EFI_USB_SUPERSPEED_INTERFACE_INFO  **InterfaceInfoTable;
} EFI_USB_SUPERSPEED_CONFIG_INFO;
/** @endcond */

/** @addtogroup efi_usbfn_data_types
@{ */
/** Device information. This structure is used by the
  EFI_USBFN_IO_CONFIGURE_ENABLE_ENDPOINTS() function. */
typedef struct
{
  EFI_USB_DEVICE_DESCRIPTOR  *DeviceDescriptor;
  /**< Device descriptor. */
  EFI_USB_CONFIG_INFO        **ConfigInfoTable;
  /**< Configuration information. */
} EFI_USB_DEVICE_INFO;

/** SuperSpeed Device information. This structure is used by the
  EFI_USBFN_IO_CONFIGURE_ENABLE_ENDPOINTS_EX() function. */
typedef struct
{
  EFI_USB_DEVICE_DESCRIPTOR       *DeviceDescriptor;
  /**< SuperSpeed Device descriptor. */
  EFI_USB_SUPERSPEED_CONFIG_INFO  **ConfigInfoTable;
  /**< SuperSpeed Configuration information. */
  EFI_USB_BOS_DESCRIPTOR          *BosDescriptor;
  /**< SuperSpeed Binary Object Store. */
} EFI_USB_SUPERSPEED_DEVICE_INFO;

/* The set of USB descriptors and associated data struture **/
typedef struct _USB_DESCRIPTOR_SET {
  EFI_USB_DEVICE_DESCRIPTOR               DeviceDescriptor;
  EFI_USB_DEVICE_DESCRIPTOR               SSDeviceDescriptor;
  EFI_USB_DEVICE_QUALIFIER_DESCRIPTOR     DeviceQualifierDescriptor;
  VOID                                  **Descriptors;
  VOID                                  **SSDescriptors;
  VOID                                   *BinaryDeviceOjectStore;
  UINT8                                   StrDescCount;
  EFI_USB_STRING_DESCRIPTOR             **StrDescriptors;
} USB_DESCRIPTOR_SET;

/** Transfer result. This structure is the payload for the
  EfiUsbMsgEndpointStatusChangedRx or EfiUsbMsgEndpointStatusChangedTx
  message. */
typedef struct _EFI_USBFN_TRANSFER_RESULT
{
  UINTN BytesTransferred;
  /**< Bytes transferred. */
  EFI_USBFN_TRANSFER_STATUS     TransferStatus;
  /**< Transfer status; see #_EFI_USBFN_TRANSFER_STATUS. */
  UINT8                         EndpointIndex;
  /**< Endpoint index. */
  EFI_USBFN_ENDPOINT_DIRECTION  EndpointDirection;
  /**< Endpoint direction; see #_EFI_USBFN_ENDPOINT_DIRECTION. */
  VOID                          *Buffer;
  /**< Buffer.  @newpagetable */
} /** @cond */EFI_USBFN_TRANSFER_RESULT/** @endcond */;

/** Payload for the current message. */
typedef union _EFI_USBFN_MESSAGE_PAYLOAD
{
  EFI_USB_DEVICE_REQUEST     udr;
  /**< Device request. */
  EFI_USBFN_TRANSFER_RESULT  utr;
  /**< Transfer result; see #_EFI_USBFN_TRANSFER_RESULT for details. */
  EFI_USB_BUS_SPEED          ubs;
  /**< Bus speed; see #_EFI_USB_BUS_SPEED for details. */
} /** @cond */EFI_USBFN_MESSAGE_PAYLOAD/** @endcond */;
/** @} */ /* end_addtogroup efi_usbfn_data_types */

//
// Protocol Definitions
//

/*===========================================================================
  FUNCTION DEFINITIONS
===========================================================================*/
/* EFI_USBFN_IO_DETECT_PORT */
/** @ingroup efi_usbfn_detect_port
  @par Summary
  Returns information about the USB port type. See #_EFI_USBFN_PORT_TYPE
  for details.

  @param[in]  This              Pointer to the EFI_USBFN_IO_PROTOCOL instance.
  @param[out] PortType          USB port type.

  @return
  EFI_SUCCESS           -- Function completed successfully. \n
  EFI_INVALID_PARAMETER -- Parameter is invalid. \n
  EFI_DEVICE_ERROR      -- Physical device reported an error. \n
  EFI_NOT_READY         -- Physical device is busy or not ready to
                           process this request, or the device is not
                           attached to the host.

  @dependencies
  EFI_USBFN_IO_CONFIGURE_ENABLE_ENDPOINTS() must have already been called.
*/
typedef
EFI_STATUS
(EFIAPI *EFI_USBFN_IO_DETECT_PORT)(
  IN  EFI_USBFN_IO_PROTOCOL  *This,
  OUT EFI_USBFN_PORT_TYPE    *PortType
  );

/* EFI_USBFN_IO_CONFIGURE_ENABLE_ENDPOINTS */
/** @ingroup efi_usbfn_configure_enable_endpoints
  @par Summary
  Assuming that the hardware has already been initialized, this function
  configures the endpoints using the supplied DeviceInfo, activates the port,
  and starts receiving USB events.

  @param[in]  This              Pointer to the EFI_USBFN_IO_PROTOCOL instance.
  @param[in]  DeviceInfo        Pointer to the #EFI_USB_DEVICE_INFO data
                                structure.

  @par Description
  This function must ignore the bMaxPacketSize0 field of the standard device
  descriptor and the wMaxPacketSize field of standard endpoint descriptor that
  are made available through the supplied DeviceInfo.

  @return
  EFI_SUCCESS           -- Function completed successfully. \n
  EFI_INVALID_PARAMETER -- Parameter is invalid. \n
  EFI_DEVICE_ERROR      -- Physical device reported an error. \n
  EFI_NOT_READY         -- Physical device is busy or not ready to
                           process this request. \n
  EFI_OUT_OF_RESOURCES  -- Request could not be completed due to lack
                           of resources.
*/
typedef
EFI_STATUS
(EFIAPI *EFI_USBFN_IO_CONFIGURE_ENABLE_ENDPOINTS)(
  IN EFI_USBFN_IO_PROTOCOL *This,
  IN EFI_USB_DEVICE_INFO   *DeviceInfo
  );

/* EFI_USBFN_IO_GET_ENDPOINT_MAXPACKET_SIZE */
/** @ingroup efi_usbfn_get_endpoint_maxpacket_size
  @par Summary
  Returns the maximum packet size of the specified endpoint type for the
  supplied bus speed. If UsbBusSpeedUnknown is specified as BusSpeed, the
  maximum speed the underlying controller supports is assumed.

  @param[in]  This              Pointer to the EFI_USBFN_IO_PROTOCOL instance.
  @param[in]  EndpointType      Endpoint type as defined in
                                #_EFI_USB_ENDPOINT_TYPE.
  @param[in]  BusSpeed          Bus speed as defined in #_EFI_USB_BUS_SPEED.
  @param[out] MaxPacketSize     Maximum packet size, in bytes, of the
                                specified endpoint type.

  @return
  EFI_SUCCESS           -- Function completed successfully. \n
  EFI_INVALID_PARAMETER -- Parameter is invalid. \n
  EFI_DEVICE_ERROR      -- Physical device reported an error. \n
  EFI_NOT_READY         -- Physical device is busy or not ready to
                           process this request.
*/
typedef
EFI_STATUS
(EFIAPI *EFI_USBFN_IO_GET_ENDPOINT_MAXPACKET_SIZE)(
  IN  EFI_USBFN_IO_PROTOCOL  *This,
  IN  EFI_USB_ENDPOINT_TYPE  EndpointType,
  IN  EFI_USB_BUS_SPEED      BusSpeed,
  OUT UINT16                 *MaxPacketSize
  );

/* EFI_USBFN_IO_GET_DEVICE_INFO */
/** @ingroup efi_usbfn_get_device_info
  @par Summary
  Returns device-specific information based on the supplied identifier as a
  Unicode string.

  @note1hang This function does not pertain to the #EFI_USB_DEVICE_INFO
  data structure.

  @param[in]     This        Pointer to the EFI_USBFN_IO_PROTOCOL instance.
  @param[in]     Id          Requested information ID;
                             see #_EFI_USBFN_DEVICE_INFO_ID for details.
  @param[in,out] BufferSize  On input, the size of Buffer in bytes. On
                             output, the amount of data returned in Buffer
                             in bytes.
  @param[out]    Buffer      Pointer to a buffer to return the requested
                             information as a Unicode string
                             (not NULL-terminated).

  @par Description
  If BufferSize is not large enough to hold the requested string,
  the function fails with EFI_BUFFER_TOO_SMALL and the required size is
  returned through BufferSize. All returned strings are in Unicode format.
  @par
  Specifying EfiUsbDeviceInfoUnknown as the ID is treated as invalid parameter.

  @return
  EFI_SUCCESS           -- Function completed successfully. \n
  EFI_INVALID_PARAMETER -- Parameter is invalid. \n
  EFI_DEVICE_ERROR      -- Physical device reported an error. \n
  EFI_NOT_READY         -- Physical device is busy or not ready to
                           process this request. \n
  EFI_BUFFER_TOO_SMALL  -- Supplied buffer is not large enough to hold the
                           request string.
*/
typedef
EFI_STATUS
(EFIAPI *EFI_USBFN_IO_GET_DEVICE_INFO)(
  IN     EFI_USBFN_IO_PROTOCOL     *This,
  IN     EFI_USBFN_DEVICE_INFO_ID  Id,
  IN OUT UINTN                     *BufferSize,
  OUT    VOID                      *Buffer OPTIONAL
  );

/* EFI_USBFN_IO_GET_VENDOR_ID_PRODUCT_ID */
/** @ingroup efi_usbfn_get_vendor_id_product_id
  @par Summary
  Returns the Vendor ID (VID) and Product ID (PID) of the device.

  @param[in]  This              Pointer to the EFI_USBFN_IO_PROTOCOL instance.
  @param[out] Vid               Returned vendor ID of the device.
  @param[out] Pid               Returned product ID of the device.

  @par Description
  VIDs are 16-bit numbers owned by the vendor company and are
  assigned and maintained by the USB-IF. PIDs are 16-bit numbers
  assigned by each vendor as required.

  @return
  EFI_SUCCESS           -- Function completed successfully. \n
  EFI_INVALID_PARAMETER -- Parameter is invalid. \n
  EFI_NOT_FOUND         -- Unable to return the VID or PID.

  @dependencies
  EFI_USBFN_IO_CONFIGURE_ENABLE_ENDPOINTS() must have already been called.
*/
typedef
EFI_STATUS
(EFIAPI *EFI_USBFN_IO_GET_VENDOR_ID_PRODUCT_ID)(
  IN  EFI_USBFN_IO_PROTOCOL  *This,
  OUT UINT16                 *Vid,
  OUT UINT16                 *Pid
  );

/* EFI_USBFN_IO_ABORT_TRANSFER */
/** @ingroup efi_usbfn_abort_transfer
  @par Summary
  Aborts a transfer on the specified endpoint. This function fails with
  EFI_INVALID_PARAMETER if the specified direction is incorrect for the
  endpoint.

  @param[in]  This              Pointer to the EFI_USBFN_IO_PROTOCOL instance.
  @param[in]  EndpointIndex     Indicates the endpoint on which the ongoing
                                transfer is to be canceled.
  @param[in]  Direction         Direction of the endpoint;
                                see #_EFI_USBFN_ENDPOINT_DIRECTION for details.

  @return
  EFI_SUCCESS           -- Function completed successfully. \n
  EFI_INVALID_PARAMETER -- Parameter is invalid. \n
  EFI_DEVICE_ERROR      -- Physical device reported an error. \n
  EFI_NOT_READY         -- Physical device is busy or not ready to
                           process this request.

  @dependencies
  EFI_USBFN_IO_CONFIGURE_ENABLE_ENDPOINTS() must have already been called.
*/
typedef
EFI_STATUS
(EFIAPI *EFI_USBFN_IO_ABORT_TRANSFER)(
  IN EFI_USBFN_IO_PROTOCOL         *This,
  IN UINT8                         EndpointIndex,
  IN EFI_USBFN_ENDPOINT_DIRECTION  Direction
  );

/* EFI_USBFN_IO_GET_ENDPOINT_STALL_STATE */
/** @ingroup efi_usbfn_get_endpoint_stall_state
  @par Summary
  Returns the Stall state on the specified endpoint. This function fails
  with EFI_INVALID_PARAMETER if the specified direction is incorrect for the
  endpoint.

  @param[in]  This              Pointer to the EFI_USBFN_IO_PROTOCOL instance.
  @param[in]  EndpointIndex     Indicates the endpoint.
  @param[in]  Direction         Direction of the endpoint;
                                see #_EFI_USBFN_ENDPOINT_DIRECTION for details.
  @param[out] State             State on the specified endpoint. TRUE indicates
                                that the endpoint is in a stalled state;
                                otherwise FALSE.

  @return
  EFI_SUCCESS           -- Function completed successfully. \n
  EFI_INVALID_PARAMETER -- Parameter is invalid. \n
  EFI_DEVICE_ERROR      -- Physical device reported an error. \n
  EFI_NOT_READY         -- Physical device is busy or not ready to
                           process this request.

  @dependencies
  EFI_USBFN_IO_CONFIGURE_ENABLE_ENDPOINTS() must have already been called.
*/
typedef
EFI_STATUS
(EFIAPI *EFI_USBFN_IO_GET_ENDPOINT_STALL_STATE)(
  IN  EFI_USBFN_IO_PROTOCOL         *This,
  IN  UINT8                         EndpointIndex,
  IN  EFI_USBFN_ENDPOINT_DIRECTION  Direction,
  OUT BOOLEAN                       *State // TODO: draft
                                           // specifies IN OUT, which is
                                           // incorrect.
  );

/* EFI_USBFN_IO_SET_ENDPOINT_STALL_STATE */
/** @ingroup efi_usbfn_set_endpoint_stall_state
  @par Summary
  Sets or clears the Stall state on the specified endpoint. This function
  fails with EFI_INVALID_PARAMETER if the specified direction is incorrect for
  the endpoint.

  @param[in]  This              Pointer to the EFI_USBFN_IO_PROTOCOL instance.
  @param[in]  EndpointIndex     Indicates the endpoint.
  @param[in]  Direction         Direction of the endpoint;
                                see #_EFI_USBFN_ENDPOINT_DIRECTION for details.
  @param[in]  State             Requested Stall state on the specified endpoint.
                                TRUE causes the endpoint to stall; FALSE clears
                                an existing stall.

  @return
  EFI_SUCCESS           -- Function completed successfully. \n
  EFI_INVALID_PARAMETER -- Parameter is invalid. \n
  EFI_DEVICE_ERROR      -- Physical device reported an error. \n
  EFI_NOT_READY         -- Physical device is busy or not ready to
                           process this request.

  @dependencies
  EFI_USBFN_IO_CONFIGURE_ENABLE_ENDPOINTS() must have already been called.
*/
typedef
EFI_STATUS
(EFIAPI *EFI_USBFN_IO_SET_ENDPOINT_STALL_STATE)(
  IN EFI_USBFN_IO_PROTOCOL         *This,
  IN UINT8                         EndpointIndex,
  IN EFI_USBFN_ENDPOINT_DIRECTION  Direction,
  IN BOOLEAN                       State
  );

/* EFI_USBFN_IO_EVENTHANDLER */
/** @ingroup efi_usbfn_eventhandler
  @par Summary
  This function is called repeatedly to receive updates on USB bus states,
  receive/transmit status changes on endpoints, and receive/transmit the
  Setup packet on endpoint 0.

  @param[in]     This           Pointer to the EFI_USBFN_IO_PROTOCOL instance.
  @param[out]    Message        Indicates the event that initiated this
                                notification; see #_EFI_USBFN_MESSAGE for
                                details.
  @param[in,out] PayloadSize    On input, the size of the memory pointed to by
                                Payload. On output, the amount of data returned
                                in Payload.
  @param[out]    Payload        Pointer to the #_EFI_USBFN_MESSAGE_PAYLOAD
                                instance to return an additional payload for
                                the current message.

  @par Description
  A class driver must call EFI_USBFN_IO_EVENTHANDLER() repeatedly to receive
  updates on the transfer status and number of bytes transferred on various
  endpoints.
  @par
  Some messages have an associated payload that is returned in the supplied
  buffer as follows:
  - EfiUsbMsgEndpointStatusChangedRx -- Payload is #_EFI_USBFN_TRANSFER_RESULT
  - EfiUsbMsgEndpointStatusChangedTx -- Payload is #_EFI_USBFN_TRANSFER_RESULT
  - EfiUsbMsgBusEventSpeed -- Payload is #_EFI_USB_BUS_SPEED

  @return
  EFI_SUCCESS           -- Function completed successfully.
  EFI_INVALID_PARAMETER -- Parameter is invalid.
  EFI_DEVICE_ERROR      -- Physical device reported an error.
  EFI_NOT_READY         -- Physical device is busy or not ready to
                           process this request.
  EFI_BUFFER_TOO_SMALL  -- Supplied buffer is not large enough to hold the
                           message payload.

  @dependencies
  EFI_USBFN_IO_CONFIGURE_ENABLE_ENDPOINTS() must have already been called.
*/
typedef
EFI_STATUS
(EFIAPI *EFI_USBFN_IO_EVENTHANDLER)(
  IN     EFI_USBFN_IO_PROTOCOL      *This,
  OUT    EFI_USBFN_MESSAGE          *Message,
  IN OUT UINTN                      *PayloadSize,
  OUT    EFI_USBFN_MESSAGE_PAYLOAD  *Payload
  );

/* EFI_USBFN_IO_TRANSFER */
/** @ingroup efi_usbfn_transfer
  @par Summary
  Handles transferring data to or from the host on the specified
  endpoint, depending on the direction specified.
  @par
  If the direction is EfiUsbEndpointDirectionDeviceTx, the function
  starts a transmit transfer on the specified endpoint and returns immediately.
  @par
  If the direction is EfiUsbEndpointDirectionDeviceRx, the function
  starts a receive transfer on the specified endpoint and returns immediately.

  @param[in]  This              Pointer to the EFI_USBFN_IO_PROTOCOL instance.
  @param[in]  EndpointIndex     Indicates the endpoint on which Tx or Rx
                                transfer is to take place.
  @param[in]  Direction         Direction of the endpoint;
                                see #_EFI_USBFN_ENDPOINT_DIRECTION for details.
  @param[in]  BufferSize        Size of Buffer in bytes.
  @param[in]  Buffer            If Direction is EfiUsbEndpointDirectionDeviceRx,
                                this is the buffer in which to return the
                                received data.
                                If Direction is EfiUsbEndpointDirectionDeviceTx,
                                this is the buffer that contains the data to be
                                transmitted. \n
                                @note1
                                This buffer is allocated and freed using the
                                EFI_USBFN_IO_ALLOCATE_TRANSFER_BUFFER() and
                                EFI_USBFN_IO_FREE_TRANSFER_BUFFER() functions.
                                The caller of this function must not free or
                                reuse the buffer until the
                                EfiUsbMsgEndpointStatusChangedRx or
                                EfiUsbMsgEndpointStatusChangedTx message was
                                received along with the address of the transfer
                                buffer as part of the message payload.

  @newpage
  @par Description
  A class driver must call EFI_USBFN_IO_EVENTHANDLER() repeatedly to receive
  updates on the transfer status and number of bytes transferred on various
  endpoints. Upon update on the transfer status, the Buffer field of the
  #_EFI_USBFN_TRANSFER_RESULT data structure must be initialized with the
  Buffer pointer that was supplied to this function.
  @par
  This function fails with EFI_INVALID_PARAMETER if the specified direction
  is incorrect for the endpoint.

  @return
  EFI_SUCCESS           -- Function completed successfully. \n
  EFI_INVALID_PARAMETER -- Parameter is invalid. \n
  EFI_DEVICE_ERROR      -- Physical device reported an error. \n
  EFI_NOT_READY         -- Physical device is busy or not ready to
                           process this request.

  @dependencies
  EFI_USBFN_IO_CONFIGURE_ENABLE_ENDPOINTS() must have already been called.
*/
typedef
EFI_STATUS
(EFIAPI *EFI_USBFN_IO_TRANSFER)(
  IN EFI_USBFN_IO_PROTOCOL         *This,
  IN UINT8                         EndpointIndex,
  IN EFI_USBFN_ENDPOINT_DIRECTION  Direction,
  IN UINTN                         *BufferSize,
  IN VOID                          *Buffer
  );

/* EFI_USBFN_IO_GET_MAXTRANSFER_SIZE */
/** @ingroup efi_usbfn_get_maxtransfer_size
  @par Summary
  Returns the maximum number of bytes that the underlying controller can
  accommodate in a single transfer.

  @param[in]  This              Pointer to the EFI_USBFN_IO_PROTOCOL instance.
  @param[out] MaxTransferSize   Maximum supported transfer size in bytes.

  @return
  EFI_SUCCESS           -- Function completed successfully. \n
  EFI_INVALID_PARAMETER -- Parameter is invalid. \n
  EFI_DEVICE_ERROR      -- Physical device reported an error. \n
  EFI_NOT_READY         -- Physical device is busy or not ready to
                           process this request.
*/
typedef
EFI_STATUS
(EFIAPI *EFI_USBFN_IO_GET_MAXTRANSFER_SIZE)(
  IN  EFI_USBFN_IO_PROTOCOL  *This,
  IN  OUT UINTN              *MaxTransferSize
  );


/* EFI_USBFN_IO_ALLOCATE_TRANSFER_BUFFER */
/** @ingroup efi_usbfn_allocate_transfer_buffer
  @par Summary
  Allocates a memory region of Size bytes and returns the address of the
  allocated memory that satisfies the underlying controller requirements in
  the location referenced by Buffer.
  @par
  The allocated transfer buffer must be freed using a matching call to
  the EFI_USBFN_IO_FREE_TRANSFER_BUFFER() function.

  @param[in]  This              Pointer to the EFI_USBFN_IO_PROTOCOL instance.
  @param[in]  Size              Number of bytes to allocate for the transfer
                                buffer.
  @param[out] Buffer            Pointer to a pointer to the allocated buffer
                                if the call succeeds; otherwise undefined.

  @return
  EFI_SUCCESS           -- Function completed successfully. \n
  EFI_INVALID_PARAMETER -- Parameter is invalid. \n
  EFI_OUT_OF_RESOURCES  -- Requested transfer buffer could not be
                           allocated.
*/
typedef
EFI_STATUS
(EFIAPI *EFI_USBFN_IO_ALLOCATE_TRANSFER_BUFFER)(
  IN   EFI_USBFN_IO_PROTOCOL  *This,
  IN   UINTN                  Size,
  OUT  VOID                   **Buffer
  );

/* EFI_USBFN_IO_FREE_TRANSFER_BUFFER */
/** @ingroup efi_usbfn_free_transfer_buffer
  @par Summary
  Deallocates the memory specified by Buffer. The buffer that is freed
  must have been allocated by EFI_USBFN_IO_ALLOCATE_TRANSFER_BUFFER().

  @param[in]  This              Pointer to the EFI_USBFN_IO_PROTOCOL instance.
  @param[in]  Buffer            Pointer to the transfer buffer to deallocate.

  @return
  EFI_SUCCESS           -- Function completed successfully. \n
  EFI_INVALID_PARAMETER -- Parameter is invalid.
*/
typedef
EFI_STATUS
(EFIAPI *EFI_USBFN_IO_FREE_TRANSFER_BUFFER)(
  IN EFI_USBFN_IO_PROTOCOL  *This,
  IN VOID                   *Buffer
  );

/* EFI_USBFN_IO_START_CONTROLLER */
/** @ingroup efi_usbfn_start_controller
  @par Summary
  Initializes the hardware and internal data structures, but must not activate
  the port.

  @param[in]  This              Pointer to the EFI_USBFN_IO_PROTOCOL instance.

  @return
  EFI_SUCCESS           -- Function completed successfully. \n
  EFI_INVALID_PARAMETER -- Parameter is invalid. \n
  EFI_DEVICE_ERROR      -- Physical device reported an error.
*/
typedef
EFI_STATUS
(EFIAPI *EFI_USBFN_IO_START_CONTROLLER)(
  IN EFI_USBFN_IO_PROTOCOL  *This
  );

/* EFI_USBFN_IO_STOP_CONTROLLER */
/** @ingroup efi_usbfn_stop_controller
  @par Summary
  Disables the hardware by resetting the run/stop bit and powering off the USB
  controller, if needed.

  @param[in]  This              Pointer to the EFI_USBFN_IO_PROTOCOL instance.

  @return
  EFI_SUCCESS           -- Function completed successfully. \n
  EFI_INVALID_PARAMETER -- Parameter is invalid. \n
  EFI_DEVICE_ERROR      -- Physical device reported an error.
*/
typedef
EFI_STATUS
(EFIAPI *EFI_USBFN_IO_STOP_CONTROLLER)(
  IN EFI_USBFN_IO_PROTOCOL  *This
  );

/* EFI_USBFN_IO_SET_ENDPOINT_POLICY */
/** @ingroup efi_usbfn_set_endpoint_policy
  @par Summary
  Sets the policy supported by the endpoint.

  @note1hang This function must be called before
             EFI_USBFN_IO_START_CONTROLLER() or after
             EFI_USBFN_IO_STOP_CONTROLLER() has been called.

  @param[in] This               Pointer to the EFI_USBFN_IO_PROTOCOL instance.
  @param[in] EndpointIndex      Indicates the endpoint on which the policy
                                applies.
  @param[in] Direction          Direction of the endpoint;
                                see #_EFI_USBFN_ENDPOINT_DIRECTION for details.
  @param[in] PolicyType         Policy to be applied;
                                see #_EFI_USBFN_POLICY_TYPE for details.
  @param[in] BufferSize         Length of Buffer in bytes.
  @param[in] Buffer             Buffer associated with the policy.

  @return
  EFI_SUCCESS           -- Function completed successfully. \n
  EFI_INVALID_PARAMETER -- Parameter is invalid. \n
  EFI_DEVICE_ERROR      -- Physical device reported an error. \n
  EFI_UNSUPPORTED       -- Changing this policy value is not supported.
*/
typedef
EFI_STATUS
(EFIAPI *EFI_USBFN_IO_SET_ENDPOINT_POLICY)(
  IN      EFI_USBFN_IO_PROTOCOL         *This,
  IN      UINT8                         EndpointIndex,
  IN      EFI_USBFN_ENDPOINT_DIRECTION  Direction,
  IN      EFI_USBFN_POLICY_TYPE         PolicyType,
  IN      UINTN                         BufferSize,
  IN      VOID                          *Buffer
 );


/* EFI_USBFN_IO_GET_ENDPOINT_POLICY */
/** @ingroup efi_usbfn_get_endpoint_policy
  @par Summary
  Gets the policy supported by the endpoint.

  @param[in] This               Pointer to the EFI_USBFN_IO_PROTOCOL instance.
  @param[in] EndpointIndex      Indicates the endpoint on which the policy
                                applies.
  @param[in] Direction          Direction of the endpoint;
                                see #_EFI_USBFN_ENDPOINT_DIRECTION for details.
  @param[in] PolicyType         Policy to be applied;
                                see #_EFI_USBFN_POLICY_TYPE for details.
  @param[in,out] BufferSize  On input, the length of Buffer in bytes. On
                             output, the size of the data contained in Buffer.
  @param[out] Buffer            Buffer associated with the policy.

  @return
  EFI_SUCCESS           -- Function completed successfully. \n
  EFI_INVALID_PARAMETER -- Parameter is invalid. \n
  EFI_DEVICE_ERROR      -- Physical device reported an error. \n
  EFI_UNSUPPORTED       -- Specified policy value is not supported. \n
  EFI_BUFFER_TOO_SMALL  -- Supplied buffer is not large enough to hold the
                           requested policy value.
*/
typedef
EFI_STATUS
(EFIAPI *EFI_USBFN_IO_GET_ENDPOINT_POLICY)(
  IN      EFI_USBFN_IO_PROTOCOL         *This,
  IN      UINT8                         EndpointIndex,
  IN      EFI_USBFN_ENDPOINT_DIRECTION  Direction,
  IN      EFI_USBFN_POLICY_TYPE         PolicyType,
  IN OUT  UINTN                         *BufferSize,
  OUT     VOID                          *Buffer
 );

/* EFI_USBFN_IO_CONFIGURE_ENABLE_ENDPOINTS_EX */
/** @ingroup efi_usbfn_configure_enable_endpoints
  @par Summary
  Assuming that the hardware has already been initialized, this function
  configures the endpoints using the supplied DeviceInfo, activates the port,
  and starts receiving USB events. This function accepts DeviceInfo and
  SSDeviceInfo objects and configures the endpoints with the information from
  the object that supports the highest speed allowed by the underlying hardware.
  The high speed and super speed DeviceInfo objects passed in must have the
  same DeviceClass in the EFI_USB_DEVICE_DESCRIPTOR, otherwise this function
  will return EFI_UNSUPPORTED.

  @param[in]  This              Pointer to the EFI_USBFN_IO_PROTOCOL instance.
  @param[in]  DeviceInfo        Pointer to the #EFI_USB_DEVICE_INFO data
                                structure.

  @par Description
  This function must ignore the bMaxPacketSize0 field of the standard device
  descriptor and the wMaxPacketSize field of standard endpoint descriptor that
  are made available through the supplied DeviceInfo.

  @return
  EFI_SUCCESS           -- Function completed successfully. \n
  EFI_INVALID_PARAMETER -- Parameter is invalid. \n
  EFI_DEVICE_ERROR      -- Physical device reported an error. \n
  EFI_NOT_READY         -- Physical device is busy or not ready to
                           process this request. \n
  EFI_OUT_OF_RESOURCES  -- Request could not be completed due to lack
                           of resources.
  EFI_UNSUPPORTED       -- This operation is not supported.
*/
typedef
EFI_STATUS
(EFIAPI *EFI_USBFN_IO_CONFIGURE_ENABLE_ENDPOINTS_EX)(
  IN EFI_USBFN_IO_PROTOCOL           *This,
  IN EFI_USB_DEVICE_INFO             *DeviceInfo,
  IN EFI_USB_SUPERSPEED_DEVICE_INFO  *SSDeviceInfo
  );

/* EFI_USBFN_IO_Update_SS_Descriptor */
/** @par Summary
  Assuming that the hardware has already been initialized, this function
  update the superspeed descriptors using the supplied DeviceInfo. This
  function accepts DeviceInfo and SSDescriptorSet objects and updates these.

  @param[in]  This              Pointer to the EFI_USBFN_IO_PROTOCOL instance.
  @param[in]  DescriptorSet     Pointer to the #USB_DESCRIPTOR_SET data
                                structure.

  @return
  EFI_SUCCESS           -- Function completed successfully. \n
  EFI_INVALID_PARAMETER -- Parameter is invalid. \n
  EFI_DEVICE_ERROR      -- Physical device reported an error. \n
  EFI_NOT_READY         -- Physical device is busy or not ready to
                           process this request. \n
  EFI_OUT_OF_RESOURCES  -- Request could not be completed due to lack
                           of resources.
  EFI_UNSUPPORTED       -- This operation is not supported.
*/
typedef
EFI_STATUS
(EFIAPI *EFI_USBFN_IO_Update_SS_Descriptor)(
  IN  EFI_USBFN_IO_PROTOCOL         *This,
  OUT USB_DESCRIPTOR_SET    *DescriptorSet
  );

/** @addtogroup efi_usbfn_constants
@{ */
/** Protocol version. */
#define EFI_USBFN_IO_PROTOCOL_REVISION  0x00010003
/** @} */ /* end_addtogroup efi_usbfn_constants */

/*===========================================================================
  PROTOCOL INTERFACE
===========================================================================*/
/** @ingroup efi_usbfnIo_protocol
  @par Summary
  This protocol enables a lightweight communication between the host and
  device over the USB in a preboot environment.

  @par Parameters
  @inputprotoparams{usbfn_io_proto_params.tex}
*/
/*
This protocol enables a lightweight communication between host and device
over USB in a preboot environment.
*/
struct _EFI_USBFN_IO_PROTOCOL {
  ///
  /// The revision to which the EFI_USBFN_IO_PROTOCOL adheres. All future
  /// revisions must be backward compatible. If a future version is not
  /// backward compatible, a different GUID must be used.
  ///
  UINT32                                      Revision;
  ///
  /// Returns information about USB port type.
  ///
  EFI_USBFN_IO_DETECT_PORT                    DetectPort;
  ///
  /// Initialize all endpoints based on supplied device and configuration
  /// descriptors. Enable the device by setting the run/stop bit.
  ///
  EFI_USBFN_IO_CONFIGURE_ENABLE_ENDPOINTS     ConfigureEnableEndpoints;
  ///
  /// Returns the maximum packet size of the specified endpoint.
  ///
  EFI_USBFN_IO_GET_ENDPOINT_MAXPACKET_SIZE    GetEndpointMaxPacketSize;
  ///
  /// Returns device specific information based on the supplied identifier as a
  /// Unicode string.
  ///
  EFI_USBFN_IO_GET_DEVICE_INFO                GetDeviceInfo;
  ///
  /// Returns vendor-id and product-id of the device.
  ///
  EFI_USBFN_IO_GET_VENDOR_ID_PRODUCT_ID       GetVendorIdProductId;
  ///
  /// Aborts transfer on the specified endpoint.
  ///
  EFI_USBFN_IO_ABORT_TRANSFER                 AbortTransfer;
  ///
  /// Returns the stall state on the specified endpoint.
  ///
  EFI_USBFN_IO_GET_ENDPOINT_STALL_STATE       GetEndpointStallState;
  ///
  /// Sets or clears the stall state on the specified endpoint.
  ///
  EFI_USBFN_IO_SET_ENDPOINT_STALL_STATE       SetEndpointStallState;
  ///
  /// This function is called repeatedly to receive updates on USB bus states,
  /// receive, transmit complete events on endpoints and setup packet on
  /// endpoint 0.
  ///
  EFI_USBFN_IO_EVENTHANDLER                   EventHandler;
  ///
  /// This function handles transferring data to or from the host on the
  /// specified endpoint, depending on the direction specified.
  ///
  EFI_USBFN_IO_TRANSFER                       Transfer;
  ///
  /// Returns the maximum supported transfer size in bytes.
  ///
  EFI_USBFN_IO_GET_MAXTRANSFER_SIZE           GetMaxTransferSize;
  ///
  /// Allocates transfer buffer of the specified size that satisfies controller
  /// requirements.
  ///
  EFI_USBFN_IO_ALLOCATE_TRANSFER_BUFFER       AllocateTransferBuffer;
  ///
  /// Deallocates the memory allocated for the transfer buffer by
  /// AllocateTransferBuffer() function.
  ///
  EFI_USBFN_IO_FREE_TRANSFER_BUFFER           FreeTransferBuffer;
  ///
  /// Initialize hardware and internal data structures. The port must
  /// not be activated by this function
  ///
  EFI_USBFN_IO_START_CONTROLLER               StartController;
  ///
  /// Disable the hardware device by resetting the run/stop bit
  /// and power off the USB controller, if needed
  ///
  EFI_USBFN_IO_STOP_CONTROLLER                StopController;
  ///
  /// Sets the configuration policy for the specified non-control
  /// endpoint.
  ///
  EFI_USBFN_IO_SET_ENDPOINT_POLICY            SetEndpointPolicy;
  ///
  /// Gets the configuration policy for the specified non-control
  /// endpoint.
  ///
  EFI_USBFN_IO_GET_ENDPOINT_POLICY            GetEndpointPolicy;
  ///
  /// Initialize all endpoints based on supplied device and configuration
  /// descriptors. Enable the device by setting the run/stop bit.
  ///
  EFI_USBFN_IO_CONFIGURE_ENABLE_ENDPOINTS_EX  ConfigureEnableEndpointsEx;
  ///
  /// Update the Super speed descriptor of the device
  ///
  ///
  EFI_USBFN_IO_Update_SS_Descriptor  UpdateSSDescriptor;
};

/** @cond */
extern EFI_GUID gEfiUsbfnIoProtocolGuid;
/** @endcond */

#endif //__EFIUSBFNIO__

