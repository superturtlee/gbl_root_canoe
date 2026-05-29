/*
 * Copyright (c) 2015,2017,2019 The Linux Foundation. All rights reserved.
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

#ifndef __FASTBOOT_MAIN_H__
#define __FASTBOOT_MAIN_H__
/* USB Endpoint Direction
 * OUT: Transfer from the host
 * IN: Transfer to the host
 */
#define USB_ENDPOINT_DIRECTION_OUT 0
#define USB_ENDPOINT_DIRECTION_IN 1

/* Get ep and dir from EndpointIndex
 * EpAddress contains information about EP as below:
 * 0 .. 3b --> EP number
 * 7b --> Ep direction
 *///ablbuild/edk2_fB/QcomModulePkg/Include/Protocol/EFIUsbDevice.h
#include <Protocol/EFIUsbDevice.h>
#define USB_INDEX_TO_EP(index) ((index)&0xf)
#define USB_INDEX_TO_EPDIR(index)                                              \
  (((index) >> 7 & 0x1) ? USB_ENDPOINT_DIRECTION_IN                            \
                        : USB_ENDPOINT_DIRECTION_OUT)

typedef struct FasbootDevice {
  EFI_USB_DEVICE_PROTOCOL *UsbDeviceProtocol;
  VOID *gRxBuffer;
  VOID *gTxBuffer;
} FastbootDeviceData;

FastbootDeviceData *GetFastbootDeviceData (VOID);
EFI_STATUS HandleUsbEvents (VOID);
EFI_STATUS FastbootUsbDeviceStop (VOID);
EFI_STATUS FastbootInitialize (VOID);
#endif
