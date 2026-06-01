#ifndef ARM64_INST_DECODER_H
#define ARM64_INST_DECODER_H
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
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
    INST_BCOND,             /* B.cond label (B.EQ/NE/LT/GE…) */
} InstType;
#define NOP 0xD503201F  /* NOP is an alias for HINT #0x1F */
typedef struct {
    InstType type;
    uint32_t raw;       /* 原始 32-bit 编码 */
    uint8_t    rt;        /* destination / transfer register (Rd/Rt) */
    uint8_t    rn;        /* first source / base register            */
    uint8_t    rm;        /* second source (move / shift)            */
    uint32_t   imm;       /* decoded unsigned immediate               */
    int32_t    simm;      /* decoded signed immediate (branches etc.) */
    uint8_t    shift;     /* ADD shift (0 or 12)                     */
    uint8_t    cond;      /* condition code for CSET etc.            */
} DecodedInst;

typedef bool (*DecodeFunc)(uint32_t, DecodedInst*);

int32_t decode_imm19(uint32_t raw);
uint32_t read_instr(const char* buf, int32_t off);

void write_instr(char* buf, int32_t off, uint32_t val);

/* ---- LDRB Wt, [Xn, #imm] unsigned offset ---- */
bool decode_inst_ldrb_imm(uint32_t raw, DecodedInst* out);

/* ---- STRB Wt, [Xn, #imm] unsigned offset ---- */
bool decode_inst_strb_imm(uint32_t raw, DecodedInst* out);

/* ---- STRB Wt, [Xn], #simm  (post-index) ---- */
bool decode_inst_strb_post(uint32_t raw, DecodedInst* out);

/* ---- STRB Wt, [Xn, #simm]!  (pre-index) ---- */
bool decode_inst_strb_pre(uint32_t raw, DecodedInst* out);

/* ---- LDR Xt, [Xn, #imm] 64-bit unsigned offset ---- */
bool decode_inst_ldr_x_imm(uint32_t raw, DecodedInst* out);

/* ---- STR Xt, [Xn, #imm] 64-bit unsigned offset ---- */
bool decode_inst_str_x_imm(uint32_t raw, DecodedInst* out);

/* ---- LDR Wt, [Xn, #imm] 32-bit unsigned offset ---- */
bool decode_inst_ldr_w_imm(uint32_t raw, DecodedInst* out);

/* ---- STR Wt, [Xn, #imm] 32-bit unsigned offset ---- */
bool decode_inst_str_w_imm(uint32_t raw, DecodedInst* out);

/* ---- MOV Xd, Xm  (ORR Xd, XZR, Xm) ---- */
bool decode_inst_mov_x(uint32_t raw, DecodedInst* out);

/* ---- MOV Wd, Wm  (ORR Wd, WZR, Wm) ---- */
bool decode_inst_mov_w(uint32_t raw, DecodedInst* out);

/* ---- MOVZ Wd, #imm16 ---- */
bool decode_inst_movz_w(uint32_t raw, DecodedInst* out);

/* ---- ADRP Xd, label ---- */
bool decode_inst_adrp(uint32_t raw, DecodedInst* out);

/* ---- ADD Xd, Xn, #imm{, LSL #0 | LSL #12} ---- */
bool decode_inst_add_x_imm(uint32_t raw, DecodedInst* out);

/* ---- PACIASP / function start hint ---- */
bool decode_inst_paciasp(uint32_t raw, DecodedInst* out);

/* ---- CMP Wn, #imm  (SUBS WZR, Wn, #imm) ---- */
bool decode_inst_cmp_w_imm(uint32_t raw, DecodedInst* out);

/* ---- UBFM Wd, Wn, #immr, #imms ---- */
bool decode_inst_ubfm_w(uint32_t raw, DecodedInst* out);

bool decode_inst_cbz_cbnz(uint32_t raw, DecodedInst* out);

/* ---- B.cond label (B.EQ, B.NE, B.LT, B.GE …) ---- */
bool decode_inst_bcond(uint32_t raw, DecodedInst* out);

bool get_JUMP_target(DecodedInst* inst, int64_t instoff, int64_t* target);
uint32_t change_rt(DecodedInst* inst, uint8_t new_rt);
/* CBZ/CBNZ -> B，保留分支偏移 */
uint32_t change_to_b(uint32_t raw);
/* 解码优先级表 —— 按照编码空间重叠度排序 */
DecodedInst decode_inst(uint32_t raw);

DecodedInst decode_at(const char* buf, int32_t off);

/* 构造 MOV Wd, #imm16 */
uint32_t encode_movz_w(uint8_t rd, uint16_t imm16);

/* STRB: 替换 Rt -> WZR (31) */
uint32_t strb_with_reg(uint32_t raw, uint8_t new_rt);

/* ADRP: 替换 Rd */
uint32_t adrp_with_rd(uint32_t raw, uint8_t new_rd);

/* ADD: 替换 Rd 和 Rn 为同一个寄存器 */
uint32_t add_with_reg(uint32_t raw, uint8_t new_reg);
#endif