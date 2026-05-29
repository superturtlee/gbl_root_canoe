/*
 * Copyright (c) 2015-2018, The Linux Foundation. All rights reserved.
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
#include "UsbDescriptors.h"
#include "FastbootCmds.h"
#include <Library/Board.h>

#define FAST_BOOT_VENDOR 0x18d1
#define FAST_BOOT_IDPRODUCT 0xD00D
#define MAX_DESC_LEN 62
#define UUID_STR_LEN 36

STATIC
EFI_USB_DEVICE_DESCRIPTOR
DeviceDescriptor = {
    sizeof (EFI_USB_DEVICE_DESCRIPTOR), // uint8  bLength;
    USB_DESC_TYPE_DEVICE,               // uint8  bDescriptorType;
    0x0210,                             // uint16 bcdUSB;
    0x00,                               // uint8  bDeviceClass;
    0x00,                               // uint8  bDeviceSubClass;
    0x00,                               // uint8  bDeviceProtocol;
    64,                                 // uint8  bMaxPacketSize0;
    FAST_BOOT_VENDOR,                   // uint16 idVendor;
    FAST_BOOT_IDPRODUCT,                // uint16 idProduct;
    0x100,                              // uint16 bcdDevice;
    1,                                  // uint8  iManufacturer;
    2,                                  // uint8  iProduct;
    3,                                  // uint8  iSerialNumber;
    1                                   // uint8  bNumConfigurations;
};

STATIC
EFI_USB_DEVICE_DESCRIPTOR
SSDeviceDescriptor = {
    sizeof (EFI_USB_DEVICE_DESCRIPTOR), // uint8  bLength;
    USB_DESC_TYPE_DEVICE,               // uint8  bDescriptorType;
    0x0300,                             // uint16 bcdUSB;
    0x00,                               // uint8  bDeviceClass;
    0x00,                               // uint8  bDeviceSubClass;
    0x00,                               // uint8  bDeviceProtocol;
    9,                                  // uint8  bMaxPacketSize0;
    FAST_BOOT_VENDOR,                   // uint16 idVendor;
    FAST_BOOT_IDPRODUCT,                // uint16 idProduct;
    0x100,                              // uint16 bcdDevice;
    1,                                  // uint8  iManufacturer;
    2,                                  // uint8  iProduct;
    3,                                  // uint8  iSerialNumber;
    1                                   // uint8  bNumConfigurations;
};

EFI_USB_DEVICE_QUALIFIER_DESCRIPTOR
DeviceQualifier = {
    sizeof (EFI_USB_DEVICE_QUALIFIER_DESCRIPTOR), // uint8  bLength;
    USB_DESC_TYPE_DEVICE_QUALIFIER,               // uint8  bDescriptorType;
    0x0200,                                       // uint16 bcdUSB;
    0xff,                                         // uint8  bDeviceClass;
    0xff,                                         // uint8  bDeviceSubClass;
    0xff,                                         // uint8  bDeviceProtocol;
    64,                                           // uint8  bMaxPacketSize0;
    1,                                            // uint8  bNumConfigurations;
    0                                             // uint8  bReserved;
};

STATIC
struct _SSCfgDescTree {
  EFI_USB_CONFIG_DESCRIPTOR ConfigDescriptor;
  EFI_USB_INTERFACE_DESCRIPTOR InterfaceDescriptor;
  EFI_USB_ENDPOINT_DESCRIPTOR EndpointDescriptor0;
  EFI_USB_SS_ENDPOINT_COMPANION_DESCRIPTOR EndpointCompanionDescriptor0;
  EFI_USB_ENDPOINT_DESCRIPTOR EndpointDescriptor1;
  EFI_USB_SS_ENDPOINT_COMPANION_DESCRIPTOR EndpointCompanionDescriptor1;
} TotalSSConfigDescriptor = {
    {
        sizeof (EFI_USB_CONFIG_DESCRIPTOR), // uint8  bLength;
        USB_DESC_TYPE_CONFIG,               // uint8  bDescriptorType;
        sizeof (TotalSSConfigDescriptor),   // uint16 wTotalLength;
        1,                                  // uint8  bNumInterfaces;
        1,                                  // uint8  bConfigurationValue;
        0,                                  // uint8  iConfiguration;
        0x80,                               // uint8  bmAttributes;
        0x10                                // uint8  bMaxPower;
    },
    {sizeof (EFI_USB_INTERFACE_DESCRIPTOR), // uint8  bLength;
     USB_DESC_TYPE_INTERFACE,               // uint8  bDescriptorType;
     0,                                     // uint8  bInterfaceNumber;
     0,                                     // uint8  bAlternateSetting;
     2,                                     // uint8  bNumEndpoints;
     0xff,                                  // uint8  bInterfaceClass;
     0x42,                                  // uint8  bInterfaceSubClass;
     0x03,                                  // uint8  bInterfaceProtocol;
     4},
    {
        sizeof (EFI_USB_ENDPOINT_DESCRIPTOR), // uint8  bLength;
        USB_DESC_TYPE_ENDPOINT,               // uint8  bDescriptorType;
        ENDPOINT_ADDR (USBLB_BULK_EP, TRUE),  // uint8  bEndpointAddress;
        USB_ENDPOINT_BULK,                    // uint8  bmAttributes;
        1024, // uint16 wMaxPacketSize; SS=1024, HS=512 , FS=64
        0     // uint8  bInterval;
    },
    {
        sizeof (EFI_USB_SS_ENDPOINT_COMPANION_DESCRIPTOR), // uint8 bLength
        USB_DESC_TYPE_SS_ENDPOINT_COMPANION, // uint8 bDescriptorType
        4, // uint8 bMaxBurst,    0 => max burst 1
        0, // uint8 bmAttributes, 0 => no stream
        0, // uint8 wBytesPerInterval. Does not apply to BULK
    },
    {
        sizeof (EFI_USB_ENDPOINT_DESCRIPTOR), // uint8  bLength;
        USB_DESC_TYPE_ENDPOINT,               // uint8  bDescriptorType;
        ENDPOINT_ADDR (USBLB_BULK_EP, FALSE), // uint8  bEndpointAddress;
        USB_ENDPOINT_BULK,                    // uint8  bmAttributes;
        1024, // uint16 wMaxPacketSize; SS=1024, HS=512 , FS=64
        0     // uint8  bInterval;
    },
    {
        sizeof (EFI_USB_SS_ENDPOINT_COMPANION_DESCRIPTOR), // uint8 bLength
        USB_DESC_TYPE_SS_ENDPOINT_COMPANION, // uint8 bDescriptorType
        4, // uint8 bMaxBurst,    0 => max burst 1
        0, // uint8 bmAttributes, 0 => no stream
        0, // uint8 wBytesPerInterval. Does not apply to BULK
    }};

#pragma pack(1)
typedef struct _CfgDescTree {
  EFI_USB_CONFIG_DESCRIPTOR ConfigDescriptor;
  EFI_USB_INTERFACE_DESCRIPTOR InterfaceDescriptor;
  EFI_USB_ENDPOINT_DESCRIPTOR EndpointDescriptor0;
  EFI_USB_ENDPOINT_DESCRIPTOR EndpointDescriptor1;
} CONFIG_DESCRITPROS;
#pragma pack()

STATIC CONFIG_DESCRITPROS TotalConfigDescriptor = {
    {
        sizeof (EFI_USB_CONFIG_DESCRIPTOR), // uint8  bLength;
        USB_DESC_TYPE_CONFIG,               // uint8  bDescriptorType;
        sizeof (EFI_USB_CONFIG_DESCRIPTOR) +
            sizeof (EFI_USB_INTERFACE_DESCRIPTOR) +
            sizeof (EFI_USB_ENDPOINT_DESCRIPTOR) +
            sizeof (EFI_USB_ENDPOINT_DESCRIPTOR), // uint16 wTotalLength;
        1,                                        // uint8  bNumInterfaces;
        1,                                        // uint8  bConfigurationValue;
        0,                                        // uint8  iConfiguration;
        0x80,                                     // uint8  bmAttributes;
        0x50                                      // uint8  bMaxPower;
    },
    {sizeof (EFI_USB_INTERFACE_DESCRIPTOR), // uint8  bLength;
     USB_DESC_TYPE_INTERFACE,               // uint8  bDescriptorType;
     0,                                     // uint8  bInterfaceNumber;
     0,                                     // uint8  bAlternateSetting;
     2,                                     // uint8  bNumEndpoints;
     0xff,                                  // uint8  bInterfaceClass;
     0x42,                                  // uint8  bInterfaceSubClass;
     0x03,                                  // uint8  bInterfaceProtocol;
     4},
    {
        sizeof (EFI_USB_ENDPOINT_DESCRIPTOR), // uint8  bLength;
        USB_DESC_TYPE_ENDPOINT,               // uint8  bDescriptorType;
        ENDPOINT_ADDR (USBLB_BULK_EP, TRUE),  // uint8  bEndpointAddress;
        USB_ENDPOINT_BULK,                    // uint8  bmAttributes;
        512, // uint16 wMaxPacketSize; HS=512 , FS=64
        0    // uint8  bInterval;
    },
    {
        sizeof (EFI_USB_ENDPOINT_DESCRIPTOR), // uint8  bLength;
        USB_DESC_TYPE_ENDPOINT,               // uint8  bDescriptorType;
        ENDPOINT_ADDR (USBLB_BULK_EP, FALSE), // uint8  bEndpointAddress;
        USB_ENDPOINT_BULK,                    // uint8  bmAttributes;
        512, // uint16 wMaxPacketSize; HS=512 , FS=64
        1    // uint8  bInterval;
    },
};

STATIC
CONST
UINT8
Str0Descriptor[4] = {
    sizeof (Str0Descriptor), USB_DESC_TYPE_STRING, 0x09, 0x04 // Langid : US_EN.
};

STATIC
CONST
UINT8
StrManufacturerDescriptor[14] = {
    sizeof (StrManufacturerDescriptor),
    USB_DESC_TYPE_STRING,
    'G',
    0,
    'o',
    0,
    'o',
    0,
    'g',
    0,
    'l',
    0,
    'e',
    0,
};

STATIC
UINT8
StrSerialDescriptor[MAX_DESC_LEN];

STATIC
CONST
UINT8
StrInterfaceDescriptor[18] = {
    sizeof (StrInterfaceDescriptor),
    USB_DESC_TYPE_STRING,
    'f',
    0,
    'a',
    0,
    's',
    0,
    't',
    0,
    'b',
    0,
    'o',
    0,
    'o',
    0,
    't',
    0,
};

STATIC
CONST
UINT8
StrProductDescriptor[16] = {
    sizeof (StrProductDescriptor),
    USB_DESC_TYPE_STRING,
    'A',
    0,
    'n',
    0,
    'd',
    0,
    'r',
    0,
    'o',
    0,
    'i',
    0,
    'd',
    0,
};

EFI_USB_STRING_DESCRIPTOR *StrDescriptors[5] = {
    (EFI_USB_STRING_DESCRIPTOR *)Str0Descriptor,
    (EFI_USB_STRING_DESCRIPTOR *)StrManufacturerDescriptor,
    (EFI_USB_STRING_DESCRIPTOR *)StrProductDescriptor,
    (EFI_USB_STRING_DESCRIPTOR *)StrSerialDescriptor,
    (EFI_USB_STRING_DESCRIPTOR *)StrInterfaceDescriptor};

VOID
BuildDefaultDescriptors (OUT USB_DEVICE_DESCRIPTOR **DevDesc,
                         OUT VOID **Descriptors,
                         OUT USB_DEVICE_DESCRIPTOR **SSDevDesc,
                         OUT VOID **SSDescriptors)
{
  UINT8 Index = 0;
  UINT8 NumCfg = 0;
  CHAR8 Str_UUID[UUID_STR_LEN];
  UINT32 i;
  EFI_STATUS Status;
  VOID **TempDescs;
  VOID **TempSSDescs;

  Status = BoardSerialNum (Str_UUID, sizeof (Str_UUID));
  if (Status != EFI_SUCCESS) {
    DEBUG ((EFI_D_ERROR, "Error Finding board serial num: %x\n", Status));
    return;
  }

  /* Full UUID descriptor should be length 74, now it only
   * works up to 62.
   * The array members of StrSerialDescriptor is:
   * sizeof(StrSerialDescriptor), USB_DESC_TYPE_STRING,
   * '9', 0,
   * 'a', 0,
   * '1', 0,
   * '8', 0,
   * '9', 0,
   * '9', 0,
   * '1', 0,
   */
  if (((AsciiStrLen (Str_UUID) - 1) * 2 + 3) > (MAX_DESC_LEN - 1)) {
    DEBUG ((EFI_D_ERROR, "Error the array index out of bounds\n"));
    return;
  }

  StrSerialDescriptor[0] = AsciiStrLen (Str_UUID) * 2 + 2;
  StrSerialDescriptor[1] = USB_DESC_TYPE_STRING;
  for (i = 0; i < AsciiStrLen (Str_UUID); i++) {
    StrSerialDescriptor[i * 2 + 2] = Str_UUID[i];
    StrSerialDescriptor[i * 2 + 3] = 0;
  }

  *DevDesc = &DeviceDescriptor;
  *SSDevDesc = &SSDeviceDescriptor;
  NumCfg = DeviceDescriptor.NumConfigurations;

  TempDescs = AllocateZeroPool (NumCfg * sizeof (struct _CfgDescTree *));
  if (TempDescs == NULL) {
    DEBUG (
        (EFI_D_ERROR, "Error Allocating memory for HS config descriptors\n"));
    return;
  }

  TempSSDescs = AllocateZeroPool (NumCfg * sizeof (struct _SSCfgDescTree *));
  if (TempSSDescs == NULL) {
    DEBUG (
        (EFI_D_ERROR, "Error Allocating memory for SS config descriptors\n"));
    FreePool (TempDescs);
    TempDescs = NULL;
    return;
  }
  for (Index = 0; Index < NumCfg; Index++) {
    TempDescs[Index] = &TotalConfigDescriptor;
  }
  for (Index = 0; Index < NumCfg; Index++) {
    TempSSDescs[Index] = &TotalSSConfigDescriptor;
  }
  *Descriptors = TempDescs;
  *SSDescriptors = TempSSDescs;
}
