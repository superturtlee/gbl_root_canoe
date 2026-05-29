/*
 * Copyright (C) 2008 The Android Open Source Project
 * All rights reserved.
 *
 * Copyright (c) 2014-2019, 2021 The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _BOOT_IMAGE_H_
#define _BOOT_IMAGE_H_

#define BOOT_MAGIC "ANDROID!"
#define BOOT_MAGIC_SIZE 8
#define BOOT_NAME_SIZE 16
#define BOOT_ARGS_SIZE 512
#define BOOT_IMG_MAX_PAGE_SIZE 4096
#define KERNEL64_HDR_MAGIC 0x644D5241 /* ARM64 */
#define BOOT_EXTRA_ARGS_SIZE 1024

#define VENDOR_BOOT_MAGIC "VNDRBOOT"
#define VENDOR_BOOT_MAGIC_SIZE 8
#define VENDOR_BOOT_ARGS_SIZE 2048
#define VENDOR_BOOT_NAME_SIZE 16

#define BOOT_HEADER_VERSION_ZERO 0

#define VENDOR_RAMDISK_TYPE_NONE 0
#define VENDOR_RAMDISK_TYPE_PLATFORM 1
#define VENDOR_RAMDISK_TYPE_RECOVERY 2
#define VENDOR_RAMDISK_TYPE_DLKM 3
#define VENDOR_RAMDISK_NAME_SIZE 32
#define VENDOR_RAMDISK_TABLE_ENTRY_BOARD_ID_SIZE 16

/* Struct def for boot image header
 * Bootloader expects the structure of boot_img_hdr with header version
 *  BOOT_HEADER_VERSION_ZERO to be as follows:
 */
struct boot_img_hdr_v0 {
  CHAR8 magic[BOOT_MAGIC_SIZE];

  UINT32 kernel_size; /* size in bytes */
  UINT32 kernel_addr; /* physical load addr */

  UINT32 ramdisk_size; /* size in bytes */
  UINT32 ramdisk_addr; /* physical load addr */

  UINT32 second_size; /* size in bytes */
  UINT32 second_addr; /* physical load addr */

  UINT32 tags_addr;  /* physical addr for kernel tags */
  UINT32 page_size;  /* flash page size we assume */
  UINT32 header_version; /* version for the boot image header */
  UINT32 os_version; /* version << 11 | patch_level */

  UINT8 name[BOOT_NAME_SIZE]; /* asciiz product name */

  UINT8 cmdline[BOOT_ARGS_SIZE];

  UINT32 id[8]; /* timestamp / checksum / sha1 / etc */

  /* Supplemental command line data; kept here to maintain
   * binary compatibility with older versions of mkbootimg
   */
  UINT8 extra_cmdline[BOOT_EXTRA_ARGS_SIZE];
} __attribute__((packed));

/*
 * It is expected that callers would explicitly specify which version of the
 * boot image header they need to use.
 */
typedef struct boot_img_hdr_v0 boot_img_hdr;

/**
 * Offset of recovery DTBO length in a boot image header of version V1 or
 * above.
 */
#define BOOT_IMAGE_HEADER_V1_RECOVERY_DTBO_SIZE_OFFSET sizeof (boot_img_hdr)

/*
 * ** +-----------------+
 * ** | boot header     | 1 page
 * ** +-----------------+
 * ** | kernel          | n pages
 * ** +-----------------+
 * ** | ramdisk         | m pages
 * ** +-----------------+
 * ** | second stage    | o pages
 * ** +-----------------+
 * ** n = (kernel_size + page_size - 1) / page_size
 * ** m = (ramdisk_size + page_size - 1) / page_size
 * ** o = (second_size + page_size - 1) / page_size
 * ** 0. all entities are page_size aligned in flash
 * ** 1. kernel and ramdisk are required (size != 0)
 * ** 2. second is optional (second_size == 0 -> no second)
 * ** 3. load each element (kernel, ramdisk, second) at
 * **    the specified physical address (kernel_addr, etc)
 * ** 4. prepare tags at tag_addr.  kernel_args[] is
 * **    appended to the kernel commandline in the tags.
 * ** 5. r0 = 0, r1 = MACHINE_TYPE, r2 = tags_addr
 * ** 6. if second_size != 0: jump to second_addr
 * **    else: jump to kernel_addr
 * */

