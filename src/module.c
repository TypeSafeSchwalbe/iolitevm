
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "module.h"
#include "cli.h"


void read_file(char* filename, char** data, size_t* size) {
    FILE* f = fopen(filename, "rb");
    if(f == NULL) {
        #define OPEN_ERROR_FMT "Unable to open the file '%s'.", filename
        char error_reason[snprintf(NULL, 0, OPEN_ERROR_FMT)]; 
        sprintf(error_reason, OPEN_ERROR_FMT);
        error(error_reason);
    }
    int64_t length;
    if(fseek(f, 0, SEEK_END) || (length = ftell(f)) == -1) {
        #define GET_SIZE_ERROR_FMT "Unable to get the size of the file '%s'.", filename
        char error_reason[snprintf(NULL, 0, GET_SIZE_ERROR_FMT)]; 
        sprintf(error_reason, GET_SIZE_ERROR_FMT);
        error(error_reason);
    }
    *size = length;
    rewind(f);
    *data = malloc(*size);
    if(*data == NULL || fread(*data, *size, 1, f) != 1) {
        free(*data);
        #define READ_ERROR_FMT "Unable to read the contents of the file '%s'.", filename
        char error_reason[snprintf(NULL, 0, READ_ERROR_FMT)]; 
        sprintf(error_reason, READ_ERROR_FMT);
        error(error_reason);
    }
    fclose(f);
}


#define GRAB(p, T) *((T*) (*p)); *p += sizeof(T)
#define StrIdx uint32_t
#define VarIdx uint16_t
#define FcondIdx uint16_t
#define InstrIdx uint64_t
#define MethodIdx uint16_t
#define TraitIdx uint16_t

MString* read_strings(char** p) {
    StrIdx string_count = GRAB(p, StrIdx);
    MString* strings = malloc(sizeof(MString) * string_count);
    for(StrIdx string_index = 0; string_index < string_count; string_index += 1) {
        MString string;
        string.length = GRAB(p, uint16_t);
        string.data = malloc(string.length);
        memcpy(string.data, *p, string.length);
        *p += string.length;
        strings[string_index] = string;
    }
    return strings;
}

