#include "arm64_inst/arm64_inst_decoder.h"
typedef bool (*DecodeFunc)(uint32_t, DecodedInst*);
const DecodeFunc g_decoders[] = {
    decode_inst_paciasp,
    decode_inst_adrp,
    decode_inst_add_x_imm,
    decode_inst_ldr_x_imm,
    decode_inst_str_x_imm,
    decode_inst_ldr_w_imm,
    decode_inst_str_w_imm,
    decode_inst_ldrb_imm,
    decode_inst_strb_imm,
    decode_inst_strb_post,
    decode_inst_strb_pre,
    decode_inst_mov_x,
    decode_inst_mov_w,
    decode_inst_movz_w,
    decode_inst_cmp_w_imm,
    decode_inst_ubfm_w,
    decode_inst_cbz_cbnz,
    decode_inst_bcond,
};
const int32_t g_num_decoders = (int32_t)(sizeof(g_decoders) / sizeof(g_decoders[0]));
int32_t decode_imm19(uint32_t raw) {
    int32_t imm19 = (int32_t)((raw >> 5) & 0x7FFFF);
    if (imm19 & 0x40000)
        imm19 |= (int32_t)0xFFF80000; // sign extend bit 18 -> bit 31
    return imm19 << 2;
}
uint32_t read_instr(const char* buf, int32_t off) {
    return (uint8_t)buf[off]
         | ((uint8_t)buf[off+1] << 8)
         | ((uint8_t)buf[off+2] << 16)
         | ((uint8_t)buf[off+3] << 24);
}

void write_instr(char* buf, int32_t off, uint32_t val) {
    buf[off]   = (char)(val & 0xFF);
    buf[off+1] = (char)((val >> 8)  & 0xFF);
    buf[off+2] = (char)((val >> 16) & 0xFF);
    buf[off+3] = (char)((val >> 24) & 0xFF);
}

/* ---- LDRB Wt, [Xn, #imm] unsigned offset ---- */
bool decode_inst_ldrb_imm(uint32_t raw, DecodedInst* out) {
    if ((raw & 0xFFC00000) != 0x39400000) return false;
    out->type = INST_LDRB_IMM;
    out->raw  = raw;
    out->rt   = raw & 0x1F;
    out->rn   = (raw >> 5) & 0x1F;
    out->imm  = (raw >> 10) & 0xFFF;
    return true;
}

/* ---- STRB Wt, [Xn, #imm] unsigned offset ---- */
bool decode_inst_strb_imm(uint32_t raw, DecodedInst* out) {
    if ((raw & 0xFFC00000) != 0x39000000) return false;
    out->type = INST_STRB_IMM;
    out->raw  = raw;
    out->rt   = raw & 0x1F;
    out->rn   = (raw >> 5) & 0x1F;
    out->imm  = (raw >> 10) & 0xFFF;
    return true;
}

/* ---- STRB Wt, [Xn], #simm  (post-index) ---- */
bool decode_inst_strb_post(uint32_t raw, DecodedInst* out) {
    if ((raw & 0xFFE00C00) != 0x38000000) return false;
    /* bit[11]=0, bit[10]=0 -> post-index */
    if ((raw & 0xC00) != 0x000) return false;
    out->type = INST_STRB_POST;
    out->raw  = raw;
    out->rt   = raw & 0x1F;
    out->rn   = (raw >> 5) & 0x1F;
    int32_t imm9 = (raw >> 12) & 0x1FF;
    if (imm9 & 0x100) imm9 |= ~0x1FF;  /* sign extend */
    out->simm = imm9;
    out->imm  = (uint32_t)imm9;
    return true;
}

/* ---- STRB Wt, [Xn, #simm]!  (pre-index) ---- */
bool decode_inst_strb_pre(uint32_t raw, DecodedInst* out) {
    if ((raw & 0xFFE00C00) != 0x38000C00) return false;
    out->type = INST_STRB_PRE;
    out->raw  = raw;
    out->rt   = raw & 0x1F;
    out->rn   = (raw >> 5) & 0x1F;
    int32_t imm9 = (raw >> 12) & 0x1FF;
    if (imm9 & 0x100) imm9 |= ~0x1FF;
    out->simm = imm9;
    out->imm  = (uint32_t)imm9;
    return true;
}

/* ---- LDR Xt, [Xn, #imm] 64-bit unsigned offset ---- */
bool decode_inst_ldr_x_imm(uint32_t raw, DecodedInst* out) {
    if ((raw & 0xFFC00000) != 0xF9400000) return false;
    out->type = INST_LDR_X_IMM;
    out->raw  = raw;
    out->rt   = raw & 0x1F;
    out->rn   = (raw >> 5) & 0x1F;
    out->imm  = ((raw >> 10) & 0xFFF) << 3;
    return true;
}

