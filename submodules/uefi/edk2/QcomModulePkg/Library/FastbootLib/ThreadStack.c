/* Copyright (c) 2020, The Linux Foundation. All rights reserved.
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

/*
 * Changes from Qualcomm Innovation Center are provided under the following license:
 *
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted (subject to the limitations in the
 *  disclaimer below) provided that the following conditions are met:
 *
 *      * Redistributions of source code must retain the above copyright
 *        notice, this list of conditions and the following disclaimer.
 *
 *      * Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials provided
 *        with the distribution.
 *
 *      * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *        contributors may be used to endorse or promote products derived
 *        from this software without specific prior written permission.
 *
 *  NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 *  GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 *  HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 *   WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 *  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 *  ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 *  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 *  GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 *  IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 *  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 *  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#define PAGE_SIZE (_AC(1, UL) << PAGE_SHIFT)
#define DIV_ROUND_UP(n, d) (((n) + (d) - 1) / (d))
#define HIBERNATE_SIG           "S1SUSPEND"
#define __NEW_UTS_LEN 64
#define ALIGN32_BELOW(addr) ALIGN_POINTER (addr - 32, 32)
#define LOCAL_ROUND_TO_PAGE(x, y) (((x) + (y - 1)) & (~(y - 1)))
#define ROUND_TO_PAGE(x, y) ((ADD_OF ((x), (y))) & (~(y)))
#define ALIGN_PAGES(x, y) (((x) + (y - 1)) / (y))
#define DECOMPRESS_SIZE_FACTOR 8
#define ALIGNMENT_MASK_4KB 4096
#define MAX_NUMBER_OF_LOADED_IMAGES 32
#define PVMFW_CONFIG_MAX_BLOBS 2
#include "Library/ThreadStack.h"
#include <Library/DebugLib.h>
#include <Library/Debug.h>
#include "LinuxLoaderLib.h"

STATIC EFI_KERNEL_PROTOCOL  *KernIntf = NULL;
STATIC VOID* UnSafeStackPtr;
STATIC BOOLEAN IsMultiStack = TRUE;
// This is a runtime variable to record if "thread unsafe stack low level" is
// supported,  if TRUE, we can set/get multithread stack by API, if FALSE, we
// can manage stack by link table.
STATIC BOOLEAN IsThreadUSSLLSupported = FALSE;
STATIC THREAD_STACK_NODE * ThreadStackNodeList;
STATIC Thread* TimerThread = NULL;
STATIC EFI_EVENT TimerStackEvent;

STATIC THREAD_STACK_NODE * ThreadStackNodeInit (Thread *thread)
{
  THREAD_STACK_NODE *ThreadStackNodeTmp =
          AllocateZeroPool (sizeof (THREAD_STACK_NODE));

  if (ThreadStackNodeTmp == NULL) {
    DEBUG ((EFI_D_ERROR, "Failed to Allocate memory for "
            "ThreadStackNodeTmp\n"));
    return NULL;
  }

  list_clear_node (&ThreadStackNodeTmp->Node);

  ThreadStackNodeTmp->ThreadStackEntry =
          AllocateZeroPool (sizeof (THREAD_STACK_ENTRY));

  if (!ThreadStackNodeTmp->ThreadStackEntry) {
    DEBUG ((EFI_D_ERROR, "Failed to Allocate memory for "
            "ThreadStackNodeTmp->ThreadStackEntry\n"));
    FreePool (ThreadStackNodeTmp);
    ThreadStackNodeTmp = NULL;
    return NULL;
  }

  ThreadStackNodeTmp->ThreadStackEntry->Thread = thread;

  ThreadStackNodeTmp->ThreadStackEntry->StackBottom =
          AllocateZeroPool (BOOT_LOADER_MAX_UNSAFE_STACK_SIZE);

  if (!ThreadStackNodeTmp->ThreadStackEntry->StackBottom) {
    DEBUG ((EFI_D_ERROR, "Failed to Allocate memory for "
            "ThreadStackNodeTmp->ThreadStackEntry->StackBottom \n"));
    FreePool (ThreadStackNodeTmp->ThreadStackEntry);
    FreePool (ThreadStackNodeTmp);
    ThreadStackNodeTmp->ThreadStackEntry = NULL;
    ThreadStackNodeTmp = NULL;
    return NULL;
  }

  ThreadStackNodeTmp->ThreadStackEntry->StackTop =
          ThreadStackNodeTmp->ThreadStackEntry->StackBottom;
  ThreadStackNodeTmp->ThreadStackEntry->StackTop
          += BOOT_LOADER_MAX_UNSAFE_STACK_SIZE;

  return ThreadStackNodeTmp;
}

/* If kernel version is bigger than EFI_KERNEL_PROTOCOL_VER_UNSAFE_STACK_APIS,
 * use API to set/get stack, or else use link table
**/
STATIC EFI_STATUS ThreadStackListCreate (VOID)
{
  ThreadStackNodeList = AllocateZeroPool (sizeof (THREAD_STACK_NODE));
  if (!ThreadStackNodeList) {
    DEBUG ((EFI_D_ERROR, "Failed to Allocate memory for stack list \n"));
    return EFI_OUT_OF_RESOURCES;
  }

  list_initialize (&ThreadStackNodeList->Node);
  //No entry for head
  ThreadStackNodeList->ThreadStackEntry = NULL;
  return EFI_SUCCESS;
}

