/* Copyright (c) 2016-2018, The Linux Foundation. All rights reserved.
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

#ifndef __EFISECRSA_H__
#define __EFISECRSA_H__

/*===========================================================================
  INCLUDE FILES
===========================================================================*/
#include <Uefi.h>

/*===========================================================================
  MACRO DECLARATIONS
===========================================================================*/
/** @ingroup
  Protocol version.
*/
#define QCOM_SECRSA_PROTOCOL_REVISION 0x0000000000010001
/*  Protocol GUID definition */
/** @ingroup efi_secrsa_protocol */
#define EFI_SECRSA_PROTOCOL_GUID                                           \
{                                                                          \
    0xeb7a442a, 0xcef4, 0x40a7,                                            \
    { 0xa8, 0xae, 0x5a, 0x2e, 0x7f, 0x48, 0xe1, 0x72 }                     \
}

typedef UINT32 Blong;
#define RSA_MAX_KEY_SIZE 4128   //redefinition in secboot_types.h
 ///< Maximum key size in bits
#define BLONG_SIZE sizeof(Blong)                       ///< Bytes per digit
#define BN_BITS_PER_WORD (BLONG_SIZE * 8)              ///< Bit per digit
#define BLONGS_PER_KEY (RSA_MAX_KEY_SIZE + 8 * BLONG_SIZE -1) / (8 * BLONG_SIZE)
///< Digits per key
#define BYTES_TO_BLONG_SHIFT 2                         ///< bytes/sizeof(BLONG)
#define BLONG_MAX_ELEMENTS BLONGS_PER_KEY
#define NUMBER_OF_RANDOM_WORDS 3
// n. of 32 bit words for the random values 96 bits /32 (size of bit word);
// // number of random 32 bit words
#define WORD_SIZE_32 BLONG_SIZE
#define MASKED_LENGTH (WORD_SIZE_32 * NUMBER_OF_RANDOM_WORDS)
// 96 bits (masked by multiplication)
#define MASKED_MAX_ELEMENT (BLONG_MAX_ELEMENTS + MASKED_LENGTH)
// ELEMENT:= message or key

typedef struct {
    Blong Akey[BLONGS_PER_KEY]; ///< Make room for multiplication
    INT32 Nlen;                 ///< Current length of a
} BigInt;

/*RSA key data structure with sign.
  sign is needed when the second form of private key
  is provided */
typedef struct S_BIGINT {
    BigInt Bi;
    INT32 Sign;
} S_BIGINT;

/*RSA padding type. PKCS #1 v2.1*/
typedef enum {
    /*PKCS1 v1.5 signature*/
    CE_RSA_PAD_PKCS1_V1_5_SIG = 1,
    /*PKCS1 v1.5 encryption*/
    CE_RSA_PAD_PKCS1_V1_5_ENC = 2,
    /*OAEP Encryption*/
    CE_RSA_PAD_PKCS1_OAEP = 3,
    /*PSS Signature*/
    CE_RSA_PAD_PKCS1_PSS = 4,
    /* No Padding */
    CE_RSA_NO_PAD = 5,
    /*PSS with unknown saltlen*/
    CE_RSA_PAD_PKCS1_PSS_AUTORECOVER_SALTLEN = 6,
    CE_RSA_PAD_INVALID = 0x7FFFFFFF,
} CE_RSA_PADDING_TYPE;

/** RSA PKCS key */
typedef struct CE_RSA_KEY {
    /** Type of key, CE_RSA_PRIVATE or CE_RSA_PUBLIC */
    INT32 Type;
    /** RSA key bit length **/
    INT32 BitLength;
    /** The public exponent */
    S_BIGINT *e;
    /** The private exponent */
    S_BIGINT *d;
    /** The modulus */
    S_BIGINT *N;
    /** The p factor of N */
    S_BIGINT *p;
    /** The q factor of N */
    S_BIGINT *q;
    /** The 1/q mod p CRT param */
    S_BIGINT *qP;
    /** The d mod (p - 1) CRT param */
    S_BIGINT *dP;
    /** The d mod (q - 1) CRT param */
    S_BIGINT *dQ;
} CE_RSA_KEY;

/* error codes */
typedef enum {
    E_SECMATH_SUCCESS = 0,        ///< Result OK
    E_SECMATH_FAILURE,            ///< Generic Error */
    E_SECMATH_NOP,                ///< Not a failure but no operation performed
    E_SECMATH_FAIL_TESTVECTOR,    ///< Algorithm failed test vectors */
    E_SECMATH_BUFFER_OVERFLOW,    ///< Not enough space for output */
    E_SECMATH_MEM,                ///< Out of memory */
    E_SECMATH_INVALID_ARG,        ///< Generic invalid argument */
    E_SECMATH_INVALID_PRIME_SIZE, ///< Invalid size of prime requested */
    E_SECMATH_NOT_SUPPORTED       ///< Operation not supported
} SECMATH_ERRNO_ET;

