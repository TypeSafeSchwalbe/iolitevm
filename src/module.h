
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

typedef struct {
    MString name; uint16_t argc; VarIdx varc;
    uint16_t condition_count; struct Instruction** conditions; InstrC* condition_lengths;
    InstrC body_instruction_index; struct Instruction* body; InstrC body_length;
} Instruction_Function;
typedef struct { MString name; VarIdx* argv; VarIdx returned; } Instruction_Call;
typedef struct { MString name; VarIdx* argv; VarIdx returned; } Instruction_AsyncCall;
typedef struct { MString name; VarIdx argc; VarIdx* argv; VarIdx returned; } Instruction_ExternalCall;
typedef struct { VarIdx called; VarIdx argc; VarIdx* argv; VarIdx returned; } Instruction_ClosureCall;
typedef struct { VarIdx value; } Instruction_Return;
typedef struct { VarIdx value; } Instruction_Assert;
typedef struct { MString name; uint16_t method_count; MString* method_names; MString* method_function_names; } Instruction_Trait;
typedef struct { MString name; uint16_t trait_count; MString* trait_names; } Instruction_TraitCollection;
typedef struct { VarIdx value; MString collection_name; } Instruction_AddTraits;
typedef struct { VarIdx value; MString method_name; VarIdx* argv; VarIdx returned; } Instruction_MethodCall;

typedef struct { VarIdx condition; struct Instruction* if_body; InstrC if_body_length; struct Instruction* else_body; InstrC else_body_length; } Instruction_If;
typedef struct { struct Instruction* body; InstrC body_length; } Instruction_Loop;

typedef struct { uint8_t src; uint8_t dest; } Instruction_Copy;

typedef struct { uint64_t value; VarIdx dest; } Instruction_PutNat;
typedef struct { int64_t value; VarIdx dest; } Instruction_PutInt;
typedef struct { double value; VarIdx dest; } Instruction_PutFlt;
typedef struct { InstrC instruction_index; VarIdx args_offset; struct Instruction* body; InstrC body_length; VarIdx dest; } Instruction_PutClosure;

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

typedef struct { VarIdx x; VarIdx dest; } Instruction_ConvertToNat;
typedef struct { VarIdx x; VarIdx dest; } Instruction_ConvertToInt;
typedef struct { VarIdx x; VarIdx dest; } Instruction_ConvertToFlt;

typedef struct { VarIdx size; VarIdx dest; } Instruction_MallocDynamic;
typedef struct { uint64_t size; VarIdx dest; } Instruction_MallocFixed;
typedef struct { VarIdx ref; VarIdx index; VarIdx dest; } Instruction_RefGetDynamic;
typedef struct { VarIdx ref; uint64_t index; VarIdx dest; } Instruction_RefGetFixed;
typedef struct { VarIdx ref; VarIdx index; VarIdx value; } Instruction_RefSetDynamic;
typedef struct { VarIdx ref; uint64_t index; VarIdx value; } Instruction_RefSetFixed;

typedef struct { Instruction_Function* function; VarIdx* argv; VarIdx returned; } Instruction_ResolvedCall;
typedef struct { Instruction_Function* function; VarIdx* argv; VarIdx returned; } Instruction_ResolvedAsyncCall;
typedef struct { void* function; VarIdx argc; VarIdx* argv; VarIdx returned; } Instruction_ResolvedExternalCall;
typedef struct { MString name; uint16_t method_count; MString* method_names; Instruction_Function** methods; } Instruction_ResolvedTrait;
typedef struct { MString name; uint16_t trait_count; Instruction_ResolvedTrait** traits; } Instruction_ResolvedTraitCollection;
typedef struct { VarIdx value; Instruction_ResolvedTraitCollection* collection; } Instruction_ResolvedAddTraits;

typedef struct { InstrC dest; } Instruction_Jump;
typedef struct { VarIdx condition; InstrC if_dest; InstrC else_dest; } Instruction_ConditionalJump;

typedef enum {
    // part of binaries
    FUNCTION, CALL, ASYNC_CALL, EXTERNAL_CALL, CLOSURE_CALL, RETURN, RETURN_NOTHING, ASSERT, TRAIT, TRAIT_COLLECTION, ADD_TRAITS, METHOD_CALL,
    RECORD, RECORD_INIT, RECORD_GET, RECORD_SET,
    IF, LOOP, BREAK, CONTINUE,
    COPY,
    PUT_NAT, PUT_INT, PUT_FLT, PUT_CLOSURE,
    EQUALS, NOT_EQUALS, LESS_THAN, GREATER_THAN, LESS_THAN_EQUALS, GREATER_THAN_EQUALS, NOT,
    ADD, SUBTRACT, MULTIPLY, DIVIDE, MODULO, NEGATE,
    CONVERT_TO_NAT, CONVERT_TO_INT, CONVERT_TO_FLT,
    MALLOC_DYNAMIC, MALLOC_FIXED, REF_GET_DYNAMIC, REF_GET_FIXED, REF_SET_DYNAMIC, REF_SET_FIXED,
    // not part of binaries
    RESOLVED_CALL, RESOLVED_ASYNC_CALL, RESOLVED_EXTERNAL_CALL, RESOLVED_TRAIT, RESOLVED_TRAIT_COLLECTION, RESOLVED_ADD_TRAITS,
    JUMP, CONDITIONAL_JUMP
} InstructionType;

typedef union {
    Instruction_Function function_data;
    Instruction_Call call_data;
    Instruction_AsyncCall async_call_data;
    Instruction_ExternalCall external_call_data;
    Instruction_ClosureCall closure_call_data;
    Instruction_Return return_data;
    Instruction_Assert assert_data;
    Instruction_Trait trait_data;
    Instruction_TraitCollection trait_collection_data;
    Instruction_AddTraits add_traits_data;
    Instruction_MethodCall method_call_data;

    Instruction_If if_data;
    Instruction_Loop loop_data;

    Instruction_Copy copy_data;

    Instruction_PutNat put_nat_data;
    Instruction_PutInt put_int_data;
    Instruction_PutFlt put_flt_data;
    Instruction_PutClosure put_closure_data;

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

    Instruction_ConvertToNat convert_to_nat_data;
    Instruction_ConvertToInt convert_to_int_data;
    Instruction_ConvertToFlt convert_to_flt_data;

    Instruction_MallocDynamic malloc_dynamic_data;
    Instruction_MallocFixed malloc_fixed_data;
    Instruction_RefGetDynamic ref_get_dynamic_data;
    Instruction_RefGetFixed ref_get_fixed_data;
    Instruction_RefSetDynamic ref_set_dynamic_data;
    Instruction_RefSetFixed ref_set_fixed_data;

    Instruction_ResolvedCall resolved_call_data;
    Instruction_ResolvedAsyncCall resolved_async_call_data;
    Instruction_ResolvedExternalCall resolved_external_call_data;
    Instruction_ResolvedTrait resolved_trait_data;
    Instruction_ResolvedTraitCollection resolved_trait_collection_data;
    Instruction_ResolvedAddTraits resolved_add_traits_data;

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