/* ---- STR Xt, [Xn, #imm] 64-bit unsigned offset ---- */
bool decode_inst_str_x_imm(uint32_t raw, DecodedInst* out) {
    if ((raw & 0xFFC00000) != 0xF9000000) return false;
    out->type = INST_STR_X_IMM;
    out->raw  = raw;
    out->rt   = raw & 0x1F;
    out->rn   = (raw >> 5) & 0x1F;
    out->imm  = ((raw >> 10) & 0xFFF) << 3;
    return true;
}

/* ---- LDR Wt, [Xn, #imm] 32-bit unsigned offset ---- */
bool decode_inst_ldr_w_imm(uint32_t raw, DecodedInst* out) {
    if ((raw & 0xFFC00000) != 0xB9400000) return false;
    out->type = INST_LDR_W_IMM;
    out->raw  = raw;
    out->rt   = raw & 0x1F;
    out->rn   = (raw >> 5) & 0x1F;
    out->imm  = ((raw >> 10) & 0xFFF) << 2;
    return true;
}

/* ---- STR Wt, [Xn, #imm] 32-bit unsigned offset ---- */
bool decode_inst_str_w_imm(uint32_t raw, DecodedInst* out) {
    if ((raw & 0xFFC00000) != 0xB9000000) return false;
    out->type = INST_STR_W_IMM;
    out->raw  = raw;
    out->rt   = raw & 0x1F;
    out->rn   = (raw >> 5) & 0x1F;
    out->imm  = ((raw >> 10) & 0xFFF) << 2;
    return true;
}

/* ---- MOV Xd, Xm  (ORR Xd, XZR, Xm) ---- */
bool decode_inst_mov_x(uint32_t raw, DecodedInst* out) {
    if ((raw & 0xFFE0FFE0) != 0xAA0003E0) return false;
    out->type = INST_MOV_X;
    out->raw  = raw;
    out->rt   = raw & 0x1F;          /* Rd */
    out->rm   = (raw >> 16) & 0x1F;  /* Rm */
    out->rn   = 31;                  /* XZR implied */
    return true;
}

/* ---- MOV Wd, Wm  (ORR Wd, WZR, Wm) ---- */
bool decode_inst_mov_w(uint32_t raw, DecodedInst* out) {
    if ((raw & 0xFFE0FFE0) != 0x2A0003E0) return false;
    out->type = INST_MOV_W;
    out->raw  = raw;
    out->rt   = raw & 0x1F;
    out->rm   = (raw >> 16) & 0x1F;
    out->rn   = 31;
    return true;
}

/* ---- MOVZ Wd, #imm16 ---- */
bool decode_inst_movz_w(uint32_t raw, DecodedInst* out) {
    if ((raw & 0xFF800000) != 0x52800000) return false;
    out->type = INST_MOVZ_W;
    out->raw  = raw;
    out->rt   = raw & 0x1F;
    out->imm  = (raw >> 5) & 0xFFFF;
    out->shift = (uint8_t)(((raw >> 21) & 0x3) * 16);
    return true;
}

/* ---- ADRP Xd, label ---- */
bool decode_inst_adrp(uint32_t raw, DecodedInst* out) {
    if ((raw & 0x9F000000) != 0x90000000) return false;
    out->type = INST_ADRP;
    out->raw  = raw;
    out->rt   = raw & 0x1F;
    uint64_t immlo = (raw >> 29) & 0x3;
    uint64_t immhi = (raw >> 5)  & 0x7FFFF;
    int64_t imm = (int64_t)((immhi << 2) | immlo);
    if (imm & (1LL << 20)) imm |= ~((1LL << 21) - 1);
    out->simm = (int32_t)(imm << 12);  /* page-granular signed offset */
    return true;
}

/* ---- ADD Xd, Xn, #imm{, LSL #0 | LSL #12} ---- */
bool decode_inst_add_x_imm(uint32_t raw, DecodedInst* out) {
    /* SF=1, op=0, S=0 => 0x91000000 base with possible shift */
    if ((raw & 0x7F800000) != 0x11000000) return false;  /* 32-bit ADD check */
    /* We want 64-bit: bit31=1 */
    if (!(raw & 0x80000000u)) return false;
    out->type  = INST_ADD_X_IMM;
    out->raw   = raw;
    out->rt    = raw & 0x1F;
    out->rn    = (raw >> 5) & 0x1F;
    out->imm   = (raw >> 10) & 0xFFF;
    out->shift = (raw & 0x00400000) ? 12 : 0;
    if (out->shift == 12) out->imm <<= 12;
    return true;
}

/* ---- PACIASP / function start hint ---- */
bool decode_inst_paciasp(uint32_t raw, DecodedInst* out) {
    if (raw != 0xD503233F) return false;
    out->type = INST_PACIASP;
    out->raw  = raw;
    return true;
}

