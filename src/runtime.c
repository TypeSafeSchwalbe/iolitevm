
#include <string.h>
#include <math.h>
#include "runtime.h"


Runtime create_runtime() {
    Runtime r;
    r.functions = create_vector(sizeof(Instruction_Function*));
    return r;
}

void discover_symbols(Runtime* r, Module* m) {
    for(InstrC instruction_index = 0; instruction_index < m->body_length; instruction_index += 1) {
        Instruction* instruction = &m->body[instruction_index];
        switch(instruction->type) {
            case FUNCTION: {
                Instruction_Function* function_data = &instruction->data.function_data;
                vector_push(&r->functions, &function_data);
            } break;

            default: {}
        }
    }
}

void resolve_symbols(Runtime* r, DLibLoader* l, Instruction* instructions, InstrC instruction_count) {
    for(InstrC instruction_index = 0; instruction_index < instruction_count; instruction_index += 1) {
        Instruction* instruction = &instructions[instruction_index];
        switch(instruction->type) {
            case CALL: {
                Instruction_Call* data = &instruction->data.call_data;
                // try to find the function in all loaded module functions
                uint8_t found = 0;
                for(size_t function_index = 0; function_index < r->functions.size; function_index += 1) {
                    Instruction_Function** function = vector_get(&r->functions, function_index);
                    if((*function)->name.length == data->name.length
                    && memcmp((*function)->name.data, data->name.data, data->name.length) == 0) {
                        *instruction = (Instruction) {
                            .type = RESOLVED_CALL,
                            .data = (InstructionData) { .resolved_call_data = (Instruction_ResolvedCall) {
                                .function = *function,
                                .argv = data->argv,
                                .returned = data->returned
                            } }
                        };
                        found = 1;
                        break;
                    }
                }
                if(found) { continue; }
                // still not found? -> error
                char* name_null_terminated = malloc(data->name.length + 1);
                memcpy(name_null_terminated, data->name.data, data->name.length);
                name_null_terminated[data->name.length] = '\0';
                printf("No function with the name '%s' could be found in any loaded module.\n", name_null_terminated);
                free(name_null_terminated);
                exit(1);
            } break;
            case EXTERNAL_CALL: {
                Instruction_ExternalCall* data = &instruction->data.external_call_data;
                // try to find the function in all loaded shared libraries
                char* name_null_terminated = malloc(data->name.length + 1);
                memcpy(name_null_terminated, data->name.data, data->name.length);
                name_null_terminated[data->name.length] = '\0';
                void* external_function = dlibs_find(l, name_null_terminated);
                if(external_function != NULL) {
                    *instruction = (Instruction) {
                        .type = RESOLVED_EXTERNAL_CALL,
                        .data = (InstructionData) { .resolved_external_call_data = (Instruction_ResolvedExternalCall) {
                            .function = external_function,
                            .argc = data->argc,
                            .argv = data->argv,
                            .returned = data->returned
                        } }
                    };
                    free(name_null_terminated);
                    break;
                }
                // still not found? -> error
                printf("No function with the name '%s' could be found in any loaded shared library.\n", name_null_terminated);
                free(name_null_terminated);
                exit(1);
            } break;

            case FUNCTION: {
                Instruction_Function* data = &instruction->data.function_data;
                resolve_symbols(r, l, data->body, data->body_length);
            } break;
            case IF: {
                Instruction_If* data = &instruction->data.if_data;
                resolve_symbols(r, l, data->if_body, data->if_body_length);
                resolve_symbols(r, l, data->else_body, data->else_body_length);
            } break;
            case LOOP: {
                Instruction_Loop* data = &instruction->data.loop_data;
                resolve_symbols(r, l, data->body, data->body_length);
            } break;

            default: {}
        }
    }
}


InstrC calculate_flat_length(Instruction* instructions, InstrC instruction_count) {
    InstrC length = 0;
    for(InstrC instruction_index = 0; instruction_index < instruction_count; instruction_index += 1) {
        Instruction* i = &instructions[instruction_index];
        switch(i->type) {
            case FUNCTION: {
                Instruction_Function* data = &i->data.function_data;
                // [FUNCTION]
                // ----------------
                // <jump>
                // <function>
                // [FUNCTION BODY]
                length += 1
                    + 1
                    + calculate_flat_length(data->body, data->body_length);
            } break;
            case IF: {
                Instruction_If* data = &i->data.if_data;
                // [IF]
                // ----------------
                // <conditional jump>
                // [IF BODY]
                // <jump>
                // [ELSE BODY]
                length += 1
                    + calculate_flat_length(data->if_body, data->if_body_length)
                    + 1
                    + calculate_flat_length(data->else_body, data->else_body_length);
            } break;
            case LOOP: {
                Instruction_Loop* data = &i->data.loop_data;
                // [LOOP]
                // ----------------
                // [LOOP BODY]
                // <jump>
                length += calculate_flat_length(data->body, data->body_length)
                    + 1;
            } break;
            default: length += 1;
        }
    }
    return length;
}

