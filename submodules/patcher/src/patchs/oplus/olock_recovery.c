#include "patchs/oplus/olock_recovery.h"
#include "arm64_inst/utils.h"

/*
 * OPlus ABL contains a proprietary "olock" (secure lock state) check at the
 * recovery boot path, completely separate from AVB lock state (offset 0x3C5).
 * It reads a flag at struct offset 0xA6C and explicitly blocks recovery boot
 * when the device reports as locked, leading to an infinite loop.
 *
 * The unique anchor for this block is the instruction sequence:
 *   CMP  W8,  #1
 *   CSET Wx,  EQ      ; <-- anchor - 8
 *   CMP  W9,  #1
 *   CCMP W8,  #1, #0, EQ  ; <-- anchor (bytes: 00 09 41 7A)
 *   B.NE <allow_path>     ; <-- anchor + 4  (falls through to "Not allow Recovery")
 *
 * Two patches are applied:
 *  1. CSET Wx, EQ  --> MOV Wx, WZR  (force register to 0 so CBZ always skips fastboot block)
 *  2. B.NE target  --> B target      (unconditionally jump to the allowed recovery path)
 */

/* CCMP W8, #1, #0, EQ  =  00 09 41 7A (little-endian) */
static const uint8_t CCMP_ANCHOR[4] = { 0x00, 0x09, 0x41, 0x7A };

/* CMP W8, #1  =  1F 05 00 71 */
static const uint8_t CMP_W8_1[4]    = { 0x1F, 0x05, 0x00, 0x71 };

/* CMP W9, #1  =  3F 05 00 71 */
static const uint8_t CMP_W9_1[4]    = { 0x3F, 0x05, 0x00, 0x71 };

static bool bytes_match(const char* buf, int32_t off, const uint8_t* pattern, int n) {
    for (int i = 0; i < n; i++)
        if ((uint8_t)buf[off + i] != pattern[i]) return false;
    return true;
}

bool patch_olock_recovery(char* buffer, int32_t size) {
    if (size < 20) return false;

    for (int32_t i = 0; i <= size - 20; i += 4) {
        /* Locate CCMP W8,#1,#0,EQ anchor */
        if (!bytes_match(buffer, i, CCMP_ANCHOR, 4)) continue;

        /* Validate: CMP W8,#1  at i-12 */
        if (i < 12) continue;
        if (!bytes_match(buffer, i - 12, CMP_W8_1, 4)) continue;

        /* Validate: CMP W9,#1  at i-4 */
        if (!bytes_match(buffer, i - 4, CMP_W9_1, 4)) continue;

        /* Validate: CSET Wx,EQ at i-8: encoding 0x1A9F17xx (xx encodes Rd in [4:0]) */
        uint32_t cset_raw = (uint8_t)buffer[i-8]
                          | ((uint8_t)buffer[i-7] << 8)
                          | ((uint8_t)buffer[i-6] << 16)
                          | ((uint8_t)buffer[i-5] << 24);
        if ((cset_raw & 0xFFFFFFE0) != 0x1A9F17E0) {
            /* 0x1A9F17E0 with Rd masked to 0; any Rd in low 5 bits is valid */
            /* mask: top 27 bits must match 0x1A9F17E0>>5 = 0x0D4F8BF */
            if ((cset_raw >> 5) != (0x1A9F17E0 >> 5)) continue;
        }

        /* Validate: B.NE at i+4: byte[3]=0x54, condition bits [3:0]=0x01 (NE) */
        if (i + 8 > size) continue;
        uint8_t bne_b3 = (uint8_t)buffer[i + 7];
        uint8_t bne_b0 = (uint8_t)buffer[i + 4];
        if (bne_b3 != 0x54) continue;
        if ((bne_b0 & 0x0F) != 0x01) continue; /* condition NE = 0x1 */

        printf("OLock recovery block found at anchor 0x%X\n", i);

        /* Patch 1: CSET Wx,EQ  -->  MOV Wx,WZR  (ORR Wx,WZR,WZR = 0x2A1F03E0|Rd) */
        uint8_t rd = (uint8_t)(cset_raw & 0x1F);
        uint32_t mov_wzr = 0x2A1F03E0 | rd;
        write_instr(buffer, i - 8, mov_wzr);
        printf("  0x%X: CSET W%d,EQ --> MOV W%d,WZR\n", i - 8, rd, rd);

        /* Patch 2: B.NE target  -->  B target (unconditional) */
        uint32_t bne_raw = (uint8_t)buffer[i+4]
                         | ((uint8_t)buffer[i+5] << 8)
                         | ((uint8_t)buffer[i+6] << 16)
                         | ((uint8_t)buffer[i+7] << 24);
        /* B.NE: bits[23:5] = imm19; B: 0x14000000 | imm26; imm19 becomes imm26 */
        int32_t imm19 = (int32_t)(bne_raw << 8) >> 13; /* sign-extend imm19 */
        uint32_t b_uncond = 0x14000000 | ((uint32_t)(imm19) & 0x03FFFFFF);
        write_instr(buffer, i + 4, b_uncond);
        printf("  0x%X: B.NE --> B (unconditional allow recovery)\n", i + 4);

        return true;
    }

    printf("OPlus olock recovery block not found\n");
    return false;
}
