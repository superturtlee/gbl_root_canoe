#include "arm64_inst/utils.h"
bool locset_has(const LocSet* s, DataLoc l) {
    for (int32_t i = 0; i < s->count; ++i)
        if (s->locs[i].type == l.type && s->locs[i].val == l.val) return true;
    return false;
}
bool locset_has_reg  (const LocSet* s, int8_t r)   { return locset_has(s, (DataLoc){LOC_REG, r}); }
bool locset_has_stk64(const LocSet* s, uint32_t v) { return locset_has(s, (DataLoc){LOC_STK64, (int32_t)v}); }
bool locset_has_stk8 (const LocSet* s, uint32_t v) { return locset_has(s, (DataLoc){LOC_STK8, (int32_t)v}); }

void locset_add(LocSet* s, DataLoc l) {
    if (!locset_has(s, l) && s->count < 256) s->locs[s->count++] = l;
}
void locset_add_reg  (LocSet* s, int8_t r)   { locset_add(s, (DataLoc){LOC_REG, r}); }
void locset_add_stk64(LocSet* s, uint32_t v) { locset_add(s, (DataLoc){LOC_STK64, (int32_t)v}); }
void locset_add_stk8 (LocSet* s, uint32_t v) { locset_add(s, (DataLoc){LOC_STK8, (int32_t)v}); }

void locset_del(LocSet* s, DataLoc l) {
    for (int32_t i = 0; i < s->count; ++i) {
        if (s->locs[i].type == l.type && s->locs[i].val == l.val) {
            s->locs[i] = s->locs[s->count - 1];
            s->count--;
            return;
        }
    }
}
void locset_del_reg  (LocSet* s, int8_t r)   { locset_del(s, (DataLoc){LOC_REG, r}); }
void locset_del_stk64(LocSet* s, uint32_t v) { locset_del(s, (DataLoc){LOC_STK64, (int32_t)v}); }
void locset_del_stk8 (LocSet* s, uint32_t v) { locset_del(s, (DataLoc){LOC_STK8, (int32_t)v}); }

bool locset_empty(const LocSet* s) { return s->count == 0; }

void locset_print(const LocSet* s) {
    if (s->count == 0) {
        printf("WARRNING: LocSet empty, using fallback. Will Match any LDRB\n");
        return;
    }
    printf("  LocSet{");
    for (int32_t i = 0; i < s->count; ++i) {
        if (i) printf(", ");
        switch (s->locs[i].type) {
            case LOC_REG:   printf("W%d", s->locs[i].val); break;
            case LOC_STK64: printf("[SP+0x%X]/64", s->locs[i].val); break;
            case LOC_STK8:  printf("[SP+0x%X]/8",  s->locs[i].val); break;
        }
    }
    printf("}\n");
}

StrbInfo decode_any_strb(uint32_t raw) {
    StrbInfo info = { false, 0, 0, 0 };
    DecodedInst d = decode_inst(raw);

    if (d.type == INST_STRB_IMM) {
        info.valid = true; info.rt = d.rt; info.rn = d.rn; info.imm = d.imm;
    } else if (d.type == INST_STRB_POST || d.type == INST_STRB_PRE) {
        info.valid = true; info.rt = d.rt; info.rn = d.rn; info.imm = (uint32_t)d.simm & 0x1FF;
    }
    return info;
}

