/*
 * Copyright (c) 2015-2017, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of The Linux Foundation nor
 *       the names of its contributors may be used to endorse or promote
 *       products derived from this software without specific prior written
 *       permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef _META_FORMAT_H_
#define _META_FORMAT_H_

#define META_HEADER_MAGIC 0xce1ad63c
#define META_PARTITION_NAME_SZ 72
/*Keeping the maximum number of images supported to 32 for future expansion*/
/*Refer device/qcom/common/meta_image/meta_image.c file for reference*/
#define MAX_IMAGES_IN_METAIMG 32

typedef struct meta_header {
  UINT32 magic;          /* 0xce1ad63c */
  UINT16 major_version;  /* (0x1) - reject images with higher major versions */
  UINT16 minor_version;  /* (0x0) - allow images with higer minor versions */
  CHAR8 img_version[64]; /* Top level version for images in this meta */
  UINT16 meta_hdr_sz;    /* size of this header */
  UINT16 img_hdr_sz;     /* size of img_header_entry list */
} meta_header_t;

typedef struct img_header_entry {
  CHAR8 ptn_name[META_PARTITION_NAME_SZ];
  UINT32 start_offset;
  UINT32 size;
} img_header_entry_t;

#endif
