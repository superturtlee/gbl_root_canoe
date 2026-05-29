/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/* Copyright (c) 2019, The Linux Foundation. All rights reserved.
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

#ifndef __EFIHSUART_H__
#define __EFIHSUART_H__

/** @addtogroup efi_hsuart_constants
@{ */
/**
  Protocol version.
*/
#define HSUART_DXE_REVISION 0x00010006
/** @} */ /* end_addtogroup efi_hsuart_constants */

/*  HS-UART UEFI Procotol GUID */
/** @ingroup efi_hsuart_protocol */
extern EFI_GUID gQtiHSUartProtocolGuid;

/*===========================================================================
  FUNCTION DEFINITIONS
===========================================================================*/
/* OPEN */
/** @ingroup OPEN
  @par Summary
  This function is called by the client code to initialize the respective
  HS-UART core instance used by the client.
*/
typedef
RETURN_STATUS
(EFIAPI *OPEN) (
  VOID
);

/* READ */
/** @ingroup READ
  @par Summary
  Performs an hsuart read.

  @param[out] buffer                Buffer into which data is read.
  @param[in]  buffer_len            Length of data that needs to be read.
                                    the slave.
  @return
  Actual bytes read
*/
typedef
UINTN
(EFIAPI *READ) (
  OUT UINT8 *user_buffer,
  IN UINTN bytes_requested
);

/* WRITE */
/** @ingroup WRITE
  @par Summary
  Performs an hsuart write.

  @param[out] buffer                Buffer from which data is written.
  @param[in]  buffer_len            Length of data that needs to be written.
                                    the slave.

  @return
  Actual bytes written
*/
typedef
UINTN
(EFIAPI *WRITE) (
  IN UINT8 *user_buffer,
  IN UINTN bytes_to_send
);

/* POLL */
/** @ingroup POLL
  @par Summary
  Polls for RX Watermark

  @return
  Boolean TRUE if watermark set and vice versa.
*/
typedef
BOOLEAN
(EFIAPI *POLL) (
  VOID
);

/* CLOSE */
/** @ingroup CLOSE
  @par Summary
  This function is called by the client code to de-initialize the respective
  HS-UART core instance used by the client.
*/
typedef
RETURN_STATUS
(EFIAPI *CLOSE) (
  VOID
);

/* SETBAUD */
/** @ingroup SETBAUD
  @par Summary
  This function is called by the client code to set baudrate for the respective
  HS-UART core instance used by the client.

*/
typedef
RETURN_STATUS
(EFIAPI *SETBAUD) (
  IN UINTN baud_rate
);

/*===========================================================================
  PROTOCOL INTERFACE
===========================================================================*/
/** @ingroup efi_hsuart_protocol
  @par Summary
  QTI HS-UART Protocol interface.

  @par Parameters
  @inputprotoparams{hsuart_proto_params.tex}
*/
typedef struct _EFI_QTI_HSUART_PROTOCOL {
  UINT64        Revision;
  OPEN          open;
  READ          read;
  WRITE         write;
  POLL          poll;
  CLOSE         close;
  SETBAUD       set_baudrate;
} EFI_QTI_HSUART_PROTOCOL;

#endif  /* __EFIHSUART_H__ */