int32_t find_ldrB_instructio_reverse(char* buffer, int32_t size,
                                   int32_t start_offset, int8_t target_register,int32_t* global_var_offset, SourceCallback callback) {
    int32_t now_offset = start_offset - 4;
    int8_t current_target = target_register;
    int32_t bounce_count = 0;
    const int32_t MAX_BOUNCES = 8;

    while (now_offset >= 0) {
        DecodedInst d = decode_at(buffer, now_offset);

        if (d.type == INST_PACIASP) {
            printf("Reached function start at 0x%X\n", now_offset);
            break;
        }

        /* ---- 64-bit 栈 reload 弹跳 ---- */
        if (d.type == INST_LDR_X_IMM && d.rn == 31 && (int8_t)d.rt == current_target) {
            uint32_t spill_imm = d.imm;
            printf("Bounce at 0x%X: LDR X%d,[SP,#0x%X]\n",
                   now_offset, (int)current_target, spill_imm);
            int32_t search = now_offset - 4;
            bool found = false;
            while (search >= 0) {
                DecodedInst ds = decode_at(buffer, search);
                if (ds.type == INST_PACIASP) break;
                if (ds.type == INST_STR_X_IMM && ds.rn == 31 && ds.imm == spill_imm) {
                    printf("  -> STR X%d,[SP,#0x%X] at 0x%X\n",
                           (int32_t)ds.rt, spill_imm, search);
                    current_target = (int8_t)ds.rt;
                    now_offset = search - 4;
                    found = true;
                    bounce_count++;
                    break;
                }
                search -= 4;
            }
            if (!found) { printf("  -> No matching STR, abort\n"); return -1; }
            if (bounce_count > MAX_BOUNCES) { printf("Too many bounces\n"); return -1; }
            continue;
        }

        /* ---- byte 级栈 reload 弹跳 ---- */
        if (d.type == INST_LDRB_IMM && d.rn == 31 && (int8_t)d.rt == current_target) {
            uint32_t byte_imm = d.imm;
            printf("Byte bounce at 0x%X: LDRB W%d,[SP,#0x%X]\n",
                   now_offset, (int)current_target, byte_imm);
            int32_t search = now_offset - 4;
            bool found = false;
            while (search >= 0) {
                DecodedInst ds = decode_at(buffer, search);
                if (ds.type == INST_PACIASP) break;
                if (ds.type == INST_STRB_IMM && ds.rn == 31 && ds.imm == byte_imm) {
                    printf("  -> STRB W%d,[SP,#0x%X] at 0x%X\n",
                           (int32_t)ds.rt, byte_imm, search);
                    current_target = (int8_t)ds.rt;
                    now_offset = search - 4;
                    found = true;
                    bounce_count++;
                    break;
                }
                search -= 4;
            }
            if (!found) { printf("  -> No matching STRB, abort\n"); return -1; }
            if (bounce_count > MAX_BOUNCES) { printf("Too many bounces\n"); return -1; }
            continue;
        }

        /* ---- 真正源头: LDRB W{current_target}, [Xn!=SP, #imm] ---- */
        if (d.type == INST_LDRB_IMM && (int8_t)d.rt == current_target && d.rn != 31) {
            printf("Found source LDRB at 0x%X: LDRB W%d,[X%d,#0x%X](%d bounces)\n",
                   now_offset, d.rt, d.rn, d.imm, bounce_count);
            *global_var_offset = (int32_t)d.imm;
            return callback(buffer, size, now_offset, current_target, start_offset);
        }

        now_offset -= 4;
    }
    return -1;
}
int32_t empty_source_callback(char* buffer, int32_t size, int32_t now_offset, int8_t current_reg_target, int32_t start_offset) {
    return 0;
}
//typedef int32_t (*ForwardCallback)(char* buffer, int32_t size, int32_t now_offset, DecodedInst d, int32_t ancher_offset);
int32_t track_forward(char* buffer, int32_t size, int32_t start_offset,
                                      int8_t src_reg, int32_t ancher_offset, ForwardCallback callback) {
    LocSet set;
    set.count = 0;
    locset_add_reg(&set, src_reg);

    printf("\n=== Forward tracking from LDRB@0x%X (W%d), anchor=0x%X ===\n",
           start_offset, (int)src_reg, ancher_offset);
    locset_print(&set);

    for (int32_t off = start_offset + 4; off < size - 4; off += 4) {
        DecodedInst d = decode_at(buffer, off);

        if (d.type == INST_PACIASP) {
            printf("0x%X: function boundary, stop\n", off);
            break;
        }

        switch (d.type) {

        /* ---- STR Xt, [SP, #imm] 64-bit spill ---- */
        case INST_STR_X_IMM:
            if (d.rn == 31) {
                if (locset_has_reg(&set, (int8_t)d.rt)) {
                    int32_t cb_result = callback(buffer, size, off, d, ancher_offset);
                    if (cb_result!=NEED_MORE) return cb_result;
                    printf("  0x%X: STR X%d,[SP,#0x%X] spill64\n", off, d.rt, d.imm);
                    locset_add_stk64(&set, d.imm);
                    locset_print(&set);
                } else if (locset_has_stk64(&set, d.imm)) {
                    printf("  0x%X: STR X%d,[SP,#0x%X] overwrite stk64 -> del\n", off, d.rt, d.imm);
                    locset_del_stk64(&set, d.imm);
                    locset_print(&set);
                }
            }
            break;

        /* ---- LDR Xt, [SP, #imm] 64-bit reload ---- */
        case INST_LDR_X_IMM:
            if (d.rn == 31) {
                if (locset_has_stk64(&set, d.imm)) {
                    int32_t cb_result = callback(buffer, size, off, d, ancher_offset);
                    if (cb_result!=NEED_MORE) return cb_result;
                    printf("  0x%X: LDR X%d,[SP,#0x%X] reload64\n", off, d.rt, d.imm);
                    locset_add_reg(&set, (int8_t)d.rt);
                    locset_print(&set);
                } else if (locset_has_reg(&set, (int8_t)d.rt)) {
                    printf("  0x%X: LDR X%d,[SP,#0x%X] overwrite reg -> del\n", off, d.rt, d.imm);
                    locset_del_reg(&set, (int8_t)d.rt);
                    locset_print(&set);
                }
            }
            break;

        /* ---- STR Wt, [SP, #imm] 32-bit spill ---- */
        case INST_STR_W_IMM:
            if (d.rn == 31) {
                if (locset_has_reg(&set, (int8_t)d.rt)) {
                    int32_t cb_result = callback(buffer, size, off, d, ancher_offset);
                    if (cb_result!=NEED_MORE) return cb_result;
                    printf("  0x%X: STR W%d,[SP,#0x%X] spill32\n", off, d.rt, d.imm);
                    locset_add_stk64(&set, d.imm);
                    locset_print(&set);
                } else if (locset_has_stk64(&set, d.imm)) {
                    printf("  0x%X: STR W%d,[SP,#0x%X] overwrite stk -> del\n", off, d.rt, d.imm);
                    locset_del_stk64(&set, d.imm);
                    locset_print(&set);
                }
            }
            break;

        /* ---- LDR Wt, [SP, #imm] 32-bit reload ---- */
        case INST_LDR_W_IMM:
            if (d.rn == 31) {
                if (locset_has_stk64(&set, d.imm)) {
                    int32_t cb_result = callback(buffer, size, off, d, ancher_offset);
                    if (cb_result!=NEED_MORE) return cb_result;
                    printf("  0x%X: LDR W%d,[SP,#0x%X] reload32\n", off, d.rt, d.imm);
                    locset_add_reg(&set, (int8_t)d.rt);
                    locset_print(&set);
                } else if (locset_has_reg(&set, (int8_t)d.rt)) {
                    printf("  0x%X: LDR W%d,[SP,#0x%X] overwrite reg -> del\n", off, d.rt, d.imm);
                    locset_del_reg(&set, (int8_t)d.rt);
                    locset_print(&set);
                }
            }
            break;

        /* ---- MOV Xd, Xm ---- */
        case INST_MOV_X:
            if (locset_has_reg(&set, (int8_t)d.rm) && d.rt != 31) {
                int32_t cb_result = callback(buffer, size, off, d, ancher_offset);
                if (cb_result!=NEED_MORE) return cb_result;
                printf("  0x%X: MOV X%d,X%d propagate\n", off, d.rt, d.rm);
                locset_add_reg(&set, (int8_t)d.rt);
                locset_print(&set);
            } else if (locset_has_reg(&set, (int8_t)d.rt)) {
                printf("  0x%X: MOV X%d,X%d overwrite -> del\n", off, d.rt, d.rm);
                locset_del_reg(&set, (int8_t)d.rt);
                locset_print(&set);
            }
            break;

        /* ---- MOV Wd, Wm ---- */
        case INST_MOV_W:
            if (locset_has_reg(&set, (int8_t)d.rm) && d.rt != 31) {
                int32_t cb_result = callback(buffer, size, off, d, ancher_offset);
                if (cb_result!=NEED_MORE) return cb_result;
                printf("  0x%X: MOV W%d,W%d propagate\n", off, d.rt, d.rm);
                locset_add_reg(&set, (int8_t)d.rt);
                locset_print(&set);
            } else if (locset_has_reg(&set, (int8_t)d.rt)) {
                printf("  0x%X: MOV W%d,W%d overwrite -> del\n", off, d.rt, d.rm);
                locset_del_reg(&set, (int8_t)d.rt);
                locset_print(&set);
            }
            break;

        /* ---- STRB (所有形式) ---- */
        case INST_STRB_IMM:
        case INST_STRB_POST:
        case INST_STRB_PRE: {
            StrbInfo si = decode_any_strb(d.raw);
            if (si.valid && (locset_has_reg(&set, (int8_t)si.rt)||(locset_empty(&set)&&off > ancher_offset))) {
                //typedef int32_t (*ForwardCallback)(char* buffer, int32_t size, int32_t now_offset, DecodedInst d, int32_t ancher_offset);
                int32_t cb_result = callback(buffer, size, off, d, ancher_offset);
                if (cb_result!=NEED_MORE) return cb_result;
                printf("  0x%X: STRB W%d,[X%d,#0x%X] -> spill8\n",
                        off, si.rt, si.rn, si.imm);
                if (si.rn == 31) locset_add_stk8(&set, si.imm);
                locset_print(&set);
                
            } else if (si.valid && si.rn == 31 && locset_has_stk8(&set, si.imm)) {
                printf("  0x%X: STRB W%d,[SP,#0x%X] overwrite stk8 -> del\n",
                       off, si.rt, si.imm);
                locset_del_stk8(&set, si.imm);
            }
            break;
        }

        default:
            break;
        }
    }

    printf("Forward tracking: no sink STRB found after anchor 0x%X\n", ancher_offset);
    return FAILURE;
}
bool str_at(const char* buffer, int32_t size, int64_t file_off, const char* needle) {
    if (file_off < 0) return false;
    int32_t len = strlen(needle);
    if ((int32_t)file_off + len >= size) return false;
    return memcmp(buffer + file_off, needle, len) == 0;
}
int64_t calc_adrl_file_offset(const char* buffer, int32_t adrp_off, uint64_t load_base) {
    DecodedInst d0 = decode_at(buffer, adrp_off);
    DecodedInst d1 = decode_at(buffer, adrp_off + 4);

    if (d0.type != INST_ADRP) return -1;
    if (d1.type != INST_ADD_X_IMM) return -1;
    if (d1.rt != d0.rt || d1.rn != d0.rt) return -1;

    uint64_t pc      = load_base + (uint64_t)adrp_off;
    uint64_t page_pc = pc & ~0xFFFull;
    uint64_t target_va = (uint64_t)((int64_t)page_pc + d0.simm) + d1.imm;
    return (int64_t)(target_va - load_base);
}