Vector read_instructions(char** p, char* end, size_t instr_count, MString* strings) {
    Vector instructions = create_vector(sizeof(Instruction));
    while((*p < end || end == NULL) && (instructions.size < instr_count || instr_count == 0)) {
        uint8_t opcode = GRAB(p, uint8_t);
        Instruction i;
        switch(opcode) {
            case 0x00: {
                i.type = FUNCTION;
                Instruction_Function d;
                StrIdx name = GRAB(p, StrIdx);
                d.name = strings[name];
                d.argc = GRAB(p, VarIdx);
                d.varc = GRAB(p, VarIdx);
                d.condition_count = GRAB(p, FcondIdx);
                d.conditions = malloc(sizeof(Instruction*) * d.condition_count);
                d.condition_lengths = malloc(sizeof(InstrC*) * d.condition_count);
                for(FcondIdx condition_index = 0; condition_index < d.condition_count; condition_index += 1) {
                    InstrIdx condition_length = GRAB(p, InstrIdx);
                    d.condition_lengths[condition_index] = condition_length;
                    Vector condition_body = read_instructions(p, NULL, condition_length, strings);
                    d.conditions[condition_index] = (Instruction*) condition_body.data;
                }
                d.body_length = GRAB(p, InstrIdx);
                Vector body = read_instructions(p, NULL, d.body_length, strings);
                d.body = (Instruction*) body.data;
                i.data.function_data = d;
            } break;
            case 0x01: {
                i.type = CALL;
                Instruction_Call d;
                StrIdx name = GRAB(p, StrIdx);
                d.name = strings[name];
                VarIdx argc = GRAB(p, VarIdx);
                d.argv = malloc(sizeof(VarIdx) * argc);
                for(VarIdx arg = 0; arg < argc; arg += 1) {
                    d.argv[arg] = GRAB(p, VarIdx);
                }
                d.returned = GRAB(p, VarIdx);
                i.data.call_data = d;
            } break;
            case 0x02: {
                i.type = ASYNC_CALL;
                Instruction_AsyncCall d;
                StrIdx name = GRAB(p, StrIdx);
                d.name = strings[name];
                VarIdx argc = GRAB(p, VarIdx);
                d.argv = malloc(sizeof(VarIdx) * argc);
                for(VarIdx arg = 0; arg < argc; arg += 1) {
                    d.argv[arg] = GRAB(p, VarIdx);
                }
                d.returned = GRAB(p, VarIdx);
                i.data.async_call_data = d;
            } break;
            case 0x03: {
                i.type = EXTERNAL_CALL;
                Instruction_ExternalCall d;
                StrIdx name = GRAB(p, StrIdx);
                d.name = strings[name];
                d.argc = GRAB(p, VarIdx);
                d.argv = malloc(sizeof(VarIdx) * d.argc);
                for(VarIdx arg = 0; arg < d.argc; arg += 1) {
                    d.argv[arg] = GRAB(p, VarIdx);
                }
                d.returned = GRAB(p, VarIdx);
                i.data.external_call_data = d;
            } break;
            case 0x04: {
                i.type = CLOSURE_CALL;
                Instruction_ClosureCall d;
                d.called = GRAB(p, VarIdx);
                d.argc = GRAB(p, VarIdx);
                d.argv = malloc(sizeof(VarIdx) * d.argc);
                for(VarIdx arg = 0; arg < d.argc; arg += 1) {
                    d.argv[arg] = GRAB(p, VarIdx);
                }
                d.returned = GRAB(p, VarIdx);
                i.data.closure_call_data = d;
            } break;
            case 0x05: {
                i.type = RETURN;
                Instruction_Return d;
                d.value = GRAB(p, VarIdx);
                i.data.return_data = d;
            } break;
            case 0x06: {
                i.type = RETURN_NOTHING;
            } break;
            case 0x07: {
                i.type = ASSERT;
                Instruction_Assert d;
                d.value = GRAB(p, VarIdx);
                i.data.assert_data = d;
            } break;
            case 0x08: {
                i.type = TRAIT;
                Instruction_Trait d;
                StrIdx name = GRAB(p, StrIdx);
                d.name = strings[name];
                d.method_count = GRAB(p, MethodIdx);
                d.method_names = malloc(sizeof(MString) * d.method_count);
                for(MethodIdx method_index = 0; method_index < d.method_count; method_index += 1) {
                    StrIdx method_name = GRAB(p, StrIdx);
                    d.method_names[method_index] = strings[method_name];
                }
                i.data.trait_data = d;
            } break;
            case 0x09: {
                i.type = IMPLEMENTS;
                Instruction_Implements d;
                StrIdx name = GRAB(p, StrIdx);
                d.name = strings[name];
                d.trait_count = GRAB(p, TraitIdx);
                d.trait_names = malloc(sizeof(MString) * d.trait_count);
                d.trait_impl_function_names = malloc(sizeof(MString*) * d.trait_count);
                for(TraitIdx trait_index = 0; trait_index < d.trait_count; trait_index += 1) {
                    StrIdx trait_name = GRAB(p, StrIdx);
                    d.trait_names[trait_index] = strings[trait_name];
                    MethodIdx trait_method_count = GRAB(p, MethodIdx);
                    d.trait_impl_function_names[trait_index] = malloc(sizeof(MString) * trait_method_count);
                    for(MethodIdx method_index = 0; method_index < trait_method_count; method_index += 1) {
                        StrIdx method_name = GRAB(p, StrIdx);
                        d.trait_impl_function_names[trait_index][method_index] = strings[method_name];
                    }
                }
                i.data.implements_data = d;
            } break;
            case 0x0A: {
                i.type = ADD_IMPLEMENTS;
                Instruction_AddImplements d;
                d.value = GRAB(p, VarIdx);
                StrIdx impl_name = GRAB(p, StrIdx);
                d.impl_name = strings[impl_name];
                i.data.add_implements_data = d;
            } break;
            case 0x0B: {
                i.type = METHOD_CALL;
                Instruction_MethodCall d;
                d.value = GRAB(p, VarIdx);
                StrIdx trait_name = GRAB(p, StrIdx);
                d.trait_name = strings[trait_name];
                StrIdx method_name = GRAB(p, StrIdx);
                d.method_name = strings[method_name];
                d.argc = GRAB(p, VarIdx);
                d.argv = malloc(sizeof(VarIdx) * d.argc);
                for(VarIdx arg = 0; arg < d.argc; arg += 1) {
                    d.argv[arg] = GRAB(p, VarIdx);
                }
                d.returned = GRAB(p, VarIdx);
                i.data.method_call_data = d;
            } break;

            case 0x0C: {
                i.type = IF;
                Instruction_If d;
                d.condition = GRAB(p, VarIdx);
                d.if_body_length = GRAB(p, InstrIdx);
                Vector if_body = read_instructions(p, NULL, d.if_body_length, strings);
                d.if_body = (Instruction*) if_body.data;
                d.else_body_length = GRAB(p, InstrIdx);
                Vector else_body = read_instructions(p, NULL, d.else_body_length, strings);
                d.else_body = (Instruction*) else_body.data;
                i.data.if_data = d;
            } break;
            case 0x0D: {
                i.type = LOOP;
                Instruction_Loop d;
                d.body_length = GRAB(p, InstrIdx);
                Vector body = read_instructions(p, NULL, d.body_length, strings);
                d.body = (Instruction*) body.data;
                i.data.loop_data = d;
            } break;
            case 0x0E: {
                i.type = BREAK;
            } break;
            case 0x0F: {
                i.type = CONTINUE;
            } break;

            case 0x10: {
                i.type = COPY;
                Instruction_Copy d;
                d.src = GRAB(p, VarIdx);
                d.dest = GRAB(p, VarIdx);
                i.data.copy_data = d;
            } break;

            case 0x11: {
                i.type = PUT_NAT;
                Instruction_PutNat d;
                d.value = GRAB(p, uint64_t);
                d.dest = GRAB(p, VarIdx);
                i.data.put_nat_data = d;
            } break;
            case 0x12: {
                i.type = PUT_INT;
                Instruction_PutInt d;
                d.value = GRAB(p, int64_t);
                d.dest = GRAB(p, VarIdx);
                i.data.put_int_data = d;
            } break;
            case 0x13: {
                i.type = PUT_FLT;
                Instruction_PutFlt d;
                d.value = GRAB(p, double);
                d.dest = GRAB(p, VarIdx);
                i.data.put_flt_data = d;
            } break;
            case 0x14: {
                i.type = PUT_CLOSURE;
                Instruction_PutClosure d;
                d.args_offset = GRAB(p, VarIdx);
                d.body_length = GRAB(p, InstrIdx);
                Vector body = read_instructions(p, NULL, d.body_length, strings);
                d.body = (Instruction*) body.data;
                d.dest = GRAB(p, VarIdx);
                i.data.put_closure_data = d;
            } break;

            case 0x15: {
                i.type = EQUALS;
                Instruction_Equals d;
                d.a = GRAB(p, VarIdx);
                d.b = GRAB(p, VarIdx);
                d.dest = GRAB(p, VarIdx);
                i.data.equals_data = d;
            } break;
            case 0x16: {
                i.type = NOT_EQUALS;
                Instruction_NotEquals d;
                d.a = GRAB(p, VarIdx);
                d.b = GRAB(p, VarIdx);
                d.dest = GRAB(p, VarIdx);
                i.data.not_equals_data = d;
            } break;
            case 0x17: {
                i.type = LESS_THAN;
                Instruction_LessThan d;
                d.a = GRAB(p, VarIdx);
                d.b = GRAB(p, VarIdx);
                d.dest = GRAB(p, VarIdx);
                i.data.less_than_data = d;
            } break;
            case 0x18: {
                i.type = GREATER_THAN;
                Instruction_GreaterThan d;
                d.a = GRAB(p, VarIdx);
                d.b = GRAB(p, VarIdx);
                d.dest = GRAB(p, VarIdx);
                i.data.greater_than_data = d;
            } break;
            case 0x19: {
                i.type = LESS_THAN_EQUALS;
                Instruction_LessThanEquals d;
                d.a = GRAB(p, VarIdx);
                d.b = GRAB(p, VarIdx);
                d.dest = GRAB(p, VarIdx);
                i.data.less_than_equals_data = d;
            } break;
            case 0x1A: {
                i.type = GREATER_THAN_EQUALS;
                Instruction_GreaterThanEquals d;
                d.a = GRAB(p, VarIdx);
                d.b = GRAB(p, VarIdx);
                d.dest = GRAB(p, VarIdx);
                i.data.greater_than_equals_data = d;
            } break;
            case 0x1B: {
                i.type = NOT;
                Instruction_Not d;
                d.x = GRAB(p, VarIdx);
                d.dest = GRAB(p, VarIdx);
                i.data.not_data = d;
            } break;

            case 0x1C: {
                i.type = ADD;
                Instruction_Add d;
                d.a = GRAB(p, VarIdx);
                d.b = GRAB(p, VarIdx);
                d.dest = GRAB(p, VarIdx);
                i.data.add_data = d;
            } break;
            case 0x1D: {
                i.type = SUBTRACT;
                Instruction_Subtract d;
                d.a = GRAB(p, VarIdx);
                d.b = GRAB(p, VarIdx);
                d.dest = GRAB(p, VarIdx);
                i.data.subtract_data = d;
            } break;
            case 0x1E: {
                i.type = MULTIPLY;
                Instruction_Multiply d;
                d.a = GRAB(p, VarIdx);
                d.b = GRAB(p, VarIdx);
                d.dest = GRAB(p, VarIdx);
                i.data.multiply_data = d;
            } break;
            case 0x1F: {
                i.type = DIVIDE;
                Instruction_Divide d;
                d.a = GRAB(p, VarIdx);
                d.b = GRAB(p, VarIdx);
                d.dest = GRAB(p, VarIdx);
                i.data.divide_data = d;
            } break;
            case 0x20: {
                i.type = MODULO;
                Instruction_Modulo d;
                d.a = GRAB(p, VarIdx);
                d.b = GRAB(p, VarIdx);
                d.dest = GRAB(p, VarIdx);
                i.data.modulo_data = d;
            } break;
            case 0x21: {
                i.type = NEGATE;
                Instruction_Negate d;
                d.x = GRAB(p, VarIdx);
                d.dest = GRAB(p, VarIdx);
                i.data.negate_data = d;
            } break;

            case 0x22: {
                i.type = CONVERT_TO_NAT;
                Instruction_ConvertToNat d;
                d.x = GRAB(p, VarIdx);
                d.dest = GRAB(p, VarIdx);
                i.data.convert_to_nat_data = d;
            } break;
            case 0x23: {
                i.type = CONVERT_TO_INT;
                Instruction_ConvertToInt d;
                d.x = GRAB(p, VarIdx);
                d.dest = GRAB(p, VarIdx);
                i.data.convert_to_int_data = d;
            } break;
            case 0x24: {
                i.type = CONVERT_TO_FLT;
                Instruction_ConvertToFlt d;
                d.x = GRAB(p, VarIdx);
                d.dest = GRAB(p, VarIdx);
                i.data.convert_to_flt_data = d;
            } break;

            case 0x25: {
                i.type = MALLOC_DYNAMIC;
                Instruction_MallocDynamic d;
                d.size = GRAB(p, VarIdx);
                d.dest = GRAB(p, VarIdx);
                i.data.malloc_dynamic_data = d;
            } break;
            case 0x26: {
                i.type = MALLOC_FIXED;
                Instruction_MallocFixed d;
                d.size = GRAB(p, VarIdx);
                d.dest = GRAB(p, VarIdx);
                i.data.malloc_fixed_data = d;
            } break;
            case 0x27: {
                i.type = REF_GET_DYNAMIC;
                Instruction_RefGetDynamic d;
                d.ref = GRAB(p, VarIdx);
                d.index = GRAB(p, VarIdx);
                d.dest = GRAB(p, VarIdx);
                i.data.ref_get_dynamic_data = d;
            } break;
            case 0x28: {
                i.type = REF_GET_FIXED;
                Instruction_RefGetFixed d;
                d.ref = GRAB(p, VarIdx);
                d.index = GRAB(p, VarIdx);
                d.dest = GRAB(p, VarIdx);
                i.data.ref_get_fixed_data = d;
            } break;
            case 0x29: {
                i.type = REF_SET_DYNAMIC;
                Instruction_RefSetDynamic d;
                d.ref = GRAB(p, VarIdx);
                d.index = GRAB(p, VarIdx);
                d.value = GRAB(p, VarIdx);
                i.data.ref_set_dynamic_data = d;
            } break;
            case 0x2A: {
                i.type = REF_SET_FIXED;
                Instruction_RefSetFixed d;
                d.ref = GRAB(p, VarIdx);
                d.index = GRAB(p, VarIdx);
                d.value = GRAB(p, VarIdx);
                i.data.ref_set_fixed_data = d;
            } break;
        }
        vector_push(&instructions, &i);
    }
    return instructions;
}

Module create_module(char* filename) {
    char* file_data;
    size_t file_size;
    read_file(filename, &file_data, &file_size);
    char* p = file_data;
    MString* strings = read_strings(&p);
    char* file_end = file_data + file_size;
    Vector instructions = read_instructions(&p, file_end, 0, strings);
    return (Module) {
        .body = (Instruction*) instructions.data,
        .body_length = instructions.size
    };
}