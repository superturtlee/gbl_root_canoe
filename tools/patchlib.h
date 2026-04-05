#include "types.h"

#include "arm64_inst_decoder.h"

typedef enum { LOC_REG, LOC_STK64, LOC_STK8 } DataLocType;

typedef struct {
    DataLocType type;
    INT32 val;
} DataLoc;

typedef struct {
    DataLoc locs[256];
    INT32 count;
} LocSet;

static BOOLEAN locset_has(const LocSet* s, DataLoc l) {
    for (INT32 i = 0; i < s->count; ++i)
        if (s->locs[i].type == l.type && s->locs[i].val == l.val) return TRUE;
    return FALSE;
}
static BOOLEAN locset_has_reg  (const LocSet* s, INT8 r)   { return locset_has(s, (DataLoc){LOC_REG, r}); }
static BOOLEAN locset_has_stk64(const LocSet* s, UINT32 v) { return locset_has(s, (DataLoc){LOC_STK64, (INT32)v}); }
static BOOLEAN locset_has_stk8 (const LocSet* s, UINT32 v) { return locset_has(s, (DataLoc){LOC_STK8, (INT32)v}); }

static void locset_add(LocSet* s, DataLoc l) {
    if (!locset_has(s, l) && s->count < 256) s->locs[s->count++] = l;
}
static void locset_add_reg  (LocSet* s, INT8 r)   { locset_add(s, (DataLoc){LOC_REG, r}); }
static void locset_add_stk64(LocSet* s, UINT32 v) { locset_add(s, (DataLoc){LOC_STK64, (INT32)v}); }
static void locset_add_stk8 (LocSet* s, UINT32 v) { locset_add(s, (DataLoc){LOC_STK8, (INT32)v}); }

static void locset_del(LocSet* s, DataLoc l) {
    for (INT32 i = 0; i < s->count; ++i) {
        if (s->locs[i].type == l.type && s->locs[i].val == l.val) {
            s->locs[i] = s->locs[s->count - 1];
            s->count--;
            return;
        }
    }
}
static void locset_del_reg  (LocSet* s, INT8 r)   { locset_del(s, (DataLoc){LOC_REG, r}); }
static void locset_del_stk64(LocSet* s, UINT32 v) { locset_del(s, (DataLoc){LOC_STK64, (INT32)v}); }
static void locset_del_stk8 (LocSet* s, UINT32 v) { locset_del(s, (DataLoc){LOC_STK8, (INT32)v}); }

static BOOLEAN locset_empty(const LocSet* s) { return s->count == 0; }

static void locset_print(const LocSet* s) {
    if (s->count == 0) {
        Print_patcher("WARRNING: LocSet empty, using fallback. Will Match any LDRB\n");
        return;
    }
    Print_patcher("  LocSet{");
    for (INT32 i = 0; i < s->count; ++i) {
        if (i) Print_patcher(", ");
        switch (s->locs[i].type) {
            case LOC_REG:   Print_patcher("W%d", s->locs[i].val); break;
            case LOC_STK64: Print_patcher("[SP+0x%X]/64", s->locs[i].val); break;
            case LOC_STK8:  Print_patcher("[SP+0x%X]/8",  s->locs[i].val); break;
        }
    }
    Print_patcher("}\n");
}

/* 判断一条指令是否为任意形式的 STRB，并提取字段 */
typedef struct {
    BOOLEAN valid;
    UINT8   rt;
    UINT8   rn;
    UINT32  imm;
} StrbInfo;

static StrbInfo decode_any_strb(UINT32 raw) {
    StrbInfo info = { FALSE, 0, 0, 0 };
    DecodedInst d;
    memset(&d, 0, sizeof(d));

    if (decode_inst_strb_imm(raw, &d)) {
        info.valid = TRUE; info.rt = d.rt; info.rn = d.rn; info.imm = d.imm;
    } else if (decode_inst_strb_post(raw, &d)) {
        info.valid = TRUE; info.rt = d.rt; info.rn = d.rn; info.imm = (UINT32)d.simm & 0x1FF;
    } else if (decode_inst_strb_pre(raw, &d)) {
        info.valid = TRUE; info.rt = d.rt; info.rn = d.rn; info.imm = (UINT32)d.simm & 0x1FF;
    }
    return info;
}

INT32 patch_abl_gbl(CHAR8* buffer, INT32 size) {
    CHAR8 target[]      = { 'e',0, 'f',0, 'i',0, 's',0, 'p',0 };
    CHAR8 replacement[] = { 'n',0, 'u',0, 'l',0, 'l',0, 's',0 };
    INT32 target_len = sizeof(target);
    for (INT32 i = 0; i < size - target_len; ++i) {
        if (memcmp_patcher(buffer + i, target, target_len) == 0) {
            memcpy_patcher(buffer + i, replacement, target_len);
            return 0;
        }
    }
    return -1;
}