STATIC THREAD_STACK_NODE *GetStackTableByThread (Thread *CurrentThread)
{
  THREAD_STACK_NODE *ThreadStackNodeTmp = NULL, *ThreadStackNode = NULL;

  if (ThreadStackNodeList == NULL) {
    DEBUG ((EFI_D_ERROR, "getStackTableByThread ThreadStackNode not"
        "created\n"));
    return NULL;
  }

  if (CurrentThread == NULL) {
    DEBUG ((EFI_D_ERROR, "getStackTableByThread CurrentThread is NULL \n"));
    return NULL;
  }

  list_for_every_entry_safe (&(ThreadStackNodeList->Node), ThreadStackNode,
          ThreadStackNodeTmp, THREAD_STACK_NODE, Node) {
    if (ThreadStackNode->ThreadStackEntry &&
        (ThreadStackNode->ThreadStackEntry->Thread == CurrentThread)) {
      return ThreadStackNode;
    }
  }

  return NULL;
}

VOID ThreadStackNodeRemove (Thread *CurrentThread)
{
  THREAD_STACK_NODE *ThreadStackNodeTmp = NULL;

  if (IsThreadUSSLLSupported) {
    return;
  }

  if (CurrentThread == NULL) {
    DEBUG ((EFI_D_VERBOSE, "Remove NULL thread pointer, drop it\n"));
    return ;
  }

  ThreadStackNodeTmp = GetStackTableByThread (CurrentThread);
  if (!ThreadStackNodeTmp) {
    DEBUG ((EFI_D_VERBOSE, "try to remove a NULL node"));
    return ;
  }

  //Remove and clean current thread stack
  list_delete (&ThreadStackNodeTmp->Node);
  FreePool (ThreadStackNodeTmp->ThreadStackEntry->StackBottom);
  ThreadStackNodeTmp->ThreadStackEntry->StackBottom = NULL;
  ThreadStackNodeTmp->ThreadStackEntry->StackTop = NULL;
  ThreadStackNodeTmp->ThreadStackEntry->Thread = NULL;
  FreePool (ThreadStackNodeTmp->ThreadStackEntry);
  ThreadStackNodeTmp->ThreadStackEntry = NULL;
  FreePool (ThreadStackNodeTmp);
  ThreadStackNodeTmp = NULL;

  DEBUG ((EFI_D_VERBOSE, " remove CurrentThread = %r stack\n", CurrentThread));

  return;
}

STATIC EFI_STATUS __attribute__ ( (no_sanitize ("safe-stack")))
AllocateGlobalUnSafeStackPtr (VOID)
{
  EFI_STATUS Status = EFI_SUCCESS;

  UnSafeStackPtr = AllocateZeroPool (BOOT_LOADER_MAX_UNSAFE_STACK_SIZE);
  if (UnSafeStackPtr == NULL) {
    DEBUG ((EFI_D_ERROR, "Failed to Allocate memory for UnSafeStack \n"));
    Status = EFI_OUT_OF_RESOURCES;
    return Status;
  }

  UnSafeStackPtr += BOOT_LOADER_MAX_UNSAFE_STACK_SIZE;

  return Status;
}

/* Add stack and thread in list, then we can get stack by thread.
 */
