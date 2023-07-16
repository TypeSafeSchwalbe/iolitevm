
#ifndef IOLITEVM_API_HEADER
#define IOLITEVM_API_HEADER

#include <stdint.h>
#include <stdatomic.h>


typedef struct {
    struct IoliteAllocation* frame;
    uint64_t instruction_index;
    uint16_t args_offset;
} IoliteClosure;

typedef struct {
    enum Type {
        NATURAL, INTEGER, FLOAT,
        REFERENCE,
        CLOSURE,
        UNIT
    } type;
    union {
        uint64_t natural; int64_t integer; double flt;
        struct IoliteAllocation* ref;
        IoliteClosure closure; 
    } value;
    void* methods;
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