#define BOOT_HEADER_VERSION_ONE 1

struct boot_img_hdr_v1 {
  UINT32 recovery_dtbo_size;   /* size in bytes for recovery DTBO image */
  UINT64 recovery_dtbo_offset; /* physical load addr */
  UINT32 header_size;
} __attribute__((packed));

/* When the boot image header has a version of BOOT_HEADER_VERSION_ONE,
 * the structure of the boot image is as follows:
 *
 * +-----------------+
 * | boot header     | 1 page
 * +-----------------+
 * | kernel          | n pages
 * +-----------------+
 * | ramdisk         | m pages
 * +-----------------+
 * | second stage    | o pages
 * +-----------------+
 * | recovery dtbo   | p pages
 * +-----------------+
 * n = (kernel_size + page_size - 1) / page_size
 * m = (ramdisk_size + page_size - 1) / page_size
 * o = (second_size + page_size - 1) / page_size
 * p = (recovery_dtbo_size + page_size - 1) / page_size
 *
 * 0. all entities are page_size aligned in flash
 * 1. kernel and ramdisk are required (size != 0)
 * 2. recovery_dtbo is required for recovery.img
 *    in non-A/B devices(recovery_dtbo_size != 0)
 * 3. second is optional (second_size == 0 -> no second)
 * 4. load each element (kernel, ramdisk, second, recovery_dtbo) at
 *    the specified physical address (kernel_addr, etc)
 * 5. prepare tags at tag_addr.  kernel_args[] is
 *    appended to the kernel commandline in the tags.
 * 6. r0 = 0, r1 = MACHINE_TYPE, r2 = tags_addr
 * 7. if second_size != 0: jump to second_addr
 *    else: jump to kernel_addr
 */

#define BOOT_IMAGE_HEADER_V2_OFFSET sizeof (struct boot_img_hdr_v1)
#define BOOT_HEADER_VERSION_TWO 2

struct boot_img_hdr_v2 {
  UINT32 dtb_size; /* size in bytes for DTB image */
  UINT64 dtb_addr; /* physical load address for DTB image */
} __attribute__((packed));

/* When the boot image header has a version of BOOT_HEADER_VERSION_TWO,
 * the structure of the boot image is as follows:
 *
 * +-----------------+
 * | boot header     | 1 page
 * +-----------------+
 * | kernel          | n pages
 * +-----------------+
 * | ramdisk         | m pages
 * +-----------------+
 * | second stage    | o pages
 * +-----------------+
 * | recovery dtbo   | p pages
 * +-----------------+
 * | dtb.img         | q pages
 * +-----------------+
 *
 * n = (kernel_size + page_size - 1) / page_size
 * m = (ramdisk_size + page_size - 1) / page_size
 * o = (second_size + page_size - 1) / page_size
 * p = (recovery_dtbo_size + page_size - 1) / page_size
 * q = (dtb_size + page_size - 1) / page_size
 *
 * 0. all entities are page_size aligned in flash
 * 1. kernel and ramdisk are required (size != 0)
 * 2. recovery_dtbo is required for recovery.img (recovery_dtbo_size != 0)
 * 3. second is optional (second_size == 0 -> no second)
 * 4. dtb.img has all the dtbs catted one after the other
 * 5. load each element (kernel, ramdisk, second, recovery_dtbo) at
 *    the specified physical address (kernel_addr, etc)
 * 6. prepare tags at tag_addr.  kernel_args[] is
 *    appended to the kernel commandline in the tags.
 * 7. r0 = 0, r1 = MACHINE_TYPE, r2 = tags_addr
 * 8. if second_size != 0: jump to second_addr
 *    else: jump to kernel_addr
 */