EFI_STATUS __attribute__ ( (no_sanitize ("safe-stack")))
AllocateUnSafeStackPtr (Thread *CurrentThread)
{
  EFI_STATUS Status = EFI_SUCCESS;
  THREAD_STACK_NODE *ThreadStackNodeTmp = NULL;
  VOID* UnSafeStackPtr = NULL;
  ThrUnsafeStackIntf ThrUnsafeStackIntf = {
      NULL,
      BOOT_LOADER_MAX_UNSAFE_STACK_SIZE,
      ThreadStackReleaseCb,
      CurrentThread,
    };

  if (CurrentThread == NULL) {
    DEBUG ((EFI_D_ERROR, "Add NULL thread pointer, drop it\n"));
    return EFI_INVALID_PARAMETER;
  }

  if (IsThreadUSSLLSupported) {
    UnSafeStackPtr = AllocatePages (ALIGN_PAGES (
        BOOT_LOADER_MAX_UNSAFE_STACK_SIZE, ALIGNMENT_MASK_4KB));
    if (UnSafeStackPtr == NULL) {
      DEBUG ((EFI_D_ERROR, "Failed to Allocate memory for UnSafeStack \n"));
      Status = EFI_OUT_OF_RESOURCES;
      return Status;
    }

    DEBUG ((EFI_D_VERBOSE, "AllocateUnSafeStackPtr CurrentThread = 0x%x,"
        "UnSafeStackPtr = 0x%x with API \n", CurrentThread, UnSafeStackPtr));

    ThrUnsafeStackIntf.unsafe_sp_base = UnSafeStackPtr;

    Status = KernIntf->Thread->ThreadSetUnsafeSP (CurrentThread,
        &ThrUnsafeStackIntf);

    if (Status != EFI_SUCCESS) {
      DEBUG ((EFI_D_ERROR, "AllocateUnSafeStackPtr ThreadSetThreadUnsafeSP"
          "failed, Status = %d \n", Status));
    }

    return Status;
  }

  //Thread unsafe stack low level is not supported, set stack by link table
  DEBUG ((EFI_D_VERBOSE, "AllocateUnSafeStackPtr CurrentThread = 0x%x with link"
      "table \n", CurrentThread));
  if (ThreadStackNodeList == NULL) {
    DEBUG ((EFI_D_ERROR, "ThreadStackNodeAppend ThreadStackNode not"
        "created.\n"));
    return EFI_INVALID_PARAMETER;
  }
  ThreadStackNodeTmp = ThreadStackNodeInit (CurrentThread);
  if (!ThreadStackNodeTmp) {
    return EFI_OUT_OF_RESOURCES;
  }

  list_add_tail (&ThreadStackNodeList->Node, &ThreadStackNodeTmp->Node);

  return EFI_SUCCESS;
}

//This function is to return the Unsafestack ptr address
VOID** __attribute__ ( (no_sanitize ("safe-stack")))
__safestack_pointer_address (VOID)
{
  THREAD_STACK_NODE *ThreadStackNodeTmp = NULL;

  if (!IsMultiStack) {
      return (VOID**) &UnSafeStackPtr;
  }

  if (KernIntf == NULL) {
    return NULL;
  }

  if (IsThreadUSSLLSupported) {
    return KernIntf->Thread->ThreadGetUnsafeSPCurrent (
        KernIntf->Thread->GetCurrentThread ());
  }

  //Thread unsafe stack low level is not supported, get stack by link table
  ThreadStackNodeTmp =
      GetStackTableByThread (KernIntf->Thread->GetCurrentThread ());
  if (!ThreadStackNodeTmp ||
      !ThreadStackNodeTmp->ThreadStackEntry) {
    return (VOID**) &UnSafeStackPtr;
  }

  return (VOID**) &(ThreadStackNodeTmp->ThreadStackEntry->StackTop);
}

STATIC VOID EFIAPI __attribute__ ( (no_sanitize ("safe-stack")))
TimerStackHandler (IN EFI_EVENT Event, IN VOID *Context)
{
  TimerThread = KernIntf->Thread->GetCurrentThread ();
  DEBUG ((EFI_D_VERBOSE, "TimerStackHandler TimerThread =  %r\n", TimerThread));

  //Timer use one thread in abl, so only need to allocate unsafestack once.
  AllocateUnSafeStackPtr (TimerThread);

  return;
}

/* Close the timer and event */
VOID CloseStackTimer (VOID)
{
  /* Close the timer and event */
  if (TimerStackEvent) {
    gBS->SetTimer (TimerStackEvent, TimerCancel, 0);
    gBS->CloseEvent (TimerStackEvent);
    TimerStackEvent = NULL;
  }
  DEBUG ((EFI_D_VERBOSE, "Close the timer and event\n"));
}

/* Start timer stack allocate timer, it only runs one time to get timer thread
 * ID in TimerStackHandler.
 */
