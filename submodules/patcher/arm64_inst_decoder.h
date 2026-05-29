#ifndef ARM64_INST_DECODER_H
#define ARM64_INST_DECODER_H
#include "types.h"
typedef enum {
    INST_UNKNOWN = 0,

    /* loads / stores - byte */
    INST_LDRB_IMM,          /* LDRB Wt, [Xn, #imm]         unsigned offset */
    INST_STRB_IMM,          /* STRB Wt, [Xn, #imm]         unsigned offset */
    INST_STRB_POST,         /* STRB Wt, [Xn], #simm        post-index     */
    INST_STRB_PRE,          /* STRB Wt, [Xn, #simm]!       pre-index      */

    /* loads / stores - 32-bit word */
    INST_LDR_W_IMM,         /* LDR Wt, [Xn, #imm]          unsigned offset */
    INST_STR_W_IMM,         /* STR Wt, [Xn, #imm]          unsigned offset */

    /* loads / stores - 64-bit */
    INST_LDR_X_IMM,         /* LDR Xt, [Xn, #imm]          unsigned offset */
    INST_STR_X_IMM,         /* STR Xt, [Xn, #imm]          unsigned offset */

    /* moves */
    INST_MOV_X,             /* MOV Xd, Xm   (ORR Xd, XZR, Xm) */
    INST_MOV_W,             /* MOV Wd, Wm   (ORR Wd, WZR, Wm) */
    INST_MOVZ_W,            /* MOVZ Wd, #imm16              */

    /* address generation */
    INST_ADRP,              /* ADRP Xd, label               */
    INST_ADD_X_IMM,         /* ADD Xd, Xn, #imm  (no shift / LSL#12) */

    /* misc */
    INST_PACIASP,           /* PACIASP / function-start hint 0xD503233F */
    INST_CBZ_W,             /* CBZ Wt, label                 */
    INST_CBNZ_W,            /* CBNZ Wt, label                */
    INST_B,                 /* B label                       */
    INST_BL,                /* BL label                      */
    INST_RET,               /* RET                           */
    INST_NOP,               /* NOP                           */
    INST_CMP_W_IMM,         /* CMP Wn, #imm  (SUBS WZR)     */
    INST_CSET_W,            /* CSET Wd, cond (CSINC)         */
    INST_UBFM_W,            /* UBFM Wd, Wn, #r, #s          */
    INST_CBNZ_X,            /* CBNZ Xt, label                */
    INST_CBZ_X,             /* CBZ Xt, label                 */
} InstType;
#define NOP 0xD503201F  /* NOP is an alias for HINT #0x1F */
typedef struct {
    InstType type;
    UINT32   raw;       /* 原始 32-bit 编码 */
    UINT8    rt;        /* destination / transfer register (Rd/Rt) */
    UINT8    rn;        /* first source / base register            */
    UINT8    rm;        /* second source (move / shift)            */
    UINT32   imm;       /* decoded unsigned immediate               */
    INT32    simm;      /* decoded signed immediate (branches etc.) */
    UINT8    shift;     /* ADD shift (0 or 12)                     */
    UINT8    cond;      /* condition code for CSET etc.            */
} DecodedInst;
static INT32 decode_imm19(UINT32 raw) {
    INT32 imm19 = (INT32)((raw >> 5) & 0x7FFFF);
    if (imm19 & 0x40000)
        imm19 |= (INT32)0xFFF80000; // sign extend bit 18 -> bit 31
    return imm19 << 2;
}
static UINT32 read_instr(const CHAR8* buf, INT32 off) {
    return (UINT8)buf[off]
         | ((UINT8)buf[off+1] << 8)
         | ((UINT8)buf[off+2] << 16)
         | ((UINT8)buf[off+3] << 24);
}

static void write_instr(CHAR8* buf, INT32 off, UINT32 val) {
    buf[off]   = (char)(val & 0xFF);
    buf[off+1] = (char)((val >> 8)  & 0xFF);
    buf[off+2] = (char)((val >> 16) & 0xFF);
    buf[off+3] = (char)((val >> 24) & 0xFF);
}