/* ---- CMP Wn, #imm  (SUBS WZR, Wn, #imm) ---- */
bool decode_inst_cmp_w_imm(uint32_t raw, DecodedInst* out) {
    /* 0x71000000 mask: sf=0, op=1, S=1 => 0x71... and Rd=11111 */
    if ((raw & 0xFF80001F) != 0x7100001F) return false;
    out->type = INST_CMP_W_IMM;
    out->raw  = raw;
    out->rn   = (raw >> 5) & 0x1F;
    out->imm  = (raw >> 10) & 0xFFF;
    out->rt   = 31; /* WZR */
    return true;
}

/* ---- UBFM Wd, Wn, #immr, #imms ---- */
bool decode_inst_ubfm_w(uint32_t raw, DecodedInst* out) {
    if ((raw & 0xFFC00000) != 0x53000000) return false;
    out->type = INST_UBFM_W;
    out->raw  = raw;
    out->rt   = raw & 0x1F;
    out->rn   = (raw >> 5) & 0x1F;
    /* imms in [15:10], immr in [21:16] */
    out->imm  = raw; /* caller can extract fields as needed */
    return true;
}

bool decode_inst_cbz_cbnz(uint32_t raw, DecodedInst* out) {
    // bits [30:25] = 011010 是 CBZ/CBNZ 家族共有的
    if ((raw & 0x7E000000) != 0x34000000) return false;

    bool is64  = (raw >> 31) & 1;  // bit 31: sf
    bool isNZ  = (raw >> 24) & 1;  // bit 24: 0=CBZ, 1=CBNZ

    if (isNZ)
        out->type = is64 ? INST_CBNZ_X : INST_CBNZ_W;
    else
        out->type = is64 ? INST_CBZ_X : INST_CBZ_W;

    out->raw  = raw;
    out->rt   = raw & 0x1F;
    out->simm = decode_imm19(raw);

    return true;
}

bool decode_inst_bcond(uint32_t raw, DecodedInst* out) {
    if ((raw & 0xFF000010) != 0x54000000) return false;
    out->type = INST_BCOND;
    out->raw  = raw;
    out->cond = raw & 0xF;
    out->simm = decode_imm19(raw);
    return true;
}

bool get_JUMP_target(DecodedInst* inst, int64_t instoff, int64_t* target) {
    switch (inst->type) {
        case INST_B:
        case INST_BL:
        case INST_CBZ_W:
        case INST_CBZ_X:
        case INST_CBNZ_W:
        case INST_CBNZ_X:
        case INST_BCOND:
            *target = instoff + inst->simm;
            return true;
        default:
            return false;
    }
}
uint32_t change_rt(DecodedInst* inst, uint8_t new_rt) {
    return (inst->raw & ~0x1Fu) | (new_rt & 0x1Fu);
}
uint32_t change_to_b(uint32_t raw) {
    int32_t imm19 = (int32_t)((raw >> 5) & 0x7FFFF);
    if (imm19 & 0x40000) imm19 |= (int32_t)0xFFF80000;
    return 0x14000000u | ((uint32_t)imm19 & 0x03FFFFFFu);
}
/* 解码优先级表 —— 按照编码空间重叠度排序 */
DecodedInst decode_inst(uint32_t raw) {
    DecodedInst d;
    memset(&d, 0, sizeof(d));
    d.type = INST_UNKNOWN;
    d.raw  = raw;
    for (int32_t i = 0; i < g_num_decoders; ++i) {
        if (g_decoders[i](raw, &d)) return d;
    }
    /* 提取通用字段即使未知 */
    d.rt = raw & 0x1F;
    d.rn = (raw >> 5) & 0x1F;
    d.rm = (raw >> 16) & 0x1F;
    return d;
}

DecodedInst decode_at(const char* buf, int32_t off) {
    return decode_inst(read_instr(buf, off));
}

/* 构造 MOV Wd, #imm16 */
uint32_t encode_movz_w(uint8_t rd, uint16_t imm16) {
    return 0x52800000u | ((uint32_t)imm16 << 5) | (rd & 0x1Fu);
}

/* STRB: 替换 Rt -> WZR (31) */
uint32_t strb_with_reg(uint32_t raw, uint8_t new_rt) {
    return (raw & ~0x1Fu) | (new_rt & 0x1Fu);
}

/* ADRP: 替换 Rd */
uint32_t adrp_with_rd(uint32_t raw, uint8_t new_rd) {
    return (raw & ~0x1Fu) | (new_rd & 0x1Fu);
}

/* ADD: 替换 Rd 和 Rn 为同一个寄存器 */
uint32_t add_with_reg(uint32_t raw, uint8_t new_reg) {
    raw = (raw & ~0x1Fu)        | (new_reg & 0x1Fu);
    raw = (raw & ~(0x1Fu << 5)) | ((uint32_t)(new_reg & 0x1Fu) << 5);
    return raw;
}