#define BOOT_HEADER_VERSION_THREE 3

/* When the boot image header has a version of 3, the structure of the boot
 * image is as follows:
 *
 * +---------------------+
 * | boot header         | 1 page
 * +---------------------+
 * | kernel              | m pages
 * +---------------------+
 * | ramdisk             | n pages
 * +---------------------+
 * m = (kernel_size + page_size - 1) / page_size
 * n = (ramdisk_size + page_size - 1) / page_size
 *
 * and the structure of the vendor boot image (introduced with version 3) is as
 * follows:
 *
 * +---------------------+
 * | vendor boot header  | 1 page
 * +---------------------+
 * | vendor ramdisk      | o pages
 * +---------------------+
 * | dtb                 | p pages
 * +---------------------+
 * o = (vendor_ramdisk_size + page_size - 1) / page_size
 * p = (dtb_size + page_size - 1) / page_size
 *
 * 0. all entities are page_size aligned in flash
 * 1. kernel, ramdisk, vendor ramdisk, and DTB are required (size != 0)
 * 2. load the kernel and DTB at the specified physical address (kernel_addr,
 *    dtb_addr)
 * 3. load the vendor ramdisk at ramdisk_addr
 * 4. load the generic ramdisk immediately following the vendor ramdisk in
 *    memory
 * 5. prepare tags at tag_addr.  kernel_args[] is appended to the kernel
 *    commandline in the tags.
 * 6. r0 = 0, r1 = MACHINE_TYPE, r2 = tags_addr
 * 7. if the platform has a second stage bootloader jump to it (must be
 *    contained outside boot and vendor boot partitions), otherwise
 *    jump to kernel_addr
 */
struct boot_img_hdr_v3 {
  // Must be BOOT_MAGIC.
  UINT8 magic[BOOT_MAGIC_SIZE];

  UINT32 kernel_size; /* size in bytes */
  UINT32 ramdisk_size; /* size in bytes */

  // Operating system version and security patch level.
  // For version "A.B.C" and patch level "Y-M-D":
  //   (7 bits for each of A, B, C; 7 bits for (Y-2000), 4 bits for M)
  //   os_version = A[31:25] B[24:18] C[17:11] (Y-2000)[10:4] M[3:0]
  UINT32 os_version;

  UINT32 header_size;

  UINT32 reserved[4];

  // Version of the boot image header.
  UINT32 header_version;

  UINT8 cmdline[BOOT_ARGS_SIZE + BOOT_EXTRA_ARGS_SIZE];
} __attribute__((packed));

typedef struct boot_img_hdr_v3 boot_img_hdr_v3;

struct vendor_boot_img_hdr_v3 {
  // Must be VENDOR_BOOT_MAGIC.
  UINT8 magic[VENDOR_BOOT_MAGIC_SIZE];

  // Version of the vendor boot image header.
  UINT32 header_version;

  UINT32 page_size; /* flash page size we assume */

  UINT32 kernel_addr; /* physical load addr */
  UINT32 ramdisk_addr; /* physical load addr */

  UINT32 vendor_ramdisk_size; /* size in bytes */

  UINT8 cmdline[VENDOR_BOOT_ARGS_SIZE];

  UINT32 tags_addr; /* physical addr for kernel tags */
  UINT8 name[VENDOR_BOOT_NAME_SIZE]; /* asciiz product name */

  UINT32 header_size;

  UINT32 dtb_size; /* size in bytes for DTB image */
  UINT64 dtb_addr; /* physical load address for DTB image */
} __attribute__((packed));