void flatten_instructions(
        Instruction* src_instructions,
        InstrC src_instruction_count,
        Instruction* dest_instructions,
        InstrC* absolute_index,
        InstrC parent_loop_absolute_start,
        InstrC parent_loop_absolute_post
) {
    InstrC relative_index = 0;
    while(relative_index < src_instruction_count) {
        Instruction* i = &src_instructions[relative_index];
        switch(i->type) {
            case FUNCTION: {
                Instruction_Function* data = &i->data.function_data;
                dest_instructions[*absolute_index] = (Instruction) { .type = JUMP, .data = { .jump_data = {
                    .dest = 0
                } } };
                InstrC* pre_jump_dest = &dest_instructions[*absolute_index].data.jump_data.dest;
                *absolute_index += 1;
                dest_instructions[*absolute_index] = src_instructions[relative_index];
                *absolute_index += 1;
                data->instruction_index = *absolute_index;
                flatten_instructions(data->body, data->body_length, dest_instructions, absolute_index, parent_loop_absolute_start, parent_loop_absolute_post);
                *pre_jump_dest = *absolute_index;
            } break;
            case IF: {
                Instruction_If* data = &i->data.if_data;
                dest_instructions[*absolute_index] = (Instruction) { .type = CONDITIONAL_JUMP, .data = { .conditional_jump_data = {
                    .condition = data->condition,
                    .if_dest = *absolute_index + 1,
                    .else_dest = 0
                } } };
                InstrC* jmp_else_dest = &dest_instructions[*absolute_index].data.conditional_jump_data.else_dest;
                *absolute_index += 1;
                flatten_instructions(data->if_body, data->if_body_length, dest_instructions, absolute_index, parent_loop_absolute_start, parent_loop_absolute_post);
                dest_instructions[*absolute_index] = (Instruction) { .type = JUMP, .data = { .jump_data = {
                    .dest = 0
                } } };
                InstrC* if_jmp_dest = &dest_instructions[*absolute_index].data.jump_data.dest;
                *absolute_index += 1;
                *jmp_else_dest = *absolute_index;
                flatten_instructions(data->else_body, data->else_body_length, dest_instructions, absolute_index, parent_loop_absolute_start, parent_loop_absolute_post);
                *if_jmp_dest = *absolute_index;
            } break;
            case LOOP: {
                Instruction_Loop* data = &i->data.loop_data;
                InstrC loop_start_index = *absolute_index;
                flatten_instructions(data->body, data->body_length, dest_instructions, absolute_index, *absolute_index, *absolute_index + calculate_flat_length(data->body, data->body_length) + 1);
                dest_instructions[*absolute_index] = (Instruction) { .type = JUMP, .data = { .jump_data = {
                    .dest = loop_start_index
                } } };
                *absolute_index += 1;
            } break;
            case CONTINUE: {
                dest_instructions[*absolute_index] = (Instruction) { .type = JUMP, .data = { .jump_data = {
                    .dest = parent_loop_absolute_start
                } } };
                *absolute_index += 1;
            } break;
            case BREAK: {
                dest_instructions[*absolute_index] = (Instruction) { .type = JUMP, .data = { .jump_data = {
                    .dest = parent_loop_absolute_post
                } } };
                *absolute_index += 1;
            } break;
            default: {
                dest_instructions[*absolute_index] = src_instructions[relative_index];
                *absolute_index += 1;
            } break;
        }
        relative_index += 1;
    }
}

void flatten_combine(Module* modules, size_t module_count, Instruction** instructions, InstrC* instruction_count) {
    *instruction_count = 0;
    for(size_t module_index = 0; module_index < module_count; module_index += 1) {
        *instruction_count += calculate_flat_length(modules[module_index].body, modules[module_index].body_length);
    }
    *instructions = malloc(sizeof(Instruction) * *instruction_count);
    InstrC instruction_index = 0;
    for(size_t module_index = 0; module_index < module_count; module_index += 1) {
        flatten_instructions(modules[module_index].body, modules[module_index].body_length, *instructions, &instruction_index, 0, *instruction_count);
    }
}


#define BI_OPERATION(OP) switch(a->type) {\
    case U8: *result = (IoliteValue) { .type = U8, .value = { .u8 = a->value.u8 OP b->value.u8 } }; break;\
    case U16: *result = (IoliteValue) { .type = U16, .value = { .u16 = a->value.u16 OP b->value.u16 } }; break;\
    case U32: *result = (IoliteValue) { .type = U32, .value = { .u32 = a->value.u32 OP b->value.u32 } }; break;\
    case U64: *result = (IoliteValue) { .type = U64, .value = { .u64 = a->value.u64 OP b->value.u64 } }; break;\
    case S8: *result = (IoliteValue) { .type = S8, .value = { .s8 = a->value.s8 OP b->value.s8 } }; break;\
    case S16: *result = (IoliteValue) { .type = S16, .value = { .s16 = a->value.s16 OP b->value.s16 } }; break;\
    case S32: *result = (IoliteValue) { .type = S32, .value = { .s32 = a->value.s32 OP b->value.s32 } }; break;\
    case S64: *result = (IoliteValue) { .type = S64, .value = { .s64 = a->value.s64 OP b->value.s64 } }; break;\
    case F32: *result = (IoliteValue) { .type = F32, .value = { .f32 = a->value.f32 OP b->value.f32 } }; break;\
    case F64: *result = (IoliteValue) { .type = F64, .value = { .f64 = a->value.f64 OP b->value.f64 } }; break;\
    default: {\
        printf("Cannot perform a mathematical / comparative operation on a non-number value!\n");\
        exit(1);\
    }\
}

