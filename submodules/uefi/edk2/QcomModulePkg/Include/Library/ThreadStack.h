/*
 * Copyright (c) 2020, The Linux Foundation. All rights reserved.
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

#ifndef __THREADSTACK_H__
#define __THREADSTACK_H__

#include "list.h"
#include <Protocol/EFIKernelInterface.h>

/* Stack address will change frequently, we need record the top adderss and
 * free it at suitable place.
 */
typedef struct _THREAD_STACK_ENTRY {
  Thread                      *Thread;
  VOID                        *StackBottom;
  VOID                        *StackTop;
}THREAD_STACK_ENTRY;

typedef struct _THREAD_LIST_TABLE {
  struct list_node Node;
  struct _THREAD_STACK_ENTRY   *ThreadStackEntry;
}THREAD_STACK_NODE;

EFI_STATUS __attribute__ ( (no_sanitize ("safe-stack")))
AllocateUnSafeStackPtr (Thread* CurrentThread);
VOID** __attribute__ ( (no_sanitize ("safe-stack")))
__safestack_pointer_address (VOID);
VOID ThreadStackNodeRemove (Thread* CurrentThread);
VOID ThreadStackReleaseCb (VOID * Arg);
EFI_STATUS InitThreadUnsafeStack (VOID);
VOID DeInitThreadUnsafeStack (VOID);
EFI_STATUS EFIAPI TimerStackInit (VOID);
VOID CloseStackTimer (VOID);
#endif