INT16 Original[] = {
    -1, 0x00, 0x00, 0x34, 0x28, 0x00, 0x80, 0x52,
    0x06, 0x00, 0x00, 0x14, 0xE8, -1, 0x40, 0xF9,
    0x08, 0x01, 0x40, 0x39, 0x1F, 0x01, 0x00, 0x71,
    0xE8, 0x07, 0x9F, 0x1A, 0x08, 0x79, 0x1F, 0x53
};
INT16 Patched[] = {
    -1, -1, -1, -1, 0x08, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1
};

INT32 patch_abl_bootstate(CHAR8* buffer, INT32 size,
                          INT8* lock_register_num, INT32* offset) {
    INT32 pattern_len = sizeof(Original) / sizeof(INT16);
    INT32 patched_count = 0;
    if (size < pattern_len) return 0;
    for (INT32 i = 0; i <= size - pattern_len; ++i) {
        BOOLEAN match = TRUE;
        for (INT32 j = 0; j < pattern_len; ++j) {
            if (Original[j] != -1 && (UINT8)buffer[i + j] != (UINT8)Original[j]) {
                match = FALSE; break;
            }
        }
        if (match) {
            *lock_register_num = (INT8)((UINT8)buffer[i] & 0x1F);
            *offset = i;
            #ifndef DISABLE_PATCH_3
            for (INT32 j = 0; j < pattern_len; ++j)
                if (Patched[j] != -1) buffer[i + j] = (char)Patched[j];
            #endif
            patched_count++;
            i += pattern_len - 1;
        }
    }
    return patched_count;
}