typedef struct vendor_boot_img_hdr_v3 vendor_boot_img_hdr_v3;
/* When the boot image header has a version of 4, the structure of the boot
 * image is as follows:
 *
 * +---------------------+
 * | boot header         | 4096 bytes
 * +---------------------+
 * | kernel              | m pages
 * +---------------------+
 * | boot signature      | g pages
 * +---------------------+
 *
 * m = (kernel_size + 4096 - 1) / 4096
 * g = (signature_size + 4096 - 1) / 4096
 *
 * The generic ramdisk is no longer be contained in the
 * boot image, the generic ramdisk will be placed in a new init_boot partition
 * Structure for the new init_boot partition:
 *
 *
 * +---------------------+
 * | boot header         | 4096 bytes
 * +---------------------+
 * | ramdisk             | n pages
 * +---------------------+
 * | boot signature      | g pages
 * +---------------------+
 *
 * n = (ramdisk_size + 4096 - 1) / 4096
 * g = (signature_size + 4096 - 1) / 4096
 *
 * Note that in version 4 of the boot image header, page size is fixed at 4096
 * bytes.
 *
 * The structure of the vendor boot image version 4, which is required to be
 * present when a version 4 boot image is used, is as follows:
 *
 * +------------------------+
 * | vendor boot header     | o pages
 * +------------------------+
 * | vendor ramdisk section | p pages
 * +------------------------+
 * | dtb                    | q pages
 * +------------------------+
 * | vendor ramdisk table   | r pages
 * +------------------------+
 * | bootconfig             | s pages
 * +------------------------+
 *
 * o = (2128 + page_size - 1) / page_size
 * p = (vendor_ramdisk_size + page_size - 1) / page_size
 * q = (dtb_size + page_size - 1) / page_size
 * r = (VendorRamdiskTableSize + page_size - 1) / page_size
 * s = (VendorBootconfigSize + page_size - 1) / page_size
 *
 * Note that in version 4 of the vendor boot image, multiple vendor ramdisks can
 * be included in the vendor boot image. The bootloader can select a subset of
 * ramdisks to load at runtime. To help the bootloader select the ramdisks, each
 * ramdisk is tagged with a type tag and a set of hardware identifiers
 * describing the board, soc or platform that this ramdisk is intended for.
 *
 * The vendor ramdisk section is consist of multiple ramdisk images concatenated
 * one after another, and vendor_ramdisk_size is the size of the section, which
 * is the total size of all the ramdisks included in the vendor boot image.
 *
 * The vendor ramdisk table holds the size, offset, type, name and hardware
 * identifiers of each ramdisk. The type field denotes the type of its content.
 * The hardware identifiers are specified in the board_id field in each table
 * entry. The board_id field is consist of a vector of unsigned integer words,
 * and the encoding scheme is defined by the hardware vendor.
 *
 * For the different type of ramdisks, there are:
 *    - VENDOR_RAMDISK_TYPE_NONE indicates the value is unspecified.
 *    - VENDOR_RAMDISK_TYPE_PLATFORM ramdisk contains platform specific bits.
 *    - VENDOR_RAMDISK_TYPE_RECOVERY ramdisk contains recovery resources.
 *    - VENDOR_RAMDISK_TYPE_DLKM ramdisk contains dynamic loadable kernel
 *      modules.
 *
 * Version 4 of the vendor boot image also adds a bootconfig section to the end
 * of the image. This section contains Boot Configuration parameters known at
 * build time. The bootloader is responsible for placing this section directly
 * after the generic ramdisk, followed by the bootconfig trailer, before
 * entering the kernel.
 *
 * 0. all entities in the boot image are 4096-byte aligned in flash, all
 *    entities in the vendor boot image are page_size (determined by the vendor
 *    and specified in the vendor boot image header) aligned in flash
 * 1. kernel, ramdisk, and DTB are required (size != 0)
 * 2. load the kernel and DTB at the specified physical address (kernel_addr,
 *    dtb_addr)
 * 3. load the vendor ramdisks at ramdisk_addr
 * 4. load the generic ramdisk immediately following the vendor ramdisk in
 *    memory
 * 5. load the bootconfig immediately following the generic ramdisk. Add
 *    additional bootconfig parameters followed by the bootconfig trailer.
 * 6. set up registers for kernel entry as required by your architecture
 * 7. if the platform has a second stage bootloader jump to it (must be
 *    contained outside boot and vendor boot partitions), otherwise
 *    jump to kernel_addr
 */

