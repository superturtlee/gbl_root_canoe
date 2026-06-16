#ifndef INST_UTILS_H
#define INST_UTILS_H
#include "arm64_inst_decoder.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
typedef enum { LOC_REG, LOC_STK64, LOC_STK8 } DataLocType;

typedef struct {
    DataLocType type;
    int32_t val;
} DataLoc;

typedef struct {
    DataLoc locs[256];
    int32_t count;
} LocSet;
/* 判断一条指令是否为任意形式的 STRB，并提取字段 */
typedef struct {
    bool valid;
    uint8_t   rt;
    uint8_t   rn;
    uint32_t  imm;
} StrbInfo;
bool locset_has(const LocSet* s, DataLoc l);
bool locset_has_reg  (const LocSet* s, int8_t r);
bool locset_has_stk64(const LocSet* s, uint32_t v);
bool locset_has_stk8 (const LocSet* s, uint32_t v);

void locset_add(LocSet* s, DataLoc l);
void locset_add_reg  (LocSet* s, int8_t r);
void locset_add_stk64(LocSet* s, uint32_t v);
void locset_add_stk8 (LocSet* s, uint32_t v);

void locset_del(LocSet* s, DataLoc l);
void locset_del_reg  (LocSet* s, int8_t r);
void locset_del_stk64(LocSet* s, uint32_t v);
void locset_del_stk8 (LocSet* s, uint32_t v);

bool locset_empty(const LocSet* s);

void locset_print(const LocSet* s);

StrbInfo decode_any_strb(uint32_t raw);
typedef int32_t (*SourceCallback)(char* buffer, int32_t size, int32_t now_offset, int8_t current_reg_target, int32_t start_offset);
#define SUCCESS 0
#define FAILURE 1
#define TOO_MANY_BOUNCES 2
#define NEED_MORE -1
typedef int32_t (*ForwardCallback)(char* buffer, int32_t size, int32_t now_offset, DecodedInst d, int32_t anchor_offset);
//寻找寄存器值来源
int32_t track_forward(char* buffer, int32_t size, int32_t start_offset,
                                      int8_t src_reg, int32_t ancher_offset, ForwardCallback callback);
int32_t find_ldrB_instructio_reverse(char* buffer, int32_t size,
                                   int32_t start_offset, int8_t target_register,int32_t* global_var_offset, SourceCallback callback);
int32_t empty_source_callback(char* buffer, int32_t size, int32_t now_offset, int8_t current_reg_target, int32_t start_offset);
bool str_at(const char* buffer, int32_t size, int64_t file_off, const char* needle);    
int64_t calc_adrl_file_offset(const char* buffer, int32_t adrp_off, uint64_t load_base);                               
#endif /* INST_UTILS_H */