static INT32 track_forward_patch_strb(CHAR8* buffer, INT32 size, INT32 ldrb_off,
                                      INT8 src_reg, INT32 anchor_off) {
    LocSet set;
    set.count = 0;
    locset_add_reg(&set, src_reg);

    Print_patcher("\n=== Forward tracking from LDRB@0x%X (W%d), anchor=0x%X ===\n",
           ldrb_off, (int)src_reg, anchor_off);
    locset_print(&set);

    for (INT32 off = ldrb_off + 4; off < size - 4; off += 4) {
        DecodedInst d = decode_at(buffer, off);

        if (d.type == INST_PACIASP) {
            Print_patcher("0x%X: function boundary, stop\n", off);
            break;
        }

        switch (d.type) {

        /* ---- STR Xt, [SP, #imm] 64-bit spill ---- */
        case INST_STR_X_IMM:
            if (d.rn == 31) {
                if (locset_has_reg(&set, (INT8)d.rt)) {
                    Print_patcher("  0x%X: STR X%d,[SP,#0x%X] spill64\n", off, d.rt, d.imm);
                    locset_add_stk64(&set, d.imm);
                    locset_print(&set);
                } else if (locset_has_stk64(&set, d.imm)) {
                    Print_patcher("  0x%X: STR X%d,[SP,#0x%X] overwrite stk64 -> del\n", off, d.rt, d.imm);
                    locset_del_stk64(&set, d.imm);
                    locset_print(&set);
                }
            }
            break;

        /* ---- LDR Xt, [SP, #imm] 64-bit reload ---- */
        case INST_LDR_X_IMM:
            if (d.rn == 31) {
                if (locset_has_stk64(&set, d.imm)) {
                    Print_patcher("  0x%X: LDR X%d,[SP,#0x%X] reload64\n", off, d.rt, d.imm);
                    locset_add_reg(&set, (INT8)d.rt);
                    locset_print(&set);
                } else if (locset_has_reg(&set, (INT8)d.rt)) {
                    Print_patcher("  0x%X: LDR X%d,[SP,#0x%X] overwrite reg -> del\n", off, d.rt, d.imm);
                    locset_del_reg(&set, (INT8)d.rt);
                    locset_print(&set);
                }
            }
            break;

        /* ---- STR Wt, [SP, #imm] 32-bit spill ---- */
        case INST_STR_W_IMM:
            if (d.rn == 31) {
                if (locset_has_reg(&set, (INT8)d.rt)) {
                    Print_patcher("  0x%X: STR W%d,[SP,#0x%X] spill32\n", off, d.rt, d.imm);
                    locset_add_stk64(&set, d.imm);
                    locset_print(&set);
                } else if (locset_has_stk64(&set, d.imm)) {
                    Print_patcher("  0x%X: STR W%d,[SP,#0x%X] overwrite stk -> del\n", off, d.rt, d.imm);
                    locset_del_stk64(&set, d.imm);
                    locset_print(&set);
                }
            }
            break;

        /* ---- LDR Wt, [SP, #imm] 32-bit reload ---- */
        case INST_LDR_W_IMM:
            if (d.rn == 31) {
                if (locset_has_stk64(&set, d.imm)) {
                    Print_patcher("  0x%X: LDR W%d,[SP,#0x%X] reload32\n", off, d.rt, d.imm);
                    locset_add_reg(&set, (INT8)d.rt);
                    locset_print(&set);
                } else if (locset_has_reg(&set, (INT8)d.rt)) {
                    Print_patcher("  0x%X: LDR W%d,[SP,#0x%X] overwrite reg -> del\n", off, d.rt, d.imm);
                    locset_del_reg(&set, (INT8)d.rt);
                    locset_print(&set);
                }
            }
            break;

        /* ---- LDRB Wt, [Xn, #imm] — 外部内存覆写寄存器 ---- */
        case INST_LDRB_IMM:
            if (locset_has_reg(&set, (INT8)d.rt)) {
                Print_patcher("  0x%X: LDRB W%d,[X%d,#0x%X] overwrite reg -> del\n",
                       off, d.rt, d.rn, d.imm);
                locset_del_reg(&set, (INT8)d.rt);
                locset_print(&set);
            }
            break;

        /* ---- MOV Xd, Xm ---- */
        case INST_MOV_X:
            if (locset_has_reg(&set, (INT8)d.rm) && d.rt != 31) {
                Print_patcher("  0x%X: MOV X%d,X%d propagate\n", off, d.rt, d.rm);
                locset_add_reg(&set, (INT8)d.rt);
                locset_print(&set);
            } else if (locset_has_reg(&set, (INT8)d.rt)) {
                Print_patcher("  0x%X: MOV X%d,X%d overwrite -> del\n", off, d.rt, d.rm);
                locset_del_reg(&set, (INT8)d.rt);
                locset_print(&set);
            }
            break;

        /* ---- MOV Wd, Wm ---- */
        case INST_MOV_W:
            if (locset_has_reg(&set, (INT8)d.rm) && d.rt != 31) {
                Print_patcher("  0x%X: MOV W%d,W%d propagate\n", off, d.rt, d.rm);
                locset_add_reg(&set, (INT8)d.rt);
                locset_print(&set);
            } else if (locset_has_reg(&set, (INT8)d.rt)) {
                Print_patcher("  0x%X: MOV W%d,W%d overwrite -> del\n", off, d.rt, d.rm);
                locset_del_reg(&set, (INT8)d.rt);
                locset_print(&set);
            }
            break;

        /* ---- STRB (所有形式) ---- */
        case INST_STRB_IMM:
        case INST_STRB_POST:
        case INST_STRB_PRE: {
            StrbInfo si = decode_any_strb(d.raw);
            if (si.valid && (locset_has_reg(&set, (INT8)si.rt)||(locset_empty(&set)&&off > anchor_off))) {
                if (off > anchor_off) {

                    #ifndef DISABLE_PRINT
                    if (si.rn == 31) {
                        Print_patcher("  0x%X: STRB W%d,[SP,#0x%X] ** SINK (after anchor0x%X) **\n",
                        off, si.rt, si.imm, anchor_off);
                    } else {
                        Print_patcher("  0x%X: STRB W%d,[X%d,#0x%X] ** SINK (after anchor0x%X) **\n",
                        off, si.rt, si.rn, si.imm, anchor_off);
                    }
                    #endif
                    Print_patcher("  Before: %02X %02X %02X %02X\n",
                           (UINT8)buffer[off], (UINT8)buffer[off+1],
                           (UINT8)buffer[off+2], (UINT8)buffer[off+3]);

                    write_instr(buffer, off, strb_with_reg(d.raw, 31));

                    Print_patcher("  After : %02X %02X %02X %02X (Rt -> WZR)\n",
                           (UINT8)buffer[off], (UINT8)buffer[off+1],
                           (UINT8)buffer[off+2], (UINT8)buffer[off+3]);
                    return 1;
                } else {
                    Print_patcher("  0x%X: STRB W%d,[X%d,#0x%X] before anchor -> spill8\n",
                           off, si.rt, si.rn, si.imm);
                    if (si.rn == 31) locset_add_stk8(&set, si.imm);
                    locset_print(&set);
                }
            } else if (si.valid && si.rn == 31 && locset_has_stk8(&set, si.imm)) {
                Print_patcher("  0x%X: STRB W%d,[SP,#0x%X] overwrite stk8 -> del\n",
                       off, si.rt, si.imm);
                locset_del_stk8(&set, si.imm);
            }
            break;
        }

        default:
            break;
        }
    }

    Print_patcher("Forward tracking: no sink STRB found after anchor 0x%X\n", anchor_off);
    return -1;
}

/* ============================================================
 *  第十二部分：反向找 LDRB 源头
 * ============================================================ */