#define BOOT_HEADER_VERSION_FOUR 4

struct boot_img_hdr_v4 {
  // Must be BOOT_MAGIC.
  UINT8 magic[BOOT_MAGIC_SIZE];

  UINT32 kernel_size; /* size in bytes */
  UINT32 ramdisk_size; /* size in bytes */

  // Operating system version and security patch level.
  // For version "A.B.C" and patch level "Y-M-D":
  //   (7 bits for each of A, B, C; 7 bits for (Y-2000), 4 bits for M)
  //   os_version = A[31:25] B[24:18] C[17:11] (Y-2000)[10:4] M[3:0]
  UINT32 os_version;

  UINT32 header_size;

  UINT32 reserved[4];

  // Version of the boot image header.
  UINT32 header_version;

  UINT8 cmdline[BOOT_ARGS_SIZE + BOOT_EXTRA_ARGS_SIZE];
  UINT32 signature_size;
} __attribute__( (packed) );
typedef struct boot_img_hdr_v4 boot_img_hdr_v4;

struct vendor_boot_img_hdr_v4
{
  UINT8 magic[VENDOR_BOOT_MAGIC_SIZE];
  UINT32 header_version;
  UINT32 page_size;           /* flash page size we assume */

  UINT32 kernel_addr;         /* physical load addr */
  UINT32 ramdisk_addr;        /* physical load addr */

  UINT32 vendor_ramdisk_size; /* size in bytes */

  UINT8 cmdline[VENDOR_BOOT_ARGS_SIZE];

  UINT32 tags_addr;           /* physical addr for kernel tags */

  UINT8 name[VENDOR_BOOT_NAME_SIZE]; /* asciiz product name */

  /* size of vendor boot image header in bytes */
  UINT32 header_size;
  UINT32 dtb_size;            /* size of dtb image */
  UINT64 dtb_addr;            /* physical load address */

  /* size in bytes for the vendor ramdisk table */
  UINT32 VendorRamdiskTableSize;
  /* number of entries in the vendor ramdisk table */
  UINT32 VendorRamdiskTableEntryNums;
  /* size in bytes for a vendor ramdisk table entry */
  UINT32 VendorRamdiskTableEntrySize;
  UINT32 VendorBootconfigSize;
};

typedef struct vendor_boot_img_hdr_v4 vendor_boot_img_hdr_v4;

struct vendor_ramdisk_table_entry_v4
{
  UINT32 ramdisk_size; /* size in bytes for the ramdisk image */
  /* offset to the ramdisk image in vendor ramdisk section */
  UINT32 ramdisk_offset;
  UINT32 ramdisk_type; /* type of the ramdisk */
  UINT8 ramdisk_name[VENDOR_RAMDISK_NAME_SIZE]; /* asciiz ramdisk name */

  // Hardware identifiers describing the board, soc or platform which this
  // ramdisk is intended to be loaded on.
  UINT32 board_id[VENDOR_RAMDISK_TABLE_ENTRY_BOARD_ID_SIZE];
};
struct kernel64_hdr {
  UINT32 Code0;       /* Executable code */
  UINT32 Code1;       /* Executable code */
  UINT64 TextOffset; /* Image load offset, little endian */
  UINT64 ImageSize;  /* Effective Image size, little endian */
  UINT64 Flags;       /* kernel flags, little endian */
  UINT64 Res2;        /* reserved */
  UINT64 Res3;        /* reserved */
  UINT64 Res4;        /* reserved */
  UINT32 magic_64;    /* Magic number, little endian,
                         "ARM\x64" i.e 0x644d5241*/
  UINT32 Res5;        /* reserved (used for PE COFF offset) */
};

typedef struct kernel64_hdr Kernel64Hdr;

#endif