/* ---- LDRB Wt, [Xn, #imm] unsigned offset ---- */
static BOOLEAN decode_inst_ldrb_imm(UINT32 raw, DecodedInst* out) {
    if ((raw & 0xFFC00000) != 0x39400000) return FALSE;
    out->type = INST_LDRB_IMM;
    out->raw  = raw;
    out->rt   = raw & 0x1F;
    out->rn   = (raw >> 5) & 0x1F;
    out->imm  = (raw >> 10) & 0xFFF;
    return TRUE;
}

/* ---- STRB Wt, [Xn, #imm] unsigned offset ---- */
static BOOLEAN decode_inst_strb_imm(UINT32 raw, DecodedInst* out) {
    if ((raw & 0xFFC00000) != 0x39000000) return FALSE;
    out->type = INST_STRB_IMM;
    out->raw  = raw;
    out->rt   = raw & 0x1F;
    out->rn   = (raw >> 5) & 0x1F;
    out->imm  = (raw >> 10) & 0xFFF;
    return TRUE;
}

/* ---- STRB Wt, [Xn], #simm  (post-index) ---- */
static BOOLEAN decode_inst_strb_post(UINT32 raw, DecodedInst* out) {
    if ((raw & 0xFFE00C00) != 0x38000000) return FALSE;
    /* bit[11]=0, bit[10]=0 -> post-index */
    if ((raw & 0xC00) != 0x000) return FALSE;
    out->type = INST_STRB_POST;
    out->raw  = raw;
    out->rt   = raw & 0x1F;
    out->rn   = (raw >> 5) & 0x1F;
    INT32 imm9 = (raw >> 12) & 0x1FF;
    if (imm9 & 0x100) imm9 |= ~0x1FF;  /* sign extend */
    out->simm = imm9;
    out->imm  = (UINT32)imm9;
    return TRUE;
}

/* ---- STRB Wt, [Xn, #simm]!  (pre-index) ---- */
static BOOLEAN decode_inst_strb_pre(UINT32 raw, DecodedInst* out) {
    if ((raw & 0xFFE00C00) != 0x38000C00) return FALSE;
    out->type = INST_STRB_PRE;
    out->raw  = raw;
    out->rt   = raw & 0x1F;
    out->rn   = (raw >> 5) & 0x1F;
    INT32 imm9 = (raw >> 12) & 0x1FF;
    if (imm9 & 0x100) imm9 |= ~0x1FF;
    out->simm = imm9;
    out->imm  = (UINT32)imm9;
    return TRUE;
}

/* ---- LDR Xt, [Xn, #imm] 64-bit unsigned offset ---- */
static BOOLEAN decode_inst_ldr_x_imm(UINT32 raw, DecodedInst* out) {
    if ((raw & 0xFFC00000) != 0xF9400000) return FALSE;
    out->type = INST_LDR_X_IMM;
    out->raw  = raw;
    out->rt   = raw & 0x1F;
    out->rn   = (raw >> 5) & 0x1F;
    out->imm  = ((raw >> 10) & 0xFFF) << 3;
    return TRUE;
}

/* ---- STR Xt, [Xn, #imm] 64-bit unsigned offset ---- */
static BOOLEAN decode_inst_str_x_imm(UINT32 raw, DecodedInst* out) {
    if ((raw & 0xFFC00000) != 0xF9000000) return FALSE;
    out->type = INST_STR_X_IMM;
    out->raw  = raw;
    out->rt   = raw & 0x1F;
    out->rn   = (raw >> 5) & 0x1F;
    out->imm  = ((raw >> 10) & 0xFFF) << 3;
    return TRUE;
}

/* ---- LDR Wt, [Xn, #imm] 32-bit unsigned offset ---- */
static BOOLEAN decode_inst_ldr_w_imm(UINT32 raw, DecodedInst* out) {
    if ((raw & 0xFFC00000) != 0xB9400000) return FALSE;
    out->type = INST_LDR_W_IMM;
    out->raw  = raw;
    out->rt   = raw & 0x1F;
    out->rn   = (raw >> 5) & 0x1F;
    out->imm  = ((raw >> 10) & 0xFFF) << 2;
    return TRUE;
}