INT32 find_ldrB_instructio_reverse(CHAR8* buffer, INT32 size,
                                   INT32 anchor_offset, INT8 target_register) {
    INT32 now_offset = anchor_offset - 4;
    INT8 current_target = target_register;
    INT32 bounce_count = 0;
    const INT32 MAX_BOUNCES = 8;

    while (now_offset >= 0) {
        DecodedInst d = decode_at(buffer, now_offset);

        if (d.type == INST_PACIASP) {
            Print_patcher("Reached function start at 0x%X\n", now_offset);
            break;
        }

        /* ---- 64-bit 栈 reload 弹跳 ---- */
        if (d.type == INST_LDR_X_IMM && d.rn == 31 && (INT8)d.rt == current_target) {
            UINT32 spill_imm = d.imm;
            Print_patcher("Bounce at 0x%X: LDR X%d,[SP,#0x%X]\n",
                   now_offset, (int)current_target, spill_imm);
            INT32 search = now_offset - 4;
            BOOLEAN found = FALSE;
            while (search >= 0) {
                DecodedInst ds = decode_at(buffer, search);
                if (ds.type == INST_PACIASP) break;
                if (ds.type == INST_STR_X_IMM && ds.rn == 31 && ds.imm == spill_imm) {
                    Print_patcher("  -> STR X%d,[SP,#0x%X] at 0x%X\n",
                           (INT32)ds.rt, spill_imm, search);
                    current_target = (INT8)ds.rt;
                    now_offset = search - 4;
                    found = TRUE;
                    bounce_count++;
                    break;
                }
                search -= 4;
            }
            if (!found) { Print_patcher("  -> No matching STR, abort\n"); return -1; }
            if (bounce_count > MAX_BOUNCES) { Print_patcher("Too many bounces\n"); return -1; }
            continue;
        }

        /* ---- byte 级栈 reload 弹跳 ---- */
        if (d.type == INST_LDRB_IMM && d.rn == 31 && (INT8)d.rt == current_target) {
            UINT32 byte_imm = d.imm;
            Print_patcher("Byte bounce at 0x%X: LDRB W%d,[SP,#0x%X]\n",
                   now_offset, (int)current_target, byte_imm);
            INT32 search = now_offset - 4;
            BOOLEAN found = FALSE;
            while (search >= 0) {
                DecodedInst ds = decode_at(buffer, search);
                if (ds.type == INST_PACIASP) break;
                if (ds.type == INST_STRB_IMM && ds.rn == 31 && ds.imm == byte_imm) {
                    Print_patcher("  -> STRB W%d,[SP,#0x%X] at 0x%X\n",
                           (INT32)ds.rt, byte_imm, search);
                    current_target = (INT8)ds.rt;
                    now_offset = search - 4;
                    found = TRUE;
                    bounce_count++;
                    break;
                }
                search -= 4;
            }
            if (!found) { Print_patcher("  -> No matching STRB, abort\n"); return -1; }
            if (bounce_count > MAX_BOUNCES) { Print_patcher("Too many bounces\n"); return -1; }
            continue;
        }

        /* ---- 真正源头: LDRB W{current_target}, [Xn!=SP, #imm] ---- */
        if (d.type == INST_LDRB_IMM && (INT8)d.rt == current_target && d.rn != 31) {
            Print_patcher("Found source LDRB at 0x%X: LDRB W%d,[X%d,#0x%X](%d bounces)\n",
                   now_offset, d.rt, d.rn, d.imm, bounce_count);
            Print_patcher("  Before: %02X %02X %02X %02X\n",
                   (UINT8)buffer[now_offset], (UINT8)buffer[now_offset+1],
                   (UINT8)buffer[now_offset+2], (UINT8)buffer[now_offset+3]);
            #ifndef DISABLE_PATCH_4
            write_instr(buffer, now_offset, encode_movz_w((UINT8)current_target, 1));
            Print_patcher("  After : %02X %02X %02X %02X (MOV W%d, #1)\n",
                   (UINT8)buffer[now_offset], (UINT8)buffer[now_offset+1],
                   (UINT8)buffer[now_offset+2], (UINT8)buffer[now_offset+3],
                   (int)current_target);
            #endif
            #ifndef DISABLE_PATCH_5
            INT32 fwd = track_forward_patch_strb(buffer, size, now_offset,
                                                  current_target, anchor_offset);
            if (fwd <= 0) {
                Print_patcher("Warning: sink STRB not found after anchor 0x%X\n", anchor_offset);
                return -1;
            }
            Print_patcher("Sink patched successfully.\n");
            #endif
            return 0;
        }

        now_offset -= 4;
    }
    return -1;
}

static BOOLEAN str_at(const CHAR8* buffer, INT32 size, INT64 file_off, const CHAR8* needle) {
    if (file_off < 0) return FALSE;
    INT32 len = strlen(needle);
    if ((INT32)file_off + len >= size) return FALSE;
    return memcmp_patcher(buffer + file_off, needle, len) == 0;
}

static BOOLEAN rewrite_cbz_as_b(CHAR8* buffer, INT32 cbz_off) {
    UINT32 raw = 0;
    INT32 imm19;
    UINT32 branch;

    memcpy_patcher(&raw, buffer + cbz_off, 4);
    imm19 = (INT32)((raw >> 5) & 0x7FFFF);
    if (imm19 & 0x40000)
        imm19 |= ~0x7FFFF;

    branch = 0x14000000U | ((UINT32)imm19 & 0x03FFFFFFU);
    write_instr(buffer, cbz_off, branch);
    return TRUE;
}

