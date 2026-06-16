#ifndef OPLUS_FASTBOOT_H
#define OPLUS_FASTBOOT_H
#include <stdint.h>
#include <stdbool.h>
bool patch_fastboot(char* buffer, int32_t size, int32_t global_var_offset);
#endif /* OPLUS_FASTBOOT_H */