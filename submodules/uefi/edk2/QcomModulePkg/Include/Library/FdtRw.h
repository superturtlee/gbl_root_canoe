/*
 * Copyright (c) 2020-2021, The Linux Foundation. All rights reserved.
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

#ifndef __FDT_RW_H
#define __FDT_RW_H

typedef struct _FDT_FIRST_LEVEL_NODE {
  CONST CHAR8 *NodeName;
  INT32 NodeOffset;
  struct _FDT_FIRST_LEVEL_NODE *Next;
} FDT_FIRST_LEVEL_NODE;

INT32 FdtPathOffset (CONST VOID *Fdt, CONST CHAR8 *Path);
INT32 FdtGetPropLen (VOID *Fdt, INT32 Offset, CONST CHAR8 *Name);
VOID FdtUpdateNodeOffsetInList (INT32 NodeOffset, INT32 DiffLen);
INT32 FdtSetProp (VOID *Fdt, INT32 Offset, CONST CHAR8 *Name,
                    CONST VOID *Val, INT32 Len);
INT32 FdtAddSubnode (VOID *Fdt, INT32 ParentOffset, CONST CHAR8 *Name);

#define FDT_ALIGN(x, a) (((x) + (a)-1) & ~((a)-1))
#define FDT_TAGALIGN(x) (FDT_ALIGN((x), FDT_TAGSIZE))

#define FdtPropUpdateFunc(Fdt, Offset, Name, Val, FdtUpdateFunc, RetValue) \
  do {                                                                     \
    if (FixedPcdGetBool (EnableNewNodeSearchFuc)) {                        \
      INT32 OldLen = FdtGetPropLen (Fdt, Offset, Name);                    \
      RetValue = FdtUpdateFunc (Fdt, Offset, Name, Val);                   \
      if (RetValue == 0) {                                                 \
        INT32 NewLen = FdtGetPropLen (Fdt, Offset, Name);                  \
        if (OldLen == 0 &&                                                 \
            NewLen) {                                                      \
            NewLen = sizeof (struct fdt_property) + FDT_TAGALIGN (NewLen); \
        }                                                                  \
        /* Update the node's offset in the list */                         \
        FdtUpdateNodeOffsetInList (                                        \
           Offset, FDT_TAGALIGN (NewLen) - FDT_TAGALIGN (OldLen));         \
     }                                                                     \
    } else {                                                               \
      RetValue = FdtUpdateFunc (Fdt, Offset, Name, Val);                   \
    }                                                                      \
  } while (0)
#endif
