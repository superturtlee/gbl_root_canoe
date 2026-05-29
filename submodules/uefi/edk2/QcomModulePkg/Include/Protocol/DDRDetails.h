/* Copyright (c) 2018-2021, The Linux Foundation. All rights reserved.
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

#ifndef DDRDETAILS_H
#define DDRDETAILS_H

#define MAX_IDX_CH 8
#ifndef MAX_DDR_REGIONS
#define MAX_DDR_REGIONS   6
#endif
#define MAX_SHUB_ENTRIES   8

#define DDR_DETAILS_STRUCT_VERSION 0x0000000000070000

#define DDR_REGIONS_ENTRY_SMEM_OFFSET sizeof(ddr_details_entry)
#define SHUB_FREQ_PLAN_ENTRY_SMEM_OFFSET \
        (DDR_REGIONS_ENTRY_SMEM_OFFSET + sizeof(DDR_REGIONS_ENTRY))
#define DDR_MISC_INFO_SMEM_DATA_OFFSET \
        (SHUB_FREQ_PLAN_ENTRY_SMEM_OFFSET + sizeof(shub_freq_plan_entry))
#define DDR_INFO_SMEM_END \
        (DDR_MISC_INFO_SMEM_DATA_OFFSET + sizeof (ddr_misc_info))

/** DDR types. */
typedef enum
{
  DDR_TYPE_NODDR = 0,
  DDR_TYPE_LPDDR1 = 1,           /**< Low power DDR1. */
  DDR_TYPE_LPDDR2 = 2,       /**< Low power DDR2  set to 2 for compatibility*/
  DDR_TYPE_PCDDR2 = 3,           /**< Personal computer DDR2. */
  DDR_TYPE_PCDDR3 = 4,           /**< Personal computer DDR3. */

  DDR_TYPE_LPDDR3 = 5,           /**< Low power DDR3. */
  DDR_TYPE_LPDDR4 = 6,           /**< Low power DDR4. */
  DDR_TYPE_LPDDR4X = 7,            /**< Low power DDR4x. */
  DDR_TYPE_LPDDR5 = 8,            /**< Low power DDR5. */
  DDR_TYPE_LPDDR5X = 9,           /**< Low power DDR5x. */

  /**< For compatibility with deviceprogrammer(features not using DDR). */
  DDR_TYPE_UNUSED = 0x7FFFFFFF
} DDR_TYPE;


struct ddr_freq_table {
   UINT32 freq_khz;
   UINT8  enable;
};

typedef struct ddr_freq_plan_entry_info {
  struct ddr_freq_table ddr_freq[14];
  UINT8  num_ddr_freqs;
  UINT32* clk_period_address;
  UINT32  max_nom_ddr_freq;
} ddr_freq_plan_entry;

struct ddr_part_details {

  UINT8 revision_id1[2];
  UINT8 revision_id2[2];
  UINT8 width[2];
  UINT8 density[2];
};

typedef struct ddr_details_entry_info {
  UINT8   manufacturer_id;
  UINT8   device_type;
  struct  ddr_part_details ddr_params[MAX_IDX_CH];
  ddr_freq_plan_entry     ddr_freq_tbl;
  UINT8   num_channels;
  UINT8   sct_config;
} ddr_details_entry;

typedef struct
{
    UINT64 start_address;           // region physical address
    UINT64 size;                  // size of region in bytes
    UINT64 mc_address;              // region MC view address
    UINT32 granule_size;            // size of segments in MB (1024 * 1024 bytes
    UINT8  ddr_rank;                // rank bitmask (Rank 0: 0x1, Rank 1: 0x2)
    UINT8  segments_start_index;    // index of first full segment in a region
    UINT64 segments_start_offset;   // offset in bytes to first full segment
}
DDR_REGIONS_STRUCT_ENTRY;

typedef struct ddr_regions_data_info
{
  UINT32 no_of_ddr_regions;   /* No of DDR regions */
  UINT64 ddr_rank0_size;     /*Size in bytes */
  UINT64 ddr_rank1_size;     /*Size in bytes */
  UINT64 ddr_cs0_start_addr; /* DDR rank0 start address */
  UINT64 ddr_cs1_start_addr; /* DDR rank1 start address */
  UINT32 highest_bank_bit;   /* highest address bit for bank address */
  DDR_REGIONS_STRUCT_ENTRY ddr_region[MAX_DDR_REGIONS];
}DDR_REGIONS_ENTRY;

struct shub_freq_table
{
  UINT8 enable;
  UINT32 freq_khz;
};

typedef struct shub_freq_plan_entry_info
{
  UINT8 num_shub_freqs;
  struct shub_freq_table shub_freq[MAX_SHUB_ENTRIES];
} shub_freq_plan_entry;

typedef struct _ddr_misc_info
{
  UINT32 dsf_version;
  UINT32 reserved[10];
}ddr_misc_info;
#endif /* DDRDETAILS_H */