/* ---- STR Wt, [Xn, #imm] 32-bit unsigned offset ---- */
static BOOLEAN decode_inst_str_w_imm(UINT32 raw, DecodedInst* out) {
    if ((raw & 0xFFC00000) != 0xB9000000) return FALSE;
    out->type = INST_STR_W_IMM;
    out->raw  = raw;
    out->rt   = raw & 0x1F;
    out->rn   = (raw >> 5) & 0x1F;
    out->imm  = ((raw >> 10) & 0xFFF) << 2;
    return TRUE;
}

/* ---- MOV Xd, Xm  (ORR Xd, XZR, Xm) ---- */
static BOOLEAN decode_inst_mov_x(UINT32 raw, DecodedInst* out) {
    if ((raw & 0xFFE0FFE0) != 0xAA0003E0) return FALSE;
    out->type = INST_MOV_X;
    out->raw  = raw;
    out->rt   = raw & 0x1F;          /* Rd */
    out->rm   = (raw >> 16) & 0x1F;  /* Rm */
    out->rn   = 31;                  /* XZR implied */
    return TRUE;
}

/* ---- MOV Wd, Wm  (ORR Wd, WZR, Wm) ---- */
static BOOLEAN decode_inst_mov_w(UINT32 raw, DecodedInst* out) {
    if ((raw & 0xFFE0FFE0) != 0x2A0003E0) return FALSE;
    out->type = INST_MOV_W;
    out->raw  = raw;
    out->rt   = raw & 0x1F;
    out->rm   = (raw >> 16) & 0x1F;
    out->rn   = 31;
    return TRUE;
}

/* ---- MOVZ Wd, #imm16 ---- */
static BOOLEAN decode_inst_movz_w(UINT32 raw, DecodedInst* out) {
    if ((raw & 0xFF800000) != 0x52800000) return FALSE;
    out->type = INST_MOVZ_W;
    out->raw  = raw;
    out->rt   = raw & 0x1F;
    out->imm  = (raw >> 5) & 0xFFFF;
    out->shift = (UINT8)(((raw >> 21) & 0x3) * 16);
    return TRUE;
}

/* ---- ADRP Xd, label ---- */
static BOOLEAN decode_inst_adrp(UINT32 raw, DecodedInst* out) {
    if ((raw & 0x9F000000) != 0x90000000) return FALSE;
    out->type = INST_ADRP;
    out->raw  = raw;
    out->rt   = raw & 0x1F;
    UINT64 immlo = (raw >> 29) & 0x3;
    UINT64 immhi = (raw >> 5)  & 0x7FFFF;
    INT64 imm = (INT64)((immhi << 2) | immlo);
    if (imm & (1LL << 20)) imm |= ~((1LL << 21) - 1);
    out->simm = (INT32)(imm << 12);  /* page-granular signed offset */
    return TRUE;
}

/* ---- ADD Xd, Xn, #imm{, LSL #0 | LSL #12} ---- */
static BOOLEAN decode_inst_add_x_imm(UINT32 raw, DecodedInst* out) {
    /* SF=1, op=0, S=0 => 0x91000000 base with possible shift */
    if ((raw & 0x7F800000) != 0x11000000) return FALSE;  /* 32-bit ADD check */
    /* We want 64-bit: bit31=1 */
    if (!(raw & 0x80000000u)) return FALSE;
    out->type  = INST_ADD_X_IMM;
    out->raw   = raw;
    out->rt    = raw & 0x1F;
    out->rn    = (raw >> 5) & 0x1F;
    out->imm   = (raw >> 10) & 0xFFF;
    out->shift = (raw & 0x00400000) ? 12 : 0;
    if (out->shift == 12) out->imm <<= 12;
    return TRUE;
}

/* ---- PACIASP / function start hint ---- */
static BOOLEAN decode_inst_paciasp(UINT32 raw, DecodedInst* out) {
    if (raw != 0xD503233F) return FALSE;
    out->type = INST_PACIASP;
    out->raw  = raw;
    return TRUE;
}

/* ---- CMP Wn, #imm  (SUBS WZR, Wn, #imm) ---- */
static BOOLEAN decode_inst_cmp_w_imm(UINT32 raw, DecodedInst* out) {
    /* 0x71000000 mask: sf=0, op=1, S=1 => 0x71... and Rd=11111 */
    if ((raw & 0xFF80001F) != 0x7100001F) return FALSE;
    out->type = INST_CMP_W_IMM;
    out->raw  = raw;
    out->rn   = (raw >> 5) & 0x1F;
    out->imm  = (raw >> 10) & 0xFFF;
    out->rt   = 31; /* WZR */
    return TRUE;
}