static INT64 calc_adrl_file_offset(const CHAR8* buffer, INT32 adrp_off, UINT64 load_base) {
    DecodedInst d0 = decode_at(buffer, adrp_off);
    DecodedInst d1 = decode_at(buffer, adrp_off + 4);

    if (d0.type != INST_ADRP) return -1;
    if (d1.type != INST_ADD_X_IMM) return -1;
    if (d1.rt != d0.rt || d1.rn != d0.rt) return -1;

    UINT64 pc      = load_base + (UINT64)adrp_off;
    UINT64 page_pc = pc & ~0xFFFull;
    UINT64 target_va = (UINT64)((INT64)page_pc + d0.simm) + d1.imm;
    return (INT64)(target_va - load_base);
}

/*
 * Patch 7: Skip orange state screen, warning text, and 5-second boot delay.
 *
 * LinuxLoaderEntry contains:
 *   if (DAT_000b13c5 != 0) {          // unlocked flag
 *       FUN_000028b0(..., "Your device has been unlocked...", ...);
 *       FUN_000028b0(..., "Your device will boot in 5 seconds", ...);
 *       DAT_00081f0c = 3;
 *       DAT_00081f18 = uVar11;
 *       FUN_00031560();               // ShowDeviceStatetoscreen
 *   }
 *
 * The guard is a CBZ that skips the block when locked (Wt==0).
 * We rewrite that guard as an unconditional B, so the whole block is skipped
 * regardless of lock state.
 *
 * Anchor: the unique 8-byte sequence ending with the CBZ:
 *   36 31 88 1A  (AND/CSEL before the branch)
 *   ?? 04 00 34  (CBZ Wt, +#imm — Rt field varies, upper 3 bytes fixed)
 *
 * Fallback anchor: the countdown loop inside the warning block.
 * This is the same branch targeted by the older "hide unlock prompt" patch;
 * it does not skip an unlock menu, it skips the orange warning block.
 */
INT32 patch_orange_state_screen(CHAR8* buffer, INT32 size) {
    static const UINT8 anchor[] = { 0x36, 0x31, 0x88, 0x1A };
    static const UINT8 cbz_hi[] = { 0x04, 0x00, 0x34 };
    static const INT16 delay_anchor[] = {
        -1, 0x06, 0x80, 0x52,
        -1, 0x00, 0x00, -1,
        -1, 0x05, -1, -1
    };

    INT32 patched = 0;

    for (INT32 i = 0; i <= size - 8; i += 4) {
        if (memcmp_patcher(buffer + i,     anchor, 4) != 0) continue;
        if (memcmp_patcher(buffer + i + 5, cbz_hi, 3) != 0) continue;
        #ifndef DISABLE_PRINT
        Print_patcher("Patch 7: orange state CBZ at 0x%X  W%d -> B\n",
                      i + 4, (int)((UINT8)buffer[i + 4] & 0x1F));
        Print_patcher("  Before: %02X %02X %02X %02X\n",
                      (UINT8)buffer[i+4], (UINT8)buffer[i+5],
                      (UINT8)buffer[i+6], (UINT8)buffer[i+7]);
        #endif
        rewrite_cbz_as_b(buffer, i + 4);
        #ifndef DISABLE_PRINT
        Print_patcher("  After : %02X %02X %02X %02X\n",
                      (UINT8)buffer[i+4], (UINT8)buffer[i+5],
                      (UINT8)buffer[i+6], (UINT8)buffer[i+7]);
        #endif
        patched++;
    }

    if (patched == 0) {
        INT32 pattern_len = sizeof(delay_anchor) / sizeof(INT16);

        for (INT32 i = 0; i <= size - pattern_len; ++i) {
            BOOLEAN match = TRUE;
            for (INT32 j = 0; j < pattern_len; ++j) {
                if (delay_anchor[j] != -1 &&
                    (UINT8)buffer[i + j] != (UINT8)delay_anchor[j]) {
                    match = FALSE;
                    break;
                }
            }
            if (!match || i < 4)
                continue;

            #ifndef DISABLE_PRINT
            Print_patcher("Patch 7 fallback: orange state CBZ at 0x%X -> B\n", i - 4);
            Print_patcher("  Before: %02X %02X %02X %02X\n",
                          (UINT8)buffer[i-4], (UINT8)buffer[i-3],
                          (UINT8)buffer[i-2], (UINT8)buffer[i-1]);
            #endif
            rewrite_cbz_as_b(buffer, i - 4);
            #ifndef DISABLE_PRINT
            Print_patcher("  After : %02X %02X %02X %02X\n",
                          (UINT8)buffer[i-4], (UINT8)buffer[i-3],
                          (UINT8)buffer[i-2], (UINT8)buffer[i-1]);
            #endif
            patched++;
            break;
        }
    }

    #ifndef DISABLE_PRINT
    if (patched == 0)
        Print_patcher("Patch 7: orange state CBZ not found\n");
    else
        Print_patcher("Patch 7: applied %d location(s)\n", patched);
    #endif
    return patched;
}

