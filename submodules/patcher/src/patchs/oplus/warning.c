#include "patchs/oplus/warning.h"
#include "arm64_inst/utils.h"
int32_t find_warning_offset(char* buffer, int32_t size, uint64_t load_base) {
    if (size < 24) return 0;

    for (int32_t i = 0; i <= size - 24; i += 4) {
        int64_t off0 = calc_adrl_file_offset(buffer, i,      load_base);
        int64_t off1 = calc_adrl_file_offset(buffer, i + 8,  load_base);
        if (off0 < 0 || off1 < 0) continue;

        if (!str_at(buffer, size, off0, "Orange State\n")) continue;
        if (!str_at(buffer, size, off1, "Your device has been unlocked and can't be trusted\n"))   continue;
        return i;
    }
    return -1;
}
bool patch_warning(char* buffer, int32_t size, int32_t global_var_offset) {
    int32_t warn_off = find_warning_offset(buffer, size, 0);
    if (warn_off < 0) {
        printf("Warning not found\n");
        return false;
    }
    printf("Warning message ADRL offset: 0x%X\n", warn_off);
    //逆向找到CBZ
    int32_t max_search_range = warn_off - 64 > 0 ? warn_off - 64 : 0;//限制搜索范围，避免误伤其他CBZ
    for(int32_t i = warn_off - 4; i >= max_search_range; i -= 4) {
        DecodedInst d = decode_at(buffer, i);
        if (d.type == INST_PACIASP) {
            printf("Reached function start at 0x%X, stop\n", i);
            return false;
        }
        if (d.type == INST_CBZ_W) {
            printf("Register W%d is used for warning jump at 0x%X\n", d.rt, i);
            //track back
            int32_t offset = -1;
            find_ldrB_instructio_reverse(buffer, size, i, (int8_t)d.rt, &offset, empty_source_callback);
            if (offset < 0 || offset != global_var_offset) continue; // not from lock state var, skip
            printf("Warning jump source var: 0x%X Matched\n", offset);
            write_instr(buffer,i,change_rt(&d, 31)); // change to CBZ WZR
            printf("Patched CBZ at 0x%X to use WZR, warning disabled\n", i);
            break;
        }
    }
    return true;
}