/* Copyright (c) 2016, 2017, 2019, The Linux Foundation. All rights reserved.
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
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
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

#ifndef _DRAWUI_H_
#define _DRAWUI_H_

#include <Uefi.h>

/* 45 characters per line for portrait orientation
 * "720 (W) 1280(H)" : "sysfont2x" -- 720 /(8*2) = 45
 * "1080(W) 1920(H)" : "sysfont3x" -- 1080/(8*3) = 45
 * "1440(W) 2560(H)" : "sysfont4x" -- 1440/(8*4) = 45
 * "2160(W) 3840(H)" : "sysfont6x" -- 2160/(8*6) = 45
 */
#define CHAR_NUM_PERROW_POR 45

/* 80 characters per line for horizontal orientation
 * "480 (H) 640 (W)" : ""       -- 640 / (8 * 1) = 80
 * "720 (H) 1280(W)" : "sysfont2x" -- 1280/(8*2) = 80
 * "1080(H) 1920(W)" : "sysfont3x" -- 1920/(8*3) = 80
 * "1440(H) 2560(W)" : "sysfont4x" -- 2560/(8*4) = 80
 * "2160(H) 3840(W)" : "sysfont6x" -- 3840/(8*6) = 80
 */
#define CHAR_NUM_PERROW_HOR 80

/* Max row for portrait orientation
 * "720  (W) 1280(H)" : "sysfont2x" -- 1280/(19*2) = 33
 * "1080(W) 1920(H)" : "sysfont3x" -- 1920/(19*3) = 33
 * "1440(W) 2560(H)" : "sysfont4x" -- 2560/(19*4) = 33
 * "2160(W) 3840(H)" : "sysfont6x" -- 3840/(19*6) = 33
 */
#define MAX_ROW_FOR_POR 33

/* Max row for horizontal orientation
 * "480 (H) 640 (W)" : ""         -- 480/(19*1) = 25
 * "720  (H) 1280(W)" : ""        -- 720/(19*1) = 37
 * "1080(H) 1920(W)" : "sysfont2" -- 1080/(19*2) = 28
 * "1440(H) 2560(W)" : "sysfont3" -- 1440/(19*3) = 25
 * "2160(H) 3840(W)" : "sysfont4" -- 2160/(19*4) = 28
 */
#define MAX_ROW_FOR_HOR 25

#define MAX_MSG_SIZE 256
#define MAX_RSP_SIZE 64

typedef enum {
  PORTRAIT_MODE = 0,
  HORIZONTAL_MODE
} DISPLAY_MODE;

typedef enum {
  DISPLAY_MENU_YELLOW = 0,
  DISPLAY_MENU_ORANGE,
  DISPLAY_MENU_RED,
  DISPLAY_MENU_EIO,
  DISPLAY_MENU_MORE_OPTION,
  DISPLAY_MENU_UNLOCK,
  DISPLAY_MENU_FASTBOOT,
  DISPLAY_MENU_UNLOCK_CRITICAL,
  DISPLAY_MENU_LOCK,
  DISPLAY_MENU_LOCK_CRITICAL
} DISPLAY_MENU_TYPE;

typedef enum {
  BGR_WHITE,
  BGR_BLACK,
  BGR_ORANGE,
  BGR_YELLOW,
  BGR_RED,
  BGR_GREEN,
  BGR_BLUE,
  BGR_CYAN,
  BGR_SILVER,
} COLOR_TYPE;

typedef enum {
  COMMON_FACTOR = 1,
  BIG_FACTOR,
  MAX_FACTORTYPE = 2,
} SCALE_FACTOR_TYPE;

typedef enum {
  POWEROFF = 0,
  RESTART,
  RECOVER,
  FASTBOOT,
  BACK,
  CONTINUE,
  FFBM,
  QMMI,
  ALTERNATESLOT,
  NOACTION,
  OPTION_ACTION_MAX,
} OPTION_ITEM_ACTION;

typedef enum {
  COMMON = 0,
  ALIGN_RIGHT,
  ALIGN_LEFT,
  OPTION_ITEM,
  LINEATION,
} MENU_STRING_TYPE;

typedef struct {
  CHAR8 Msg[MAX_MSG_SIZE];
  UINT32 ScaleFactorType;
  UINT32 FgColor;
  UINT32 BgColor;
  UINT32 Attribute;
  UINT32 Location;
  UINT32 Action;
} MENU_MSG_INFO;

typedef struct {
  MENU_MSG_INFO *MsgInfo;
  UINT32 OptionItems[OPTION_ACTION_MAX];
  UINT32 OptionNum;
  UINT32 OptionIndex;
  UINT32 MenuType;
  UINT32 TimeoutTime;
} MENU_OPTION_ITEM_INFO;

typedef struct {
  MENU_OPTION_ITEM_INFO Info;
  UINT32 LastMenuType;
} OPTION_MENU_INFO;

VOID
SetMenuMsgInfo (MENU_MSG_INFO *MenuMsgInfo,
                CHAR8 *Msg,
                UINT32 ScaleFactorType,
                UINT32 FgColor,
                UINT32 BgColor,
                UINT32 Attribute,
                UINT32 Location,
                UINT32 Action);
EFI_STATUS
DrawMenu (MENU_MSG_INFO *TargetMenu, UINT32 *Height);
EFI_STATUS
UpdateMsgBackground (MENU_MSG_INFO *MenuMsgInfo, UINT32 NewBgColor);
EFI_STATUS BackUpBootLogoBltBuffer (VOID);
VOID RestoreBootLogoBitBuffer (VOID);
VOID FreeBootLogoBltBuffer (VOID);
VOID DrawMenuInit (VOID);
#endif
