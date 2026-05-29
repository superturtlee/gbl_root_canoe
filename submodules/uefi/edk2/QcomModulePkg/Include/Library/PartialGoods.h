/* Copyright (c) 2017, The Linux Foundation. All rights reserved.
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
 * Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All rights reserved.
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

#ifndef __PARTIALGOODS_H__
#define __PARTIALGOODS_H__

#include <Library/Board.h>
#define MAX_CPU_CLUSTER 4
#define SUBSET_PART_CHIPINFO_BASE_REVISION 0x0000000000010002

struct SubNodeListNew {
  CONST CHAR8 *SubNodeName;  /* Subnode name */
  CONST CHAR8 *PropertyName; /* Property name */
  CONST CHAR8 *PropertyStr;  /* Property string value */
  CONST CHAR8 *ReplaceStr;   /* Replace string */
};

struct PartialGoods {
  UINT32 Val;                    /* Value for the subset */
  CONST CHAR8 *ParentNode;       /* Parent Node name*/
  struct SubNodeListNew SubNode; /* Sub node name list*/
};

struct LabelStruct {
  CONST CHAR8 *LabelName;  /* Label name */
  CONST CHAR8 *PropertyName; /* Property name */
  CONST CHAR8 *ReplaceStr;   /* Replace string */
};

struct PartialGoodsWithLabel {
  UINT32 Val;                    /* Value for the subset */
  struct LabelStruct LabelRef; /* Labels list */
};

 STATIC CONST char *ChipInfoPartTypeStr[] = {
  [EFICHIPINFO_PART_UNKNOWN]   = "unknown",
  [EFICHIPINFO_PART_GPU]       = "gpu",
  [EFICHIPINFO_PART_VIDEO]     = "video",
  [EFICHIPINFO_PART_CAMERA]    = "camera",
  [EFICHIPINFO_PART_DISPLAY]   = "display",
  [EFICHIPINFO_PART_AUDIO]     = "audio",
  [EFICHIPINFO_PART_MODEM]     = "modem",
  [EFICHIPINFO_PART_WLAN]      = "wlan",
  [EFICHIPINFO_PART_COMP]      = "comp",
  [EFICHIPINFO_PART_SENSORS]   = "sensors",
  [EFICHIPINFO_PART_NPU]       = "npu",
  [EFICHIPINFO_PART_SPSS]      = "spss",
  [EFICHIPINFO_PART_NAV]       = "nav",
  [EFICHIPINFO_PART_COMPUTE_1] = "comp1",
  [EFICHIPINFO_PART_DISPLAY_1] = "display1",
  [EFICHIPINFO_PART_NSP]       = "nsp",
  [EFICHIPINFO_PART_EVA]       = "eva",
};

EFI_STATUS
UpdatePartialGoodsNode (VOID *fdt);

EFI_STATUS
ReadMMPartialGoods (EFI_CHIPINFO_PROTOCOL *pChipInfoProtocol, UINT32 *Value);
#endif