/* ---- UBFM Wd, Wn, #immr, #imms ---- */
static BOOLEAN decode_inst_ubfm_w(UINT32 raw, DecodedInst* out) {
    if ((raw & 0xFFC00000) != 0x53000000) return FALSE;
    out->type = INST_UBFM_W;
    out->raw  = raw;
    out->rt   = raw & 0x1F;
    out->rn   = (raw >> 5) & 0x1F;
    /* imms in [15:10], immr in [21:16] */
    out->imm  = raw; /* caller can extract fields as needed */
    return TRUE;
}

static BOOLEAN decode_inst_cbz_cbnz(UINT32 raw, DecodedInst* out) {
    // bits [30:25] = 011010 是 CBZ/CBNZ 家族共有的
    if ((raw & 0x7E000000) != 0x34000000) return FALSE;

    BOOLEAN is64  = (raw >> 31) & 1;  // bit 31: sf
    BOOLEAN isNZ  = (raw >> 24) & 1;  // bit 24: 0=CBZ, 1=CBNZ

    if (isNZ)
        out->type = is64 ? INST_CBNZ_X : INST_CBNZ_W;
    else
        out->type = is64 ? INST_CBZ_X : INST_CBZ_W;

    out->raw  = raw;
    out->rt   = raw & 0x1F;
    out->simm = decode_imm19(raw);

    return TRUE;
}

static BOOLEAN get_JUMP_target(DecodedInst* inst, INT64 instoff, INT64* target) {
    switch (inst->type) {
        case INST_B:
        case INST_BL:
        case INST_CBZ_W:
        case INST_CBZ_X:
        case INST_CBNZ_W:
        case INST_CBNZ_X:
            *target = instoff + inst->simm;
            return TRUE;
        default:
            return FALSE;
    }
}
static UINT32 change_rt(DecodedInst* inst, UINT8 new_rt) {
    return (inst->raw & ~0x1Fu) | (new_rt & 0x1Fu);
}
/* 解码优先级表 —— 按照编码空间重叠度排序 */
typedef BOOLEAN (*DecodeFunc)(UINT32, DecodedInst*);

static const DecodeFunc g_decoders[] = {
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
};

static const INT32 g_num_decoders = (INT32)(sizeof(g_decoders) / sizeof(g_decoders[0]));

static DecodedInst decode_inst(UINT32 raw) {
    DecodedInst d;
    memset(&d, 0, sizeof(d));
    d.type = INST_UNKNOWN;
    d.raw  = raw;
    for (INT32 i = 0; i < g_num_decoders; ++i) {
        if (g_decoders[i](raw, &d)) return d;
    }
    /* 提取通用字段即使未知 */
    d.rt = raw & 0x1F;
    d.rn = (raw >> 5) & 0x1F;
    d.rm = (raw >> 16) & 0x1F;
    return d;
}

static DecodedInst decode_at(const CHAR8* buf, INT32 off) {
    return decode_inst(read_instr(buf, off));
}

/* 构造 MOV Wd, #imm16 */
static UINT32 encode_movz_w(UINT8 rd, UINT16 imm16) {
    return 0x52800000u | ((UINT32)imm16 << 5) | (rd & 0x1Fu);
}

/* STRB: 替换 Rt -> WZR (31) */
static UINT32 strb_with_reg(UINT32 raw, UINT8 new_rt) {
    return (raw & ~0x1Fu) | (new_rt & 0x1Fu);
}

/* ADRP: 替换 Rd */
static UINT32 adrp_with_rd(UINT32 raw, UINT8 new_rd) {
    return (raw & ~0x1Fu) | (new_rd & 0x1Fu);
}

/* ADD: 替换 Rd 和 Rn 为同一个寄存器 */
static UINT32 add_with_reg(UINT32 raw, UINT8 new_reg) {
    raw = (raw & ~0x1Fu)        | (new_reg & 0x1Fu);
    raw = (raw & ~(0x1Fu << 5)) | ((UINT32)(new_reg & 0x1Fu) << 5);
    return raw;
}
#endif