STATIC VOID
StartStackTimer (VOID)
{
  EFI_STATUS Status;

  Status = gBS->SetTimer (TimerStackEvent, TimerRelative, 500000);

  if (Status != EFI_SUCCESS) {
    DEBUG ((EFI_D_ERROR, "ERROR: Failed to start stack Timer: %r\n", Status));
    CloseStackTimer ();
  }
}

/**
  Create a event and timer to allocate timer stack as all timer share the same
  thread ID.
  @retval EFI_SUCCESS     The entry point is executed successfully.
  @retval other           Some error occurs when executing this entry point.
 **/
EFI_STATUS EFIAPI
TimerStackInit (VOID)
{
  EFI_STATUS Status = EFI_SUCCESS;

  Status = gBS->CreateEvent (EVT_TIMER | EVT_NOTIFY_SIGNAL, TPL_CALLBACK,
          TimerStackHandler, NULL, &TimerStackEvent);

  if (!EFI_ERROR (Status) &&
      TimerStackEvent) {
    StartStackTimer ();

    DEBUG ((EFI_D_VERBOSE, "Create TimerStackEvent: %r\n", Status));

  }
  return Status;
}

/**
  If IsThreadUSSLLSupported is true, UEFI core will call back here to free
  stack, if false, UEFI client link table use ThreadStackNodeRemove () to free
  stack.
 **/
VOID ThreadStackReleaseCb (VOID * Arg)
{
  VOID* UnSafeStackPtr = NULL;
  Thread *CurrentThread =  (Thread *)Arg;
  if (IsThreadUSSLLSupported) {
    UnSafeStackPtr = KernIntf->Thread->ThreadGetUnsafeSPBase (CurrentThread);
    DEBUG ((EFI_D_VERBOSE, "ThreadStackReleaseCb UnSafeStackPtr = 0x%x\n",
        UnSafeStackPtr));

    FreePages (UnSafeStackPtr, ALIGN_PAGES (
        BOOT_LOADER_MAX_UNSAFE_STACK_SIZE, ALIGNMENT_MASK_4KB));

    UnSafeStackPtr = NULL;
  }
}

/* 1. if EFI Kernel Protocol is not supported in UEFI core, allocate global
      stack for main and timer;
   2. If supported, but kernel version is smaller than
      EFI_KERNEL_PROTOCOL_VER_UNSAFE_STACK_APIS, alloctate by link table;
   3. If equal to or bigger, nothing need to to in ABL, UEFI core will manage
      main and timer thread stack;
 */
EFI_STATUS InitThreadUnsafeStack (VOID)
{
  EFI_STATUS Status = EFI_SUCCESS;

  Status = gBS->LocateProtocol (&gEfiKernelProtocolGuid, NULL,
        (VOID **)&KernIntf);

  if ((Status != EFI_SUCCESS) ||
      (KernIntf == NULL) ||
      KernIntf->Version < EFI_KERNEL_PROTOCOL_VER_THR_CPU_STATS) {
    DEBUG ((EFI_D_VERBOSE, "multi stack is not supported, using global"
          " single stack.\n"));

    IsMultiStack = FALSE;
    return AllocateGlobalUnSafeStackPtr ();
  }

  DEBUG ((EFI_D_VERBOSE, "SetThreadStackEnv () kernel version = 0x%x \n",
      KernIntf->Version));
  if (KernIntf->Version >= EFI_KERNEL_PROTOCOL_VER_UNSAFE_STACK_APIS) {
    IsThreadUSSLLSupported = TRUE;
  }

  if (!IsThreadUSSLLSupported) {
    //Allocate gloabl anyway, if some thread get null stack, return gloabal
    AllocateGlobalUnSafeStackPtr ();

    Status =  ThreadStackListCreate ();
    if (Status != EFI_SUCCESS) {
      DEBUG ((EFI_D_VERBOSE, "Unable to Init thread unsafe stack: %r.\n",
              Status));
      return Status;
    }

    Status = AllocateUnSafeStackPtr (KernIntf->Thread->GetCurrentThread ());
    if (Status != EFI_SUCCESS) {
      DEBUG ((EFI_D_ERROR, "Unable to Allocate memory for Unsafe Stack: %r\n",
                Status));
      return Status;
    }
  }
  return Status;
}


VOID DeInitThreadUnsafeStack (VOID)
{
  if (!IsThreadUSSLLSupported) {
    //Release timer stack
    CloseStackTimer ();
    ThreadStackNodeRemove (TimerThread);
  }
}
