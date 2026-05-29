/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef LISTENER_DEF_H
#define LISTENER_DEF_H

/**
@file ListenerDef.h
@brief ListernDef definitions

===========================================================================*/
/*----------------------------------------------------------------------------
 * Listener Services callback function pointer
 * -------------------------------------------------------------------------*/

typedef EFI_STATUS (*QcomScmListenerCallbackPtr)
(
   IN VOID      *CallbackHandle,
   IN VOID      *SharedBufferPtr,
   IN UINT32     SharedBufferlen
);

#endif /* LISTENER_DEF_H */
