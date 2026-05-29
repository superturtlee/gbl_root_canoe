/* Copyright (c) 2021, The Linux Foundation. All rights reserved.
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
 *
 * Changes from Qualcomm Technologies, Inc. are provided under the
 * following license:
 *
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#if HIBERNATION_SUPPORT_NO_AES

#define ___aligned(x) __attribute__((aligned(x)))
#define ___packed __attribute__((packed))
#define __AC(X, Y) (X##Y)
#define _AC(X, Y) __AC(X, Y)
#define PAGE_SHIFT 12
#define PAGE_SIZE (_AC(1, UL) << PAGE_SHIFT)
#define DIV_ROUND_UP(n, d) (((n) + (d) - 1) / (d))
#define HIBERNATE_SIG           "S1SUSPEND"
#define __NEW_UTS_LEN 64

/* Return True if integer overflow will occur */
#define CHECK_ADD64(a, b) ((MAX_UINT64 - b < a) ? TRUE : FALSE)
#define printf(...)     DEBUG((EFI_D_ERROR, __VA_ARGS__))

EFI_STATUS
PartitionGetInfo (IN CHAR16 *PartitionName,
                  OUT EFI_BLOCK_IO_PROTOCOL **BlockIo,
                  OUT EFI_HANDLE **Handle);
VOID JumpToKernel (VOID);
typedef VOID (*HIBERNATION_KERNEL)(VOID);
typedef UINT32 Crc;
typedef UINT64 SectorT;

#define AES256_KEY_SIZE         32
#define NUM_OF_AES256_KEYS      2
#define PAYLOAD_KEY_SIZE (NUM_OF_AES256_KEYS * AES256_KEY_SIZE)
#define RAND_INDEX_SIZE         8
#define NONCE_LENGTH            8
#define MAC_LENGTH              16
#define TIME_STRUCT_LENGTH      48
#define WRAP_PAYLOAD_LENGTH \
                (PAYLOAD_KEY_SIZE + RAND_INDEX_SIZE + TIME_STRUCT_LENGTH)
#define AAD_WITH_PAD_LENGTH 32
#define WRAPPED_KEY_SIZE \
               (AAD_WITH_PAD_LENGTH + WRAP_PAYLOAD_LENGTH + MAC_LENGTH + \
               NONCE_LENGTH)

typedef struct S4appTime {
       UINT16 Year;
       UINT8  Month;
       UINT8  Day;
       UINT8  Hour;
       UINT8  Minute;
}S4appTime;

typedef struct WrapReq {
       struct S4appTime SaveTime;
}WrapReq;

typedef struct WrapRsp {
       UINT8  WrappedKeyBuffer[WRAPPED_KEY_SIZE];
       UINT16 WrappedKeySize;
       UINT8  KeyBuffer[PAYLOAD_KEY_SIZE];
       UINT16 KeySize;
}WrapRsp;

typedef struct UnwrapReq {
       UINT8  WrappedKeyBuffer[WRAPPED_KEY_SIZE];
       UINT16 WrappedKeySize;
       struct S4appTime CurrTime;
}UnwrapReq;

typedef struct UnwrapRsp {
       UINT8 KeyBuffer[PAYLOAD_KEY_SIZE];
       UINT16 KeySize;
}UnwrapRsp;

enum CmdId {
       WRAP_KEY_CMD = 0,
       UNWRAP_KEY_CMD = 1,
};

typedef struct CmdReq {
       enum CmdId Cmd;
       union {
               struct WrapReq WrapkeyReq;
               struct UnwrapReq UnwrapkeyReq;
       };
}CmdReq;

typedef struct CmdRsp {
       enum CmdId Cmd;
       union {
               struct WrapRsp WrapkeyRsp;
               struct UnwrapRsp UnwrapkeyRsp;
       };
       UINT32 Status;
}CmdRsp;


typedef struct DecryptParam {
       UINT32 Authsize;
       UINT32 AuthCount;
       UINT8 KeyBlob[WRAPPED_KEY_SIZE];
       UINT8 Iv[12];
       UINT8 Aad[12];
}DecryptParam;

typedef struct SwsUspHeader {
       CHAR8 Reserved[PAGE_SIZE - 20 - sizeof (SectorT) - sizeof (INT32) -
               sizeof (Crc)];
       Crc Crc32;
       SectorT Image;
       UINT32 Flags;     /* Flags to pass to the "boot" kernel */
       UINT8 OrigSig[10];
       UINT8 Sig[10];
} ___packed SwsUspHeader;


typedef struct NewUtsname {
       char Sysname[__NEW_UTS_LEN + 1];
       char Nodename[__NEW_UTS_LEN + 1];
       char Release[__NEW_UTS_LEN + 1];
       char Version[__NEW_UTS_LEN + 1];
       char Machine[__NEW_UTS_LEN + 1];
       char Domainname[__NEW_UTS_LEN + 1];
}NewUtsname;

typedef struct SwsuspInfo {
       struct NewUtsname Uts;
       UINT32 RsionCode;
       UINT64 NumPhyspages;
       INT32  Cpus;
       UINT64 ImagePages;
       UINT64 Pages;
       UINT64 Size;
} ___aligned (PAGE_SIZE) SwsuspInfo;

typedef struct ArchHibernateHdrInvariants {
       char UtsVersion[__NEW_UTS_LEN + 1];
}ArchHibernateHdrInvariants;

typedef struct ArchHibernateHdr {
       struct ArchHibernateHdrInvariants Invariants;
       UINT64 Ttbr1El1;
       VOID   (*reenter_kernel)(VOID);
       UINT64 __hyp_stub_vectors;
       UINT64 SleepCpuMpidr;
       UINT64 PhysReEnterKernel;
}ArchHibernateHdr;

#endif
