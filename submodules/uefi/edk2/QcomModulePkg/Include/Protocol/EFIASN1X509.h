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

#ifndef _EFIASN1X509_H
#define _EFIASN1X509_H

#include <Uefi.h>

#define QCOM_ASN1X509_PROTOCOL_REVISION 0x0000000000010001
#define EFI_ASN1X509_PROTOCOL_GUID                                        \
{                                                                         \
    0x408ec123, 0x134e, 0x4b80,                                           \
    {  0x91, 0x42, 0xb8, 0x9a, 0xe, 0x08, 0xf8, 0xb3 }                    \
}

extern EFI_GUID gEfiQcomASN1X509ProtocolGuid;

typedef struct _QCOM_ASN1X509_PROTOCOL QcomAsn1x509Protocol;

#define MAX_LEN 4096
#define SIGNATURE_SHA256_LEN 256
#define E_ASN1_FAILURE 5
#define EFI_FAILURE (80)

/* Program defined basic ASN1 data types for better reading/understanding
   AndroidVerifiedBoot data types */
#define INTEGER struct secasn1_data_type
#define OBJECT_IDENTIFIER struct secasn1_data_type
#define CHARACTER_STRING struct secasn1_data_type
#define OCTET_STRING struct secasn1_data_type
#define BIT_STRING struct secasn1_data_type
#define SEQUENCE secasn1_data_type

/* ASN data tags that are not defined in SecBoot_asn1.h */
#define SECASN1_CHARACTER_STRING_TYPE (0x1D)
#define SECX509_ISSUER_ID_TAG (0x81)
#define SECX509_SUBJECT_ID_TAG (0x82)
#define SECX509_VERSION_TAG (0xA0)
#define SECX509_EXTENSION_TAG (0xA3)
#define SECASN1_GTIME_TYPE (0x24)

/**
 *    AndroidVerifiedBootSignature DEFINITIONS ::=
 *    BEGIN
 *        FormatVersion ::= INTEGER
 *        Certificate ::= Certificate
 *        AlgorithmIdentifier ::=  SEQUENCE {
 *            algorithm OBJECT IDENTIFIER,
 *            parameters ANY DEFINED BY algorithm OPTIONAL
 *        }
 *        AuthenticatedAttributes ::= SEQUENCE {
 *            target CHARACTER STRING,
 *            length INTEGER
 *        }
 *        Signature ::= OCTET STRING
 *     END
 **/
typedef struct verif_boot_sig_st {
    CONST UINT8 *formatversion;
    UINTN FormatVersionLen;
    CONST UINT8 *certificate;
    UINTN CertificateLen;
    CONST UINT8 *algorithm_id;
    UINTN AlgorithmIdLen;
    CONST UINT8 *auth_attr;
    UINTN AuthAttrLen;
    CONST UINT8 *sig;
    UINTN SigLen;
} VERIFIED_BOOT_SIG;

typedef struct rsa_st {
    CONST UINT8 *algorithm_id;
    CONST UINT8 *RSAPublicKey;
} RSA;

/**
*   Certificate  ::=  SEQUENCE {
*     tbsCertificate       TBSCertificate,
*     signatureAlgorithm   AlgorithmIdentifier,
*     signature            BIT STRING
*   }
**/
typedef struct certificate_st {
    CONST UINT8 *tbscertificate;
    UINTN TbsCertificateLen;
    CONST UINT8 *algorithmIdentifier;
    UINTN AlgorithmIdentifierLen;
    CONST UINT8 *signature;
    UINTN SignatureLen;
} CERTIFICATE;

/* ASN.1 data holder */
typedef struct secasn1_data_type
{
    CONST UINT8 *data;
    CONST UINT8 *data_bound;
    UINT32 Len;
} secasn1_data_type;

typedef EFI_STATUS (EFIAPI *QCOM_ASN1X509_GET_AUTHATTR_FIELDS)(
    IN QcomAsn1x509Protocol *This,
    IN CONST UINT8 *data,
    IN UINTN Len,
    OUT secasn1_data_type *target,
    OUT secasn1_data_type *length);

typedef EFI_STATUS (EFIAPI *QCOM_ASN1X509_GET_RSA_FROM_CERTIFICATE)(
    IN QcomAsn1x509Protocol *This,
    IN CERTIFICATE *certificate,
    OUT RSA *out_key);

typedef EFI_STATUS (EFIAPI *QCOM_ASN1X509_GET_RSA_FROM_VB_SIGNATURE)(
    IN QcomAsn1x509Protocol *This,
    IN VERIFIED_BOOT_SIG *vb_signature,
    OUT RSA *out_key);

typedef EFI_STATUS (EFIAPI *QCOM_ASN1X509_GET_SIG_FROM_VB_SIGNATURE)(
    IN QcomAsn1x509Protocol *This,
    IN VERIFIED_BOOT_SIG *vbsignature,
    OUT secasn1_data_type *signature);

typedef EFI_STATUS (EFIAPI *QCOM_ASN1X509_GET_KEYMATERIAL_FROM_RSA)(
    IN QcomAsn1x509Protocol *This,
    IN RSA *key,
    OUT secasn1_data_type *out_modulus,
    OUT secasn1_data_type *out_public_exp);

typedef EFI_STATUS (EFIAPI *QCOM_ASN1X509_VERIFY_VB_SIGNATURE)(
    IN QcomAsn1x509Protocol *This,
    IN CONST UINT8 *data,
    IN UINTN Len,
    OUT VERIFIED_BOOT_SIG *out_vb_signaure);

typedef EFI_STATUS (EFIAPI *QCOM_ASN1X509_VERIFY_OEM_CERTIFICATE)(
    IN QcomAsn1x509Protocol *This,
    IN CONST UINT8 *data,
    IN UINTN Len,
    OUT CERTIFICATE *certificate);

typedef struct _QCOM_ASN1X509_PROTOCOL {
    UINT64 Revision;
    QCOM_ASN1X509_GET_AUTHATTR_FIELDS ASN1X509GetAuthAttrFields;
    QCOM_ASN1X509_GET_KEYMATERIAL_FROM_RSA ASN1X509GetKeymaterial;
    QCOM_ASN1X509_GET_RSA_FROM_CERTIFICATE ASN1X509GetRSAFromCert;
    QCOM_ASN1X509_GET_RSA_FROM_VB_SIGNATURE ASN1X509GetRSAFromVBSig;
    QCOM_ASN1X509_GET_SIG_FROM_VB_SIGNATURE ASN1X509GetSigFromVBSig;
    QCOM_ASN1X509_VERIFY_OEM_CERTIFICATE ASN1X509VerifyOEMCertificate;
    QCOM_ASN1X509_VERIFY_VB_SIGNATURE ASN1X509VerifyVBSig;
}QcomAsn1x509Protocol;

#endif