INT32 patch_adrl_unlocked_to_locked(CHAR8* buffer, INT32 size, UINT64 load_base) {
    if (size < 24) return 0;
    INT32 patched = 0;

    for (INT32 i = 0; i <= size - 24; i += 4) {
        DecodedInst a0 = decode_at(buffer, i);
        DecodedInst a1 = decode_at(buffer, i + 4);
        DecodedInst b0 = decode_at(buffer, i + 8);
        DecodedInst b1 = decode_at(buffer, i + 12);
        DecodedInst c0 = decode_at(buffer, i + 16);
        DecodedInst c1 = decode_at(buffer, i + 20);

        if (a0.type != INST_ADRP || a1.type != INST_ADD_X_IMM) continue;
        if (a1.rt != a0.rt || a1.rn != a0.rt) continue;

        if (b0.type != INST_ADRP || b1.type != INST_ADD_X_IMM) continue;
        if (b1.rt != b0.rt || b1.rn != b0.rt) continue;

        if (c0.type != INST_ADRP || c1.type != INST_ADD_X_IMM) continue;
        if (c1.rt != c0.rt || c1.rn != c0.rt) continue;

        UINT8 xa = a0.rt, xb = b0.rt, xc = c0.rt;
        if (xa == xb || xb == xc || xa == xc) continue;

        INT64 off0 = calc_adrl_file_offset(buffer, i,      load_base);
        INT64 off1 = calc_adrl_file_offset(buffer, i + 8,  load_base);
        INT64 off2 = calc_adrl_file_offset(buffer, i + 16, load_base);

        if (!str_at(buffer, size, off0, "unlocked")) continue;
        if (!str_at(buffer, size, off1, "locked"))   continue;
        if (!str_at(buffer, size, off2, "androidboot.vbmeta.device_state")) continue;

        Print_patcher("Found ADRL triple at 0x%X:\n", i);
        Print_patcher("  [0x%X] ADRP+ADD X%d -> file:0x%llX \"unlocked\"\n",
               i, xa, (unsigned long long)off0);
        Print_patcher("  [0x%X] ADRP+ADD X%d -> file:0x%llX \"locked\"\n",
               i+8, xb, (unsigned long long)off1);
        Print_patcher("  [0x%X] ADRP+ADD X%d -> file:0x%llX \"androidboot.vbmeta.device_state\"\n",
               i+16, xc, (unsigned long long)off2);

        UINT32 new_adrp = adrp_with_rd(b0.raw, xa);
        UINT32 new_add  = add_with_reg(b1.raw, xa);

        Print_patcher("  Patch pair-0: ADRP %08X->%08X, ADD %08X->%08X\n",
               a0.raw, new_adrp, a1.raw, new_add);

        write_instr(buffer, i,     new_adrp);
        write_instr(buffer, i + 4, new_add);

        patched++;
        i += 20;
    }

    if (patched == 0)
        Print_patcher("ADRL triple not found\n");
    else
        Print_patcher("ADRL patch applied: %d location(s)\n", patched);

    return patched;
}

