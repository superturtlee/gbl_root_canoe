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
        if (locset_empty(&set)) {
            Print_patcher("  LocSet empty, stop\n");
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
                }
            }
            break;

        /* ---- LDRB Wt, [Xn, #imm] — 外部内存覆写寄存器 ---- */
        case INST_LDRB_IMM:
            if (locset_has_reg(&set, (INT8)d.rt)) {
                Print_patcher("  0x%X: LDRB W%d,[X%d,#0x%X] overwrite reg -> del\n",
                       off, d.rt, d.rn, d.imm);
                locset_del_reg(&set, (INT8)d.rt);
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
            }
            break;

        /* ---- STRB (所有形式) ---- */
        case INST_STRB_IMM:
        case INST_STRB_POST:
        case INST_STRB_PRE: {
            StrbInfo si = decode_any_strb(d.raw);
            if (si.valid && locset_has_reg(&set, (INT8)si.rt)) {
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

static INT64 find_cstring(const CHAR8* buffer, INT32 size, const CHAR8* needle) {
    INT32 len = strlen(needle) + 1;
    if (len <= 0 || size < len) return -1;
    for (INT32 i = 0; i <= size - len; ++i) {
        if (memcmp_patcher(buffer + i, needle, len) == 0) return i;
    }
    return -1;
}

static INT32 find_zero_run(const CHAR8* buffer, INT32 size, INT32 start, INT32 end, INT32 need) {
    if (need <= 0) return -1;
    if (start < 0) start = 0;
    if (end > size) end = size;
    if (start >= end) return -1;

    INT32 run_start = -1;
    INT32 run_len = 0;

    for (INT32 i = start; i < end; ++i) {
        if ((UINT8)buffer[i] == 0) {
            if (run_start < 0) run_start = i;
            run_len++;
            if (run_len >= need) return run_start;
        } else {
            run_start = -1;
            run_len = 0;
        }
    }
    return -1;
}

static INT64 ensure_cstring(CHAR8* buffer, INT32 size, INT32* cursor, INT32 end, const CHAR8* needle) {
    INT64 existing = find_cstring(buffer, size, needle);
    if (existing >= 0) return existing;

    INT32 len = strlen(needle) + 1;
    if (*cursor < 0 || *cursor + len > end || *cursor + len > size) return -1;

    memcpy_patcher(buffer + *cursor, needle, len);
    existing = *cursor;
    *cursor += len;
    return existing;
}

static UINT32 encode_adrp_target(UINT8 rd, INT32 instr_off, UINT64 load_base, UINT64 target_off) {
    INT64 pc_page = ((INT64)load_base + instr_off) & ~0xFFFLL;
    INT64 target_page = ((INT64)load_base + (INT64)target_off) & ~0xFFFLL;
    INT64 page_delta = (target_page - pc_page) >> 12;
    UINT32 imm21 = (UINT32)((UINT64)page_delta & 0x1FFFFFULL);
    UINT32 immlo = imm21 & 0x3u;
    UINT32 immhi = (imm21 >> 2) & 0x7FFFFu;

    return 0x90000000u | (immlo << 29) | (immhi << 5) | (rd & 0x1Fu);
}

static UINT32 encode_add_x_imm_target(UINT8 rd, UINT8 rn, UINT32 imm) {
    UINT32 imm12 = 0;
    UINT32 shift = 0;

    if (imm < 0x1000u) {
        imm12 = imm;
    } else if ((imm & 0xFFFu) == 0 && (imm >> 12) < 0x1000u) {
        imm12 = imm >> 12;
        shift = 1;
    } else {
        return 0;
    }

    return 0x91000000u
         | (shift << 22)
         | ((imm12 & 0xFFFu) << 10)
         | ((UINT32)(rn & 0x1Fu) << 5)
         | (rd & 0x1Fu);
}

static INT32 patch_fixed_adrl_target(CHAR8* buffer, INT32 size, INT32 adrp_off, UINT64 load_base,
                                     INT64 expected_target, INT64 new_target, const CHAR8* tag) {
    if (adrp_off < 0 || adrp_off + 8 > size) {
        Print_patcher("%s patch offset out of range\n", tag);
        return -1;
    }

    DecodedInst d0 = decode_at(buffer, adrp_off);
    DecodedInst d1 = decode_at(buffer, adrp_off + 4);
    if (d0.type != INST_ADRP || d1.type != INST_ADD_X_IMM || d1.rn != d0.rt) {
        Print_patcher("%s patch site is not ADRP+ADD\n", tag);
        return -1;
    }

    INT64 current_target = calc_adrl_file_offset(buffer, adrp_off, load_base);
    if (current_target == new_target) {
        Print_patcher("%s already patched -> file:0x%llX\n",
                      tag, (unsigned long long)new_target);
        return 0;
    }

    if (current_target != expected_target) {
        Print_patcher("%s expected file:0x%llX, found file:0x%llX\n",
                      tag,
                      (unsigned long long)expected_target,
                      (unsigned long long)current_target);
        return -1;
    }

    UINT32 new_adrp = encode_adrp_target(d0.rt, adrp_off, load_base, (UINT64)new_target);
    UINT32 new_add  = encode_add_x_imm_target(d1.rt, d1.rn, (UINT32)(new_target & 0xFFF));
    if (new_add == 0) {
        Print_patcher("%s new ADD immediate is out of range\n", tag);
        return -1;
    }

    Print_patcher("%s patch: file:0x%llX -> file:0x%llX\n",
                  tag,
                  (unsigned long long)current_target,
                  (unsigned long long)new_target);

    write_instr(buffer, adrp_off, new_adrp);
    write_instr(buffer, adrp_off + 4, new_add);
    return 0;
}

static INT32 patch_first_adrl_by_string(CHAR8* buffer, INT32 size, UINT64 load_base,
                                        const CHAR8* old_str, INT64 new_target,
                                        const CHAR8* tag) {
    for (INT32 i = 0; i <= size - 8; i += 4) {
        DecodedInst d0 = decode_at(buffer, i);
        DecodedInst d1 = decode_at(buffer, i + 4);

        if (d0.type != INST_ADRP || d1.type != INST_ADD_X_IMM) continue;
        if (d1.rn != d0.rt) continue;

        INT64 current_target = calc_adrl_file_offset(buffer, i, load_base);
        if (current_target == new_target) {
            Print_patcher("%s already patched -> file:0x%llX\n",
                          tag, (unsigned long long)new_target);
            return 0;
        }

        if (!str_at(buffer, size, current_target, old_str)) continue;

        UINT32 new_adrp = encode_adrp_target(d0.rt, i, load_base, (UINT64)new_target);
        UINT32 new_add  = encode_add_x_imm_target(d1.rt, d1.rn, (UINT32)(new_target & 0xFFF));
        if (new_add == 0) {
            Print_patcher("%s new ADD immediate is out of range\n", tag);
            return -1;
        }

        Print_patcher("%s patch: file:0x%llX \"%s\" -> file:0x%llX\n",
                      tag,
                      (unsigned long long)current_target,
                      old_str,
                      (unsigned long long)new_target);

        write_instr(buffer, i, new_adrp);
        write_instr(buffer, i + 4, new_add);
        return 0;
    }

    Print_patcher("%s source string \"%s\" not found\n", tag, old_str);
    return -1;
}

INT32 patch_hwcountry_global(CHAR8* buffer, INT32 size, UINT64 load_base) {
    const CHAR8 hwcountry_value[] = "GLOBAL";
    const CHAR8 hwcountry_line[]  = "HwCountry: GLOBAL";
    const INT32 slack_start = 0x7F000;
    const INT32 slack_end   = 0x80000;
    INT32 cursor = find_zero_run(buffer, size, slack_start, slack_end,
                                 (INT32)(strlen(hwcountry_value) + strlen(hwcountry_line) + 2));
    if (cursor < 0) {
        Print_patcher("HwCountry patch: no slack space found in 0x%X-0x%X\n",
                      slack_start, slack_end);
        return -1;
    }
    if (cursor > 0 && buffer[cursor - 1] != 0) cursor++;

    INT64 value_off = ensure_cstring(buffer, size, &cursor, slack_end, hwcountry_value);
    INT64 line_off  = ensure_cstring(buffer, size, &cursor, slack_end, hwcountry_line);
    if (value_off < 0 || line_off < 0) {
        Print_patcher("HwCountry patch: failed to place replacement strings\n");
        return -1;
    }

    if (patch_fixed_adrl_target(buffer, size, 0x1DC38, load_base, 0x106D60, value_off,
                                "HwCountry getvar") != 0) {
        return -1;
    }

    if (patch_first_adrl_by_string(buffer, size, load_base, "HwCountry: %a", line_off,
                                   "HwCountry display") != 0) {
        return -1;
    }

    return 0;
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

BOOLEAN PatchBuffer(CHAR8* data, INT32 size) {
    #ifndef DISABLE_PATCH_1
    if (patch_abl_gbl(data, size) != 0)
        Print_patcher("Warning: Failed to patch ABL GBL\n");
    #endif
    #ifndef DISABLE_PATCH_6
    if (patch_hwcountry_global(data, size, 0) != 0) {
        Print_patcher("Error: Failed to patch HwCountry -> GLOBAL\n");
        free(data);
        return FALSE;
    }
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
    return 1;
}
