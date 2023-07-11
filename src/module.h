
#pragma once

#include <stdint.h>
#include "vector.h"


typedef struct {
    uint16_t length;
    char* data;
} MString;


typedef uint16_t VarIdx;
typedef uint64_t InstrC;

// Bytecode also needs to include type info and public / private info

typedef struct { MString name; uint16_t argc; VarIdx varc; InstrC instruction_index; struct Instruction* body; InstrC body_length; } Instruction_Function;
typedef struct { MString name; VarIdx* argv; VarIdx returned; } Instruction_Call;
typedef struct { MString name; VarIdx argc; VarIdx* argv; VarIdx returned; } Instruction_ExternalCall;
typedef struct { VarIdx value; } Instruction_Return;

typedef struct { VarIdx condition; struct Instruction* if_body; InstrC if_body_length; struct Instruction* else_body; InstrC else_body_length; } Instruction_If;
typedef struct { struct Instruction* body; InstrC body_length; } Instruction_Loop;

typedef struct { uint8_t src; uint8_t dest; } Instruction_Copy;

typedef struct { uint8_t value; VarIdx dest; } Instruction_PutU8;
typedef struct { uint16_t value; VarIdx dest; } Instruction_PutU16;
typedef struct { uint32_t value; VarIdx dest; } Instruction_PutU32;
typedef struct { uint64_t value; VarIdx dest; } Instruction_PutU64;
typedef struct { int8_t value; VarIdx dest; } Instruction_PutS8;
typedef struct { int16_t value; VarIdx dest; } Instruction_PutS16;
typedef struct { int32_t value; VarIdx dest; } Instruction_PutS32;
typedef struct { int64_t value; VarIdx dest; } Instruction_PutS64;
typedef struct { float value; VarIdx dest; } Instruction_PutF32;
typedef struct { double value; VarIdx dest; } Instruction_PutF64;

typedef struct { VarIdx a; VarIdx b; VarIdx dest; } Instruction_Equals;
typedef struct { VarIdx a; VarIdx b; VarIdx dest; } Instruction_NotEquals;
typedef struct { VarIdx a; VarIdx b; VarIdx dest; } Instruction_LessThan;
typedef struct { VarIdx a; VarIdx b; VarIdx dest; } Instruction_GreaterThan;
typedef struct { VarIdx a; VarIdx b; VarIdx dest; } Instruction_LessThanEquals;
typedef struct { VarIdx a; VarIdx b; VarIdx dest; } Instruction_GreaterThanEquals;
typedef struct { VarIdx x; VarIdx dest; } Instruction_Not;

typedef struct { VarIdx a; VarIdx b; VarIdx dest; } Instruction_Add;
typedef struct { VarIdx a; VarIdx b; VarIdx dest; } Instruction_Subtract;
typedef struct { VarIdx a; VarIdx b; VarIdx dest; } Instruction_Multiply;
typedef struct { VarIdx a; VarIdx b; VarIdx dest; } Instruction_Divide;
typedef struct { VarIdx a; VarIdx b; VarIdx dest; } Instruction_Modulo;
typedef struct { VarIdx x; VarIdx dest; } Instruction_Negate;

typedef struct { VarIdx x; VarIdx dest; } Instruction_ConvertU8;
typedef struct { VarIdx x; VarIdx dest; } Instruction_ConvertU16;
typedef struct { VarIdx x; VarIdx dest; } Instruction_ConvertU32;
typedef struct { VarIdx x; VarIdx dest; } Instruction_ConvertU64;
typedef struct { VarIdx x; VarIdx dest; } Instruction_ConvertS8;
typedef struct { VarIdx x; VarIdx dest; } Instruction_ConvertS16;
typedef struct { VarIdx x; VarIdx dest; } Instruction_ConvertS32;
typedef struct { VarIdx x; VarIdx dest; } Instruction_ConvertS64;
typedef struct { VarIdx x; VarIdx dest; } Instruction_ConvertF32;
typedef struct { VarIdx x; VarIdx dest; } Instruction_ConvertF64;

typedef struct { Instruction_Function* function; VarIdx* argv; VarIdx returned; } Instruction_ResolvedCall;
typedef struct { void* function; VarIdx argc; VarIdx* argv; VarIdx returned; } Instruction_ResolvedExternalCall;