INT32 patch_adrl_unlocked_to_locked_verify(CHAR8* buffer, INT32 size, UINT64 load_base) {
    if (size < 24) return 0;
    INT32 patched = 0;

    for (INT32 i = 0; i <= size - 24; i += 4) {
        DecodedInst a0 = decode_at(buffer, i);
        DecodedInst a1 = decode_at(buffer, i + 4);
        DecodedInst b0 = decode_at(buffer, i + 8);
        DecodedInst b1 = decode_at(buffer, i + 12);
        DecodedInst c0 = decode_at(buffer, i + 16);
        DecodedInst c1 = decode_at(buffer, i + 20);

        if (a0.type != INST_ADRP || a1.type != INST_ADD_X_IMM) continue;
        if (a1.rt != a0.rt || a1.rn != a0.rt) continue;

        if (b0.type != INST_ADRP || b1.type != INST_ADD_X_IMM) continue;
        if (b1.rt != b0.rt || b1.rn != b0.rt) continue;

        if (c0.type != INST_ADRP || c1.type != INST_ADD_X_IMM) continue;
        if (c1.rt != c0.rt || c1.rn != c0.rt) continue;

        UINT8 xa = a0.rt, xb = b0.rt, xc = c0.rt;
        if (xa == xb || xb == xc || xa == xc) continue;

        INT64 off0 = calc_adrl_file_offset(buffer, i,      load_base);
        INT64 off1 = calc_adrl_file_offset(buffer, i + 8,  load_base);
        INT64 off2 = calc_adrl_file_offset(buffer, i + 16, load_base);

        if (!str_at(buffer, size, off0, "locked")) continue;
        if (!str_at(buffer, size, off1, "locked")) continue;
        if (!str_at(buffer, size, off2, "androidboot.vbmeta.device_state")) continue;

        Print_patcher("Found ADRL triple at 0x%X:\n", i);
        Print_patcher("  [0x%X] ADRP+ADD X%d -> file:0x%llX \"locked\"\n",
               i, xa, (unsigned long long)off0);
        Print_patcher("  [0x%X] ADRP+ADD X%d -> file:0x%llX \"locked\"\n",
               i+8, xb, (unsigned long long)off1);
        Print_patcher("  [0x%X] ADRP+ADD X%d -> file:0x%llX \"androidboot.vbmeta.device_state\"\n",
               i+16, xc, (unsigned long long)off2);
        patched++;
        i += 20;
    }
    return patched;
}
CHAR8 keyword []="is not allowed in Lock State";
BOOLEAN check_sub_string(CHAR8* str,CHAR8* keyword){
    INT32 len = 0;
    INT32 str_len = 0;
    while(str[str_len]) str_len++;
    while(keyword[len]) len++;
    for (INT32 i = 0; i <= str_len - len; ++i) {
        if (memcmp_patcher(str + i, keyword, len) == 0) {
            return TRUE;
        }
    }
    return FALSE;
}
BOOLEAN patch_string_jump(CHAR8* buffer, INT32 size) {
    BOOLEAN patched = FALSE;
    for(int i = 0; i < size - 4; i += 4) {
        DecodedInst d = decode_at(buffer, i);
        INT64 jmp=0;
        if(get_JUMP_target(&d,i,&jmp)){
            if (jmp < 0 || jmp + 8 > size) continue;
            DecodedInst a0 = decode_at(buffer, jmp);
            DecodedInst a1 = decode_at(buffer, jmp + 4);
            if (a0.type != INST_ADRP || a1.type != INST_ADD_X_IMM) continue;
            if (a1.rt != a0.rt || a1.rn != a0.rt) continue;
            INT64 off0 = calc_adrl_file_offset(buffer, jmp, 0);
            if (off0 < 0 || off0 >= size) continue;
            CHAR8* str = buffer + off0;
            if(check_sub_string(str, keyword)){
                Print_patcher("  String: %s\n", str);
                //nop the jump instruction
                write_instr(buffer, i, NOP); // NOP
                patched = TRUE;
            }
        }
    }
    return patched;
}

/*
 * Patch 6: Force androidboot.veritymode=logging
 *
 * Two code paths set androidboot.veritymode in the ABL:
 *   Path A (VerifyBootAndAppendCmdline): ADRP+ADD loads "enforcing" string,
 *          CSEL picks between "eio" and "enforcing" based on managed mode flag.
 *   Path B (verity setup function): reads from a pointer table indexed by
 *          DeviceInfo.verity_mode — entry 0="logging", entry 1="enforcing".
 *
 * We patch BOTH paths:
 *   1) Scan for ADRP+ADD pairs where one resolves to "enforcing" near one
 *      resolving to "androidboot.veritymode", rewrite to point to "logging".
 *   2) Find the pointer table and overwrite the "enforcing" entry.
 */
