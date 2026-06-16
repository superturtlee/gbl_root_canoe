#include "patchs/oplus/forceenablefastboot.h"
#include "arm64_inst/utils.h"
bool patch_fastbootcheck(char* buffer, int32_t size, int32_t global_var_offset) {
    for (int32_t i = 0; i <= size - 4; i += 4) {
        DecodedInst d = decode_at(buffer, i);
        int64_t off0 = calc_adrl_file_offset(buffer, i+4, 0);
        if (!str_at(buffer, size, off0, "fastboot_unlock_verify error and reboot.")) continue;
        if (off0 < 0) continue;
        if (d.type == INST_CBZ_W || d.type == INST_CBZ_X){//change to B
            write_instr(buffer, i, change_to_b(d.raw));
            printf("Patched fastboot warning jump at 0x%X to always jump, fastboot always enabled\n", i);
            return true;
        }
    }
    return false;
}

bool patch_fastboot(char* buffer, int32_t size, int32_t global_var_offset) {
    if (!patch_fastbootcheck(buffer, size, global_var_offset)) {
        printf("OPlus Warning: patch_fastbootcheck failed\n");
        return false;
    }
    return true;
}