#define CONVERSION_TO(T) switch(x->type) {\
    case U8: value = (T) x->value.u8; break;\
    case U16: value = (T) x->value.u16; break;\
    case U32: value = (T) x->value.u32; break;\
    case U64: value = (T) x->value.u64; break;\
    case S8: value = (T) x->value.s8; break;\
    case S16: value = (T) x->value.s16; break;\
    case S32: value = (T) x->value.s32; break;\
    case S64: value = (T) x->value.s64; break;\
    case F32: value = (T) x->value.f32; break;\
    case F64: value = (T) x->value.f64; break;\
    default: {\
        printf("Cannot convert a non-number value to another number type!\n");\
        exit(1);\
    }\
}

void execute(Runtime* r, GC* gc, Instruction* instructions, InstrC instruction_count) {
    InstrC current_index = 0;
    Vector frames = create_vector(sizeof(Frame));
    Vector return_idx = create_vector(sizeof(InstrC));
    Frame* current_frame;
    VarIdx return_val_dest_var;
    Instruction* i;
    while(current_index < instruction_count) {
        i = &instructions[current_index];
        switch(i->type) {
            case FUNCTION: /* nothing to do, functions have already been loaded */ break; 
            case CALL: /* call should be resolved, we shouldn't ever encounter this instruction */ break;
            case EXTERNAL_CALL: /* call should be resolved, we shouldn't ever encounter this instruction */ break;
            case RETURN: case RETURN_NOTHING: {
                vector_pop(&frames);
                Frame* old_frame = frames.size > 0? vector_get(&frames, frames.size - 1) : NULL;
                if(i->type == RETURN) {
                    Instruction_Return* data = &i->data.return_data;
                    IoliteValue* return_val = &current_frame->values[data->value];
                    IoliteValue* dest = &old_frame->values[return_val_dest_var];
                    if(return_val->type == REFERENCE) { return_val->value.ref->stack_reference_count += 1; }
                    if(dest->type == REFERENCE) { dest->value.ref->stack_reference_count -= 1; }
                }
                for(VarIdx var = 0; var < current_frame->size; var += 1) {
                    IoliteValue* val = &current_frame->values[var];
                    if(val->type == REFERENCE) { val->value.ref->stack_reference_count -= 1; }
                }
                free(current_frame->values);
                current_frame = old_frame;
                InstrC* new_index = vector_get(&return_idx, return_idx.size - 1);
                current_index = *new_index;
                vector_pop(&return_idx);
                continue;
            } break;
            
            case IF: /* nothing to do, ifs were replaced by conditional jumps */ break;
            case LOOP: /* nothing to do, loops were replaced by jumps */ break;
            case CONTINUE: /* nothing to do, continues were replaced by jumps */ break;
            case BREAK: /* nothing to do, breaks were replaced by jumps */ break;

            case COPY: {
                Instruction_Copy* data = &i->data.copy_data;
                if(data->src == data->dest) { break; }
                IoliteValue* src = &current_frame->values[data->src];
                IoliteValue* dest = &current_frame->values[data->dest];
                if(src->type == REFERENCE) { src->value.ref->stack_reference_count += 1; }
                if(dest->type == REFERENCE) { src->value.ref->stack_reference_count -= 1; }
                *dest = *src;
            } break;

            case PUT_U8: {
                Instruction_PutU8* data = &i->data.put_u8_data;
                IoliteValue* dest = &current_frame->values[data->dest];
                if(dest->type == REFERENCE) { dest->value.ref->stack_reference_count -= 1; }
                *dest = (IoliteValue) { .type = U8, .value = { .u8 = data->value } };
            } break;
            case PUT_U16: {
                Instruction_PutU16* data = &i->data.put_u16_data;
                IoliteValue* dest = &current_frame->values[data->dest];
                if(dest->type == REFERENCE) { dest->value.ref->stack_reference_count -= 1; }
                *dest = (IoliteValue) { .type = U16, .value = { .u16 = data->value } };
            } break;
            case PUT_U32: {
                Instruction_PutU32* data = &i->data.put_u32_data;
                IoliteValue* dest = &current_frame->values[data->dest];
                if(dest->type == REFERENCE) { dest->value.ref->stack_reference_count -= 1; }
                *dest = (IoliteValue) { .type = U32, .value = { .u32 = data->value } };
            } break;
            case PUT_U64: {
                Instruction_PutU64* data = &i->data.put_u64_data;
                IoliteValue* dest = &current_frame->values[data->dest];
                if(dest->type == REFERENCE) { dest->value.ref->stack_reference_count -= 1; }
                *dest = (IoliteValue) { .type = U64, .value = { .u64 = data->value } };
            } break;
            case PUT_S8: {
                Instruction_PutS8* data = &i->data.put_s8_data;
                IoliteValue* dest = &current_frame->values[data->dest];
                if(dest->type == REFERENCE) { dest->value.ref->stack_reference_count -= 1; }
                *dest = (IoliteValue) { .type = S8, .value = { .s8 = data->value } };
            } break;
            case PUT_S16: {
                Instruction_PutS16* data = &i->data.put_s16_data;
                IoliteValue* dest = &current_frame->values[data->dest];
                if(dest->type == REFERENCE) { dest->value.ref->stack_reference_count -= 1; }
                *dest = (IoliteValue) { .type = S16, .value = { .s16 = data->value } };
            } break;
            case PUT_S32: {
                Instruction_PutS32* data = &i->data.put_s32_data;
                IoliteValue* dest = &current_frame->values[data->dest];
                if(dest->type == REFERENCE) { dest->value.ref->stack_reference_count -= 1; }
                *dest = (IoliteValue) { .type = S32, .value = { .s32 = data->value } };
            } break;
            case PUT_S64: {
                Instruction_PutS64* data = &i->data.put_s64_data;
                IoliteValue* dest = &current_frame->values[data->dest];
                if(dest->type == REFERENCE) { dest->value.ref->stack_reference_count -= 1; }
                *dest = (IoliteValue) { .type = S64, .value = { .s64 = data->value } };
            } break;
            case PUT_F32: {
                Instruction_PutF32* data = &i->data.put_f32_data;
                IoliteValue* dest = &current_frame->values[data->dest];
                if(dest->type == REFERENCE) { dest->value.ref->stack_reference_count -= 1; }
                *dest = (IoliteValue) { .type = F32, .value = { .f32 = data->value } };
            } break;
            case PUT_F64: {
                Instruction_PutF64* data = &i->data.put_f64_data;
                IoliteValue* dest = &current_frame->values[data->dest];
                if(dest->type == REFERENCE) { dest->value.ref->stack_reference_count -= 1; }
                *dest = (IoliteValue) { .type = F64, .value = { .f64 = data->value } };
            } break;

            case EQUALS: {
                Instruction_Equals* data = &i->data.equals_data;
                IoliteValue* a = &current_frame->values[data->a];
                IoliteValue* b = &current_frame->values[data->b];
                IoliteValue* result = &current_frame->values[data->dest];
                if(result->type == REFERENCE) { result->value.ref->stack_reference_count -= 1; }
                switch(a->type) {
                    case U8: *result = (IoliteValue) { .type = U8, .value = { .u8 = a->value.u8 == b->value.u8 } }; break;
                    case U16: *result = (IoliteValue) { .type = U16, .value = { .u16 = a->value.u16 == b->value.u16 } }; break;
                    case U32: *result = (IoliteValue) { .type = U32, .value = { .u32 = a->value.u32 == b->value.u32 } }; break;
                    case U64: *result = (IoliteValue) { .type = U64, .value = { .u64 = a->value.u64 == b->value.u64 } }; break;
                    case S8: *result = (IoliteValue) { .type = S8, .value = { .s8 = a->value.s8 == b->value.s8 } }; break;
                    case S16: *result = (IoliteValue) { .type = S16, .value = { .s16 = a->value.s16 == b->value.s16 } }; break;
                    case S32: *result = (IoliteValue) { .type = S32, .value = { .s32 = a->value.s32 == b->value.s32 } }; break;
                    case S64: *result = (IoliteValue) { .type = S64, .value = { .s64 = a->value.s64 == b->value.s64 } }; break;
                    case F32: *result = (IoliteValue) { .type = F32, .value = { .f32 = a->value.f32 == b->value.f32 } }; break;
                    case F64: *result = (IoliteValue) { .type = F64, .value = { .f64 = a->value.f64 == b->value.f64 } }; break;
                    case REFERENCE: *result = (IoliteValue) { .type = U8, .value = { .u8 = a->value.ref == b->value.ref } }; break;
                    case UNIT: break;
                }
            } break;
            case NOT_EQUALS: {
                Instruction_NotEquals* data = &i->data.not_equals_data;
                IoliteValue* a = &current_frame->values[data->a];
                IoliteValue* b = &current_frame->values[data->b];
                IoliteValue* result = &current_frame->values[data->dest];
                if(result->type == REFERENCE) { result->value.ref->stack_reference_count -= 1; }
                switch(a->type) {
                    case U8: *result = (IoliteValue) { .type = U8, .value = { .u8 = a->value.u8 != b->value.u8 } }; break;
                    case U16: *result = (IoliteValue) { .type = U16, .value = { .u16 = a->value.u16 != b->value.u16 } }; break;
                    case U32: *result = (IoliteValue) { .type = U32, .value = { .u32 = a->value.u32 != b->value.u32 } }; break;
                    case U64: *result = (IoliteValue) { .type = U64, .value = { .u64 = a->value.u64 != b->value.u64 } }; break;
                    case S8: *result = (IoliteValue) { .type = S8, .value = { .s8 = a->value.s8 != b->value.s8 } }; break;
                    case S16: *result = (IoliteValue) { .type = S16, .value = { .s16 = a->value.s16 != b->value.s16 } }; break;
                    case S32: *result = (IoliteValue) { .type = S32, .value = { .s32 = a->value.s32 != b->value.s32 } }; break;
                    case S64: *result = (IoliteValue) { .type = S64, .value = { .s64 = a->value.s64 != b->value.s64 } }; break;
                    case F32: *result = (IoliteValue) { .type = F32, .value = { .f32 = a->value.f32 != b->value.f32 } }; break;
                    case F64: *result = (IoliteValue) { .type = F64, .value = { .f64 = a->value.f64 != b->value.f64 } }; break;
                    case REFERENCE: *result = (IoliteValue) { .type = U8, .value = { .u8 = a->value.ref != b->value.ref } }; break;
                    case UNIT: break;
                }
            } break;
            case LESS_THAN: {
                Instruction_LessThan* data = &i->data.less_than_data;
                IoliteValue* a = &current_frame->values[data->a];
                IoliteValue* b = &current_frame->values[data->b];
                IoliteValue* result = &current_frame->values[data->dest];
                if(result->type == REFERENCE) { result->value.ref->stack_reference_count -= 1; }
                BI_OPERATION(<);
            } break;
            case GREATER_THAN: {
                Instruction_GreaterThan* data = &i->data.greater_than_data;
                IoliteValue* a = &current_frame->values[data->a];
                IoliteValue* b = &current_frame->values[data->b];
                IoliteValue* result = &current_frame->values[data->dest];
                if(result->type == REFERENCE) { result->value.ref->stack_reference_count -= 1; }
                BI_OPERATION(>);
            } break;
            case LESS_THAN_EQUALS: {
                Instruction_LessThanEquals* data = &i->data.less_than_equals_data;
                IoliteValue* a = &current_frame->values[data->a];
                IoliteValue* b = &current_frame->values[data->b];
                IoliteValue* result = &current_frame->values[data->dest];
                if(result->type == REFERENCE) { result->value.ref->stack_reference_count -= 1; }
                BI_OPERATION(<=);
            } break;
            case GREATER_THAN_EQUALS: {
                Instruction_GreaterThanEquals* data = &i->data.greater_than_equals_data;
                IoliteValue* a = &current_frame->values[data->a];
                IoliteValue* b = &current_frame->values[data->b];
                IoliteValue* result = &current_frame->values[data->dest];
                if(result->type == REFERENCE) { result->value.ref->stack_reference_count -= 1; }
                BI_OPERATION(>=);
            } break;
            case NOT: {
                Instruction_Not* data = &i->data.not_data;
                IoliteValue* x = &current_frame->values[data->x];
                IoliteValue* result = &current_frame->values[data->dest];
                if(result->type == REFERENCE) { result->value.ref->stack_reference_count -= 1; }
                switch(x->type) {
                    case U8: *result = (IoliteValue) { .type = U8, .value = { .u8 = !x->value.u8 } }; break;
                    case U16: *result = (IoliteValue) { .type = U16, .value = { .u16 = !x->value.u16 } }; break;
                    case U32: *result = (IoliteValue) { .type = U32, .value = { .u32 = !x->value.u32 } }; break;
                    case U64: *result = (IoliteValue) { .type = U64, .value = { .u64 = !x->value.u64 } }; break;
                    case S8: *result = (IoliteValue) { .type = S8, .value = { .s8 = !x->value.s8 } }; break;
                    case S16: *result = (IoliteValue) { .type = S16, .value = { .s16 = !x->value.s16 } }; break;
                    case S32: *result = (IoliteValue) { .type = S32, .value = { .s32 = !x->value.s32 } }; break;
                    case S64: *result = (IoliteValue) { .type = S64, .value = { .s64 = !x->value.s64 } }; break;
                    case F32: *result = (IoliteValue) { .type = F32, .value = { .f32 = !x->value.f32 } }; break;
                    case F64: *result = (IoliteValue) { .type = F64, .value = { .f64 = !x->value.f64 } }; break;
                    default: {
                        printf("Cannot negate a non-number value!\n");
                        exit(1);
                    }
                }
            } break;

            case ADD: {
                Instruction_Add* data = &i->data.add_data;
                IoliteValue* a = &current_frame->values[data->a];
                IoliteValue* b = &current_frame->values[data->b];
                IoliteValue* result = &current_frame->values[data->dest];
                if(result->type == REFERENCE) { result->value.ref->stack_reference_count -= 1; }
                BI_OPERATION(+);
            } break;
            case SUBTRACT: {
                Instruction_Subtract* data = &i->data.subtract_data;
                IoliteValue* a = &current_frame->values[data->a];
                IoliteValue* b = &current_frame->values[data->b];
                IoliteValue* result = &current_frame->values[data->dest];
                if(result->type == REFERENCE) { result->value.ref->stack_reference_count -= 1; }
                BI_OPERATION(-);
            } break;
            case MULTIPLY: {
                Instruction_Multiply* data = &i->data.multiply_data;
                IoliteValue* a = &current_frame->values[data->a];
                IoliteValue* b = &current_frame->values[data->b];
                IoliteValue* result = &current_frame->values[data->dest];
                if(result->type == REFERENCE) { result->value.ref->stack_reference_count -= 1; }
                BI_OPERATION(*);
            } break;
            case DIVIDE: {
                Instruction_Divide* data = &i->data.divide_data;
                IoliteValue* a = &current_frame->values[data->a];
                IoliteValue* b = &current_frame->values[data->b];
                IoliteValue* result = &current_frame->values[data->dest];
                if(result->type == REFERENCE) { result->value.ref->stack_reference_count -= 1; }
                BI_OPERATION(/);
            } break;
            case MODULO: {
                Instruction_Modulo* data = &i->data.modulo_data;
                IoliteValue* a = &current_frame->values[data->a];
                IoliteValue* b = &current_frame->values[data->b];
                IoliteValue* result = &current_frame->values[data->dest];
                if(result->type == REFERENCE) { result->value.ref->stack_reference_count -= 1; }
                switch(a->type) {
                    case U8: *result = (IoliteValue) { .type = U8, .value = { .u8 = a->value.u8 % b->value.u8 } }; break;
                    case U16: *result = (IoliteValue) { .type = U16, .value = { .u16 = a->value.u16 % b->value.u16 } }; break;
                    case U32: *result = (IoliteValue) { .type = U32, .value = { .u32 = a->value.u32 % b->value.u32 } }; break;
                    case U64: *result = (IoliteValue) { .type = U64, .value = { .u64 = a->value.u64 % b->value.u64 } }; break;
                    case S8: *result = (IoliteValue) { .type = S8, .value = { .s8 = a->value.s8 % b->value.s8 } }; break;
                    case S16: *result = (IoliteValue) { .type = S16, .value = { .s16 = a->value.s16 % b->value.s16 } }; break;
                    case S32: *result = (IoliteValue) { .type = S32, .value = { .s32 = a->value.s32 % b->value.s32 } }; break;
                    case S64: *result = (IoliteValue) { .type = S64, .value = { .s64 = a->value.s64 % b->value.s64 } }; break;
                    case F32: *result = (IoliteValue) { .type = F32, .value = { .f32 = (float) fmod(a->value.f32, b->value.f32) } }; break;
                    case F64: *result = (IoliteValue) { .type = F64, .value = { .f64 = fmod(a->value.f64, b->value.f64) } }; break;
                    default: {
                        printf("Cannot perform modulo on a non-number value!\n");
                        exit(1);
                    }
                }
            } break;
            case NEGATE: {
                Instruction_Negate* data = &i->data.negate_data;
                IoliteValue* x = &current_frame->values[data->x];
                IoliteValue* result = &current_frame->values[data->dest];
                if(result->type == REFERENCE) { result->value.ref->stack_reference_count -= 1; }
                switch(x->type) {
                    case U8: *result = (IoliteValue) { .type = U8, .value = { .u8 = -x->value.u8 } }; break;
                    case U16: *result = (IoliteValue) { .type = U16, .value = { .u16 = -x->value.u16 } }; break;
                    case U32: *result = (IoliteValue) { .type = U32, .value = { .u32 = -x->value.u32 } }; break;
                    case U64: *result = (IoliteValue) { .type = U64, .value = { .u64 = -x->value.u64 } }; break;
                    case S8: *result = (IoliteValue) { .type = S8, .value = { .s8 = -x->value.s8 } }; break;
                    case S16: *result = (IoliteValue) { .type = S16, .value = { .s16 = -x->value.s16 } }; break;
                    case S32: *result = (IoliteValue) { .type = S32, .value = { .s32 = -x->value.s32 } }; break;
                    case S64: *result = (IoliteValue) { .type = S64, .value = { .s64 = -x->value.s64 } }; break;
                    case F32: *result = (IoliteValue) { .type = F32, .value = { .f32 = -x->value.f32 } }; break;
                    case F64: *result = (IoliteValue) { .type = F64, .value = { .f64 = -x->value.f64 } }; break;
                    default: {
                        printf("Cannot perform negation on a non-number value!\n");
                        exit(1);
                    }
                }
            } break;

            case CONVERT_U8: {
                Instruction_ConvertU8* data = &i->data.convert_u8_data;
                IoliteValue* x = &current_frame->values[data->x];
                uint8_t value;
                CONVERSION_TO(uint8_t);
                IoliteValue* dest = &current_frame->values[data->dest];
                if(dest->type == REFERENCE) { dest->value.ref->stack_reference_count -= 1; }
                *dest = (IoliteValue) { .type = U8, .value = { .u8 = value } };
            } break;
            case CONVERT_U16: {
                Instruction_ConvertU16* data = &i->data.convert_u16_data;
                IoliteValue* x = &current_frame->values[data->x];
                uint16_t value;
                CONVERSION_TO(uint16_t);
                IoliteValue* dest = &current_frame->values[data->dest];
                if(dest->type == REFERENCE) { dest->value.ref->stack_reference_count -= 1; }
                *dest = (IoliteValue) { .type = U16, .value = { .u16 = value } };
            } break;
            case CONVERT_U32: {
                Instruction_ConvertU32* data = &i->data.convert_u32_data;
                IoliteValue* x = &current_frame->values[data->x];
                uint32_t value;
                CONVERSION_TO(uint32_t);
                IoliteValue* dest = &current_frame->values[data->dest];
                if(dest->type == REFERENCE) { dest->value.ref->stack_reference_count -= 1; }
                *dest = (IoliteValue) { .type = U32, .value = { .u32 = value } };
            } break;
            case CONVERT_U64: {
                Instruction_ConvertU64* data = &i->data.convert_u64_data;
                IoliteValue* x = &current_frame->values[data->x];
                uint64_t value;
                CONVERSION_TO(uint64_t);
                IoliteValue* dest = &current_frame->values[data->dest];
                if(dest->type == REFERENCE) { dest->value.ref->stack_reference_count -= 1; }
                *dest = (IoliteValue) { .type = U64, .value = { .u64 = value } };
            } break;
            case CONVERT_S8: {
                Instruction_ConvertS8* data = &i->data.convert_s8_data;
                IoliteValue* x = &current_frame->values[data->x];
                int8_t value;
                CONVERSION_TO(int8_t);
                IoliteValue* dest = &current_frame->values[data->dest];
                if(dest->type == REFERENCE) { dest->value.ref->stack_reference_count -= 1; }
                *dest = (IoliteValue) { .type = S8, .value = { .s8 = value } };
            } break;
            case CONVERT_S16: {
                Instruction_ConvertS16* data = &i->data.convert_s16_data;
                IoliteValue* x = &current_frame->values[data->x];
                int16_t value;
                CONVERSION_TO(int16_t);
                IoliteValue* dest = &current_frame->values[data->dest];
                if(dest->type == REFERENCE) { dest->value.ref->stack_reference_count -= 1; }
                *dest = (IoliteValue) { .type = S16, .value = { .s16 = value } };
            } break;
            case CONVERT_S32: {
                Instruction_ConvertS32* data = &i->data.convert_s32_data;
                IoliteValue* x = &current_frame->values[data->x];
                int32_t value;
                CONVERSION_TO(int32_t);
                IoliteValue* dest = &current_frame->values[data->dest];
                if(dest->type == REFERENCE) { dest->value.ref->stack_reference_count -= 1; }
                *dest = (IoliteValue) { .type = S32, .value = { .s32 = value } };
            } break;
            case CONVERT_S64: {
                Instruction_ConvertS64* data = &i->data.convert_s64_data;
                IoliteValue* x = &current_frame->values[data->x];
                int64_t value;
                CONVERSION_TO(int64_t);
                IoliteValue* dest = &current_frame->values[data->dest];
                if(dest->type == REFERENCE) { dest->value.ref->stack_reference_count -= 1; }
                *dest = (IoliteValue) { .type = S64, .value = { .s64 = value } };
            } break;
            case CONVERT_F32: {
                Instruction_ConvertF32* data = &i->data.convert_f32_data;
                IoliteValue* x = &current_frame->values[data->x];
                float value;
                CONVERSION_TO(float);
                IoliteValue* dest = &current_frame->values[data->dest];
                if(dest->type == REFERENCE) { dest->value.ref->stack_reference_count -= 1; }
                *dest = (IoliteValue) { .type = F32, .value = { .f32 = value } };
            } break;
            case CONVERT_F64: {
                Instruction_ConvertF64* data = &i->data.convert_f64_data;
                IoliteValue* x = &current_frame->values[data->x];
                double value;
                CONVERSION_TO(double);
                IoliteValue* dest = &current_frame->values[data->dest];
                if(dest->type == REFERENCE) { dest->value.ref->stack_reference_count -= 1; }
                *dest = (IoliteValue) { .type = F64, .value = { .f64 = value } };
            } break;

            case RESOLVED_CALL: {
                Instruction_ResolvedCall* data = &i->data.resolved_call_data;
                Frame call_frame;
                call_frame.size = data->function->argc + data->function->varc;
                call_frame.values = malloc(sizeof(IoliteValue) * call_frame.size);
                for(VarIdx var_index = 0; var_index < call_frame.size; var_index += 1) {
                    call_frame.values[var_index].type = UNIT;
                }
                for(VarIdx arg_index = 0; arg_index < data->function->argc; arg_index += 1) {
                    IoliteValue* arg = &current_frame->values[data->argv[arg_index]];
                    if(arg->type == REFERENCE) { arg->value.ref->stack_reference_count += 1; }
                    call_frame.values[arg_index] = *arg;
                }
                vector_push(&frames, &call_frame);
                InstrC next_index = current_index + 1;
                vector_push(&return_idx, &next_index);
                current_index = data->function->instruction_index;
                return_val_dest_var = data->returned;
                current_frame = vector_get(&frames, frames.size - 1);
                continue;
            } break;
            case RESOLVED_EXTERNAL_CALL: {
                Instruction_ResolvedExternalCall* data = &i->data.resolved_external_call_data;
                IoliteValue* argv = malloc(sizeof(IoliteValue) * data->argc);
                for(VarIdx arg = 0; arg < data->argc; arg += 1) {
                    argv[arg] = current_frame->values[data->argv[arg]];
                }
                IoliteValue return_val = ((IoliteExternalFunction) data->function)(argv);
                free(argv);
                if(return_val.type != UNIT) {
                    IoliteValue* returned_dest = &current_frame->values[data->returned];
                    if(return_val.type == REFERENCE) { returned_dest->value.ref->stack_reference_count += 1; }
                    if(returned_dest->type == REFERENCE) { returned_dest->value.ref->stack_reference_count -= 1; }
                    *returned_dest = return_val;
                }
            } break;

            case MALLOC_DYNAMIC: case MALLOC_FIXED: {
                uint64_t size;
                VarIdx dest;
                if(i->type == MALLOC_DYNAMIC) {
                    Instruction_MallocDynamic* data = &i->data.malloc_dynamic_data;
                    size = current_frame->values[data->size].value.u64;
                    dest = data->dest;
                } else {
                    Instruction_MallocFixed* data = &i->data.malloc_fixed_data;
                    size = data->size;
                    dest = data->dest;
                }
                IoliteAllocation* allocation = gc_allocate(gc, size);
                IoliteValue value = { .type = REFERENCE, .value = { .ref = allocation } };
                IoliteValue* dest_ptr = &current_frame->values[dest];
                if(dest_ptr->type == REFERENCE) { dest_ptr->value.ref->stack_reference_count -= 1; }
                *dest_ptr = value;
            } break;
            case REF_GET_DYNAMIC: case REF_GET_FIXED: {
                VarIdx ref;
                uint64_t index;
                VarIdx dest;
                if(i->type == REF_GET_DYNAMIC) {
                    Instruction_RefGetDynamic* data = &i->data.ref_get_dynamic_data;
                    ref = data->ref;
                    index = current_frame->values[data->index].value.u64;
                    dest = data->dest;
                } else {
                    Instruction_RefGetFixed* data = &i->data.ref_get_fixed_data;
                    ref = data->ref;
                    index = data->index;
                    dest = data->dest;
                }
                IoliteAllocation* allocation = current_frame->values[ref].value.ref;
                IoliteValue* value = &allocation->values[index];
                IoliteValue* dest_ptr = &current_frame->values[dest];
                if(value->type == REFERENCE) { value->value.ref->stack_reference_count += 1; }
                if(dest_ptr->type == REFERENCE) { dest_ptr->value.ref->stack_reference_count -= 1; }
                *dest_ptr = *value;
            } break;
            case REF_SET_DYNAMIC: case REF_SET_FIXED: {
                VarIdx ref;
                uint64_t index;
                VarIdx value;
                if(i->type == REF_SET_DYNAMIC) {
                    Instruction_RefSetDynamic* data = &i->data.ref_set_dynamic_data;
                    ref = data->ref;
                    index = current_frame->values[data->index].value.u64;
                    value = data->value;
                } else {
                    Instruction_RefSetFixed* data = &i->data.ref_set_fixed_data;
                    ref = data->ref;
                    index = data->index;
                    value = data->value;
                }
                IoliteAllocation* allocation = current_frame->values[ref].value.ref;
                IoliteValue* value_ptr = &current_frame->values[value];
                IoliteValue* dest = &allocation->values[index];
                if(value_ptr->type == REFERENCE) { value_ptr->value.ref->stack_reference_count += 1; }
                if(dest->type == REFERENCE) { dest->value.ref->stack_reference_count -= 1; }
                *dest = *value_ptr;
            } break;

            case JUMP: {
                Instruction_Jump* data = &i->data.jump_data;
                current_index = data->dest;
                continue;
            } break;
            case CONDITIONAL_JUMP: {
                Instruction_ConditionalJump* data = &i->data.conditional_jump_data;
                IoliteValue* cond = &current_frame->values[data->condition];
                uint8_t cond_true = 0;
                switch(cond->type) {
                    case U8: cond_true = cond->value.u8 != 0; break;
                    case U16: cond_true = cond->value.u16 != 0; break;
                    case U32: cond_true = cond->value.u32 != 0; break;
                    case U64: cond_true = cond->value.u64 != 0; break;
                    case S8: cond_true = cond->value.s8 != 0; break;
                    case S16: cond_true = cond->value.s16 != 0; break;
                    case S32: cond_true = cond->value.s32 != 0; break;
                    case S64: cond_true = cond->value.s64 != 0; break;
                    case F32: cond_true = cond->value.f32 != 0.0f; break;
                    case F64: cond_true = cond->value.f64 != 0.0; break;
                    case UNIT: break;
                    default: cond_true = 1;
                }
                if(cond_true) { current_index = data->if_dest; }
                else { current_index = data->else_dest; }
                continue;
            } break;
        }
        current_index += 1;
    }
}