INT32 patch_abl_verity_logging(CHAR8* buffer, INT32 size, UINT64 load_base) {
    INT32 patched = 0;

    /* --- Part 1: Patch ADRP+ADD code references to "enforcing" --- */
    for (INT32 i = 0; i <= size - 8; i += 4) {
        DecodedInst a0 = decode_at(buffer, i);
        DecodedInst a1 = decode_at(buffer, i + 4);
        if (a0.type != INST_ADRP || a1.type != INST_ADD_X_IMM) continue;
        if (a1.rt != a0.rt || a1.rn != a0.rt) continue;

        INT64 off = calc_adrl_file_offset(buffer, i, load_base);
        if (!str_at(buffer, size, off, "enforcing")) continue;

        /* Find a nearby "logging" string to get its address */
        INT64 off_log = -1;
        for (INT32 s = 0; s < size - 7; ++s) {
            if (buffer[s] == 'l' && memcmp_patcher(buffer + s, "logging", 7) == 0 &&
                (s == 0 || buffer[s - 1] == '\0')) {
                off_log = s;
                break;
            }
        }
        if (off_log < 0) {
            Print_patcher("Verity ADRP: found 'enforcing' at 0x%X but no 'logging' string\n", i);
            continue;
        }

        /* Verify this "enforcing" is near a "androidboot.veritymode" reference
         * (within 128 instructions / 512 bytes) to avoid false positives */
        BOOLEAN near_verity_key = FALSE;
        INT32 scan_start = (i > 512) ? i - 512 : 0;
        INT32 scan_end = (i + 512 < size - 8) ? i + 512 : size - 8;
        for (INT32 j = scan_start; j <= scan_end; j += 4) {
            DecodedInst t0 = decode_at(buffer, j);
            DecodedInst t1 = decode_at(buffer, j + 4);
            if (t0.type != INST_ADRP || t1.type != INST_ADD_X_IMM) continue;
            if (t1.rt != t0.rt || t1.rn != t0.rt) continue;
            INT64 toff = calc_adrl_file_offset(buffer, j, load_base);
            if (str_at(buffer, size, toff, "androidboot.veritymode")) {
                near_verity_key = TRUE;
                break;
            }
        }
        if (!near_verity_key) continue;

        UINT64 log_va = load_base + (UINT64)off_log;
        UINT64 pc = load_base + (UINT64)i;
        UINT64 page_pc = pc & ~0xFFFull;
        INT64 page_delta = (INT64)((log_va & ~0xFFFull) - page_pc);
        UINT32 lo12 = (UINT32)(log_va & 0xFFF);

        UINT32 immlo = (UINT32)((page_delta >> 12) & 0x3);
        UINT32 immhi = (UINT32)((page_delta >> 14) & 0x7FFFF);
        UINT32 new_adrp = 0x90000000 | (immlo << 29) | (immhi << 5) | a0.rt;
        UINT32 new_add = 0x91000000 | (lo12 << 10) | (a1.rt << 5) | a1.rt;

        Print_patcher("Verity ADRP patch at 0x%X: \"enforcing\" -> \"logging\"\n", i);
        Print_patcher("  ADRP %08X -> %08X, ADD %08X -> %08X\n",
                       a0.raw, new_adrp, a1.raw, new_add);

        write_instr(buffer, i, new_adrp);
        write_instr(buffer, i + 4, new_add);
        patched++;
    }

    Print_patcher("Verity ADRP: patched %d code reference(s)\n", patched);

    /* --- Part 2: Patch the pointer table --- */
    INT64 off_log = -1, off_enf = -1;
    for (INT32 s = 0; s < size - 9; ++s) {
        if (buffer[s] == 'l' && memcmp_patcher(buffer + s, "logging", 7) == 0 &&
            (s == 0 || buffer[s - 1] == '\0'))
            off_log = s;
        if (buffer[s] == 'e' && memcmp_patcher(buffer + s, "enforcing", 9) == 0 &&
            (s == 0 || buffer[s - 1] == '\0'))
            off_enf = s;
    }

    if (off_log >= 0 && off_enf >= 0) {
        UINT64 val_log = (UINT64)off_log;
        UINT64 val_enf = (UINT64)off_enf;
        for (INT32 i = 0; i <= size - 32; i += 8) {
            UINT64 v;
            memcpy_patcher(&v, buffer + i, 8);
            if (v == val_log) {
                UINT64 v2;
                memcpy_patcher(&v2, buffer + i + 16, 8);
                if (v2 == val_enf) {
                    Print_patcher("Verity table patch at 0x%X: [0x%llX] \"enforcing\" -> \"logging\"\n",
                                   i + 16, (unsigned long long)val_enf);
                    memcpy_patcher(buffer + i + 16, &val_log, 8);
                    patched++;
                    break;
                }
            }
        }
    }

    return (patched > 0) ? 0 : -1;
}

BOOLEAN PatchBuffer(CHAR8* data, INT32 size) {
    #ifndef DISABLE_PATCH_1
    if (patch_abl_gbl(data, size) != 0)
        Print_patcher("Warning: Failed to patch ABL GBL\n");
    #endif
    #ifndef DISABLE_PATCH_2
    if (patch_adrl_unlocked_to_locked(data, size, 0) == 0){
        Print_patcher("Warning: ADRL triple not found, skipping\n");
        free(data);
        return FALSE;
    }
    if (patch_adrl_unlocked_to_locked_verify(data, size, 0) == 0){
        Print_patcher("Error: ADRL verification failed\n");
        free(data);
        return FALSE;
    }
    #endif
    #ifndef DISABLE_PATCH_6
    if (patch_string_jump(data, size) != 0)
    Print_patcher("Warning: Failed to patch string jump\n");
    #endif
    INT32 offset = -1;
    INT8 lock_register_num = -1;
    INT32 num_patches = patch_abl_bootstate(data, size, &lock_register_num, &offset);
    if (num_patches == 0) {
        Print_patcher("Error: Failed to find/patch ABL Boot State\n");
        free(data);
        return 0;
    }
    Print_patcher("Anchor offset : 0x%X\n", offset);
    Print_patcher("Lock register : W%d\n", (int)lock_register_num);
    Print_patcher("Boot patches: %d\n", num_patches);

    if (find_ldrB_instructio_reverse(data, size, offset, lock_register_num) != 0) {
        Print_patcher("Warning: Failed to patch LDRB->STRB chain for W%d\n",
               (int)lock_register_num);
    }

    #ifndef DISABLE_PATCH_6
    if (patch_abl_verity_logging(data, size, 0) != 0)
        Print_patcher("Warning: Failed to patch verity mode to logging\n");
    #endif

    #ifndef DISABLE_PATCH_7
    if (patch_orange_state_screen(data, size) == 0)
        Print_patcher("Warning: Failed to patch orange state screen\n");
    #endif

    return 1;
}
