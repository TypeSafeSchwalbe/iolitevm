
#ifndef IOLITEVM_API_HEADER
#define IOLITEVM_API_HEADER

#include <stdint.h>


typedef struct {
    enum Type {
        U8, U16, U32, U64,
        S8, S16, S32, S64,
        F32, F64,
        UNIT
    } type;
    union {
        uint8_t u8; uint16_t u16; uint32_t u32; uint64_t u64;
        int8_t s8; int16_t s16; int32_t s32; int64_t s64;
        float f32; double f64;
    } value;
} IoliteValue;

#define IoliteUnitValue (IoliteValue) { .type = UNIT }

typedef IoliteValue (*IoliteExternalFunction)();


#endif