
#ifndef IOLITEVM_API_HEADER
#define IOLITEVM_API_HEADER

#include <stdint.h>
#include <stdatomic.h>


typedef struct {
    enum Type {
        U8, U16, U32, U64,
        S8, S16, S32, S64,
        F32, F64,
        REFERENCE,
        UNIT
    } type;
    union {
        uint8_t u8; uint16_t u16; uint32_t u32; uint64_t u64;
        int8_t s8; int16_t s16; int32_t s32; int64_t s64;
        float f32; double f64;
        struct IoliteAllocation* ref;
    } value;
} IoliteValue;

typedef struct IoliteAllocation {
    IoliteValue* values;
    uint64_t size;
    _Atomic(uint32_t) stack_reference_count; // stop data races (runtime modifies, GC reads) 
    uint8_t gc_flags;
} IoliteAllocation;

#define IoliteUnitValue (IoliteValue) { .type = UNIT, .value = {} }

typedef IoliteValue (*IoliteExternalFunction)();


#endif