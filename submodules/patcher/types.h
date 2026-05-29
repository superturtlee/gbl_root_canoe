#ifndef TOOLS_TYPES_H
#define TOOLS_TYPES_H
#ifdef UEFI
#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/BaseLib.h>
#ifdef DISABLE_PRINT
#define Print_patcher(...) do {} while(0)
#else
#define Print_patcher AsciiPrint
#endif
#define memcpy_patcher CopyMem
#define memcmp_patcher CompareMem
#define malloc AllocatePool
#define free FreePool
#define strlen AsciiStrLen
#define memset SetMem
#else
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#define Print_patcher printf
#define memcpy_patcher memcpy
#define memcmp_patcher memcmp
typedef int32_t  INT32;
typedef uint32_t UINT32;
typedef int8_t   INT8;
typedef uint8_t  UINT8;
typedef int16_t  INT16;
typedef uint16_t UINT16;
typedef uint64_t UINT64;
typedef int64_t  INT64;
typedef char     CHAR8;
#define FALSE (1==0)
#define TRUE  (1==1)
typedef uint8_t BOOLEAN;

INT32 read_file(const CHAR8* filename, CHAR8** data, INT32* size) {
    FILE* file = fopen(filename, "rb");
    if (!file) return -1;
    fseek(file, 0, SEEK_END);
    *size = ftell(file);
    fseek(file, 0, SEEK_SET);
    *data = (CHAR8*)malloc(*size);
    if (!*data) { fclose(file); return -1; }
    if ((INT32)fread(*data, 1, *size, file) != *size) {
        free(*data); fclose(file); return -1;
    }
    fclose(file);
    return 0;
}
#endif
#endif