typedef struct { VarIdx size; VarIdx dest; } Instruction_MallocDynamic;
typedef struct { uint64_t size; VarIdx dest; } Instruction_MallocFixed;
typedef struct { VarIdx ref; VarIdx index; VarIdx dest; } Instruction_RefGetDynamic;
typedef struct { VarIdx ref; uint64_t index; VarIdx dest; } Instruction_RefGetFixed;
typedef struct { VarIdx ref; VarIdx index; VarIdx value; } Instruction_RefSetDynamic;
typedef struct { VarIdx ref; uint64_t index; VarIdx value; } Instruction_RefSetFixed;

typedef struct { InstrC dest; } Instruction_Jump;
typedef struct { VarIdx condition; InstrC if_dest; InstrC else_dest; } Instruction_ConditionalJump;

typedef enum {
    // part of binaries
    FUNCTION, CALL, EXTERNAL_CALL, RETURN, RETURN_NOTHING,
    IF, LOOP, BREAK, CONTINUE,
    COPY,
    PUT_U8, PUT_U16, PUT_U32, PUT_U64, PUT_S8, PUT_S16, PUT_S32, PUT_S64, PUT_F32, PUT_F64,
    EQUALS, NOT_EQUALS, LESS_THAN, GREATER_THAN, LESS_THAN_EQUALS, GREATER_THAN_EQUALS, NOT,
    ADD, SUBTRACT, MULTIPLY, DIVIDE, MODULO, NEGATE,
    CONVERT_U8, CONVERT_U16, CONVERT_U32, CONVERT_U64, CONVERT_S8, CONVERT_S16, CONVERT_S32, CONVERT_S64, CONVERT_F32, CONVERT_F64,
    // not part of binaries
    RESOLVED_CALL, RESOLVED_EXTERNAL_CALL,
    MALLOC_DYNAMIC, MALLOC_FIXED, REF_GET_DYNAMIC, REF_GET_FIXED, REF_SET_DYNAMIC, REF_SET_FIXED,
    JUMP, CONDITIONAL_JUMP
} InstructionType;

typedef union {
    Instruction_Function function_data;
    Instruction_Call call_data;
    Instruction_ExternalCall external_call_data;
    Instruction_Return return_data;

    Instruction_If if_data;
    Instruction_Loop loop_data;

    Instruction_Copy copy_data;

    Instruction_PutU8 put_u8_data;
    Instruction_PutU16 put_u16_data;
    Instruction_PutU32 put_u32_data;
    Instruction_PutU64 put_u64_data;
    Instruction_PutS8 put_s8_data;
    Instruction_PutS16 put_s16_data;
    Instruction_PutS32 put_s32_data;
    Instruction_PutS64 put_s64_data;
    Instruction_PutF32 put_f32_data;
    Instruction_PutF64 put_f64_data;

    Instruction_Equals equals_data;
    Instruction_NotEquals not_equals_data;
    Instruction_LessThan less_than_data;
    Instruction_GreaterThan greater_than_data;
    Instruction_LessThanEquals less_than_equals_data;
    Instruction_GreaterThanEquals greater_than_equals_data;
    Instruction_Not not_data;

    Instruction_Add add_data;
    Instruction_Subtract subtract_data;
    Instruction_Multiply multiply_data;
    Instruction_Divide divide_data;
    Instruction_Modulo modulo_data;
    Instruction_Negate negate_data;

    Instruction_ConvertU8 convert_u8_data;
    Instruction_ConvertU16 convert_u16_data;
    Instruction_ConvertU32 convert_u32_data;
    Instruction_ConvertU64 convert_u64_data;
    Instruction_ConvertS8 convert_s8_data;
    Instruction_ConvertS16 convert_s16_data;
    Instruction_ConvertS32 convert_s32_data;
    Instruction_ConvertS64 convert_s64_data;
    Instruction_ConvertF32 convert_f32_data;
    Instruction_ConvertF64 convert_f64_data;

    Instruction_ResolvedCall resolved_call_data;
    Instruction_ResolvedExternalCall resolved_external_call_data;

    Instruction_MallocDynamic malloc_dynamic_data;
    Instruction_MallocFixed malloc_fixed_data;
    Instruction_RefGetDynamic ref_get_dynamic_data;
    Instruction_RefGetFixed ref_get_fixed_data;
    Instruction_RefSetDynamic ref_set_dynamic_data;
    Instruction_RefSetFixed ref_set_fixed_data;

    Instruction_Jump jump_data;
    Instruction_ConditionalJump conditional_jump_data;
} InstructionData;

typedef struct Instruction {
    InstructionType type;
    InstructionData data;
} Instruction;


typedef struct {
    Instruction* body;
    InstrC body_length;
} Module;