/*index of hash algorithm used for generating signature */
typedef enum {
    CE_HASH_IDX_NULL = 0x1,
    CE_HASH_IDX_SHA1 = 0x2,
    CE_HASH_IDX_SHA256 = 0x3,
    CE_HASH_IDX_SHA224 = 0x4,
    CE_HASH_IDX_SHA384 = 0x5,
    CE_HASH_IDX_SHA512 = 0x6,
    CE_HASH_IDX_SHA256_SHA1 = 0x7,
    CE_HASH_IDX_MAX = 0x8,
    CE_HASH_IDX_INVALID = 0x7FFFFFFF
} CE_HASH_IDX;

typedef struct CE_RSA_PSS_PAD_INFO {
  CE_HASH_IDX hashidx;
  UINT32 saltlen;
} CE_RSA_PSS_PAD_INFO;

/*sign*/
#define S_BIGINT_POS 0 /* positive */
#define S_BIGINT_NEG 1 /* negative */

/* equalities */
#define S_BIGINT_LT -1 /* less than */
#define S_BIGINT_EQ 0  /* equal to */
#define S_BIGINT_GT 1  /* greater than */

typedef enum {
    /* Result OK */
    CE_SUCCESS = E_SECMATH_SUCCESS,
    /* Generic Error */
    CE_ERROR_FAILURE = E_SECMATH_FAILURE,
    /* Not a failure but no operation was performed */
    CE_ERROR_NOP = E_SECMATH_NOP,
    /* Algorithm failed test vectors */
    CE_ERROR_FAIL_TESTVECTOR = E_SECMATH_FAIL_TESTVECTOR,
    /* Not enough space for output */
    CE_ERROR_BUFFER_OVERFLOW = E_SECMATH_BUFFER_OVERFLOW,
    /* Out of memory */
    CE_ERROR_NO_MEMORY = E_SECMATH_MEM,
    /* Generic invalid argument */
    CE_ERROR_INVALID_ARG = E_SECMATH_INVALID_ARG,
    /* Invalid size of prime requested */
    CE_ERROR_INVALID_SIZE = E_SECMATH_INVALID_PRIME_SIZE,
    /* Operation not supported */
    CE_ERROR_NOT_SUPPORTED = E_SECMATH_NOT_SUPPORTED,
    /* invalid signature  */
    CE_ERROR_INVALID_SIGNATURE = 0x1000,
    /* decrypt error  */
    CE_ERROR_DECRYPT_ERROR = 0x1002,
    /* invalid packet  */
    CE_ERROR_INVALID_PACKET = 0x1003,
    /* message too long  */
    CE_ERROR_MESSAGE_TOO_LONG = 0x1004,
} CeErrorType;

/*CE RSA Key Type*/
typedef enum {
    CE_RSA_KEY_PUBLIC = 0,             /*public key*/
    CE_RSA_KEY_PRIVATE = 1,            /*private key in non CRT representation*/
    CE_RSA_KEY_PRIVATE_CRT = 2,        /*private key in CRT representation*/
    CE_RSA_KEY_PRIVATE_PUBLIC = 3,     /*private/public key pair*/
    CE_RSA_KEY_PRIVATE_CRT_PUBLIC = 4, /*private CRT/public key pair*/
    CE_RSA_KEY_INVALID = 0x7FFFFFFF
} CE_RSA_KEY_TYPE;

/*===========================================================================
  EXTERNAL VARIABLES
===========================================================================*/
/** @ingroup */
extern EFI_GUID gEfiQcomSecRSAProtocolGuid;

/*===========================================================================
  TYPE DEFINITIONS
===========================================================================*/
/** @cond */
/* Protocol declaration.  */
typedef struct _QCOM_SECRSA_PROTOCOL QcomSecRsaProtocol;
/** @endcond */

/*===========================================================================
  FUNCTION DEFINITIONS
===========================================================================*/
typedef EFI_STATUS (EFIAPI *QCOM_SECRSA_BIGINT_READ_BIN)(
    IN QcomSecRsaProtocol *This,
    IN CONST UINT8 *buf,
    IN UINT32 BufLen,
    OUT BigInt *bigint_buf);

typedef EFI_STATUS (EFIAPI *QCOM_SECRSA_VERIFY_SIGNATURE)(
    IN QcomSecRsaProtocol *This,
    IN CE_RSA_KEY *key,
    IN CE_RSA_PADDING_TYPE PaddingType,
    IN VOID *padding_info,
    IN INT32 HashIdx,
    IN UINT8 *hash,
    IN INT32 HashLen,
    IN CONST UINT8 *signature_ptr,
    IN UINT32 SignatureLen);
/*====================================================================
  PROTOCOL INTERFACE
===========================================================================*/
/** @ingroup
  @par Summary
    SECRSA Protocol interface.

  @par Parameters
  @inputprotoparams
*/
typedef struct _QCOM_SECRSA_PROTOCOL {
    UINT64 Revision;
    QCOM_SECRSA_BIGINT_READ_BIN SecRSABigIntReadBin;
    QCOM_SECRSA_VERIFY_SIGNATURE SecRSAVerifySig;
}QcomSecRsaProtocol;

#endif /* __EFIVERIFIEDBOOT_H__ */
