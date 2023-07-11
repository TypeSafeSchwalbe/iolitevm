
#include <string.h>
#include <math.h>
#include "runtime.h"


Runtime create_runtime() {
    Runtime r;
    r.functions = create_vector(sizeof(Instruction_Function*));
    r.frames = create_vector(sizeof(Frame));
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

ExecutionSignal execute_instruction(Runtime* r, GC* gc, Instruction* i) {
    switch(i->type) {
        case FUNCTION: /* nothing to do, functions have already been loaded */ break; 
        case CALL: /* call should be resolved, we shouldn't ever encounter this instruction */ break;
        case EXTERNAL_CALL: /* call should be resolved, we shouldn't ever encounter this instruction */ break;
        case RETURN: {
            Instruction_Return* data = &i->data.return_data;
            r->returned_val_var = data->value;
            r->returned_val = 1;
            return RETURN_FUNCTION;
        } break;
        case RETURN_NOTHING: {
            r->returned_val = 0;
            return RETURN_FUNCTION;
        } break;
        
        case IF: {
            Instruction_If* data = &i->data.if_data;
            Frame* current_frame = vector_get(&r->frames, r->frames.size - 1);
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
            }
            if(cond_true) {
                return execute_instructions(r, gc, data->if_body, data->if_body_length);
            } else {
                return execute_instructions(r, gc, data->else_body, data->else_body_length);
            }
        } break;
        case LOOP: {
            Instruction_Loop* data = &i->data.loop_data;
            for(;;) {
                ExecutionSignal sig = execute_instructions(r, gc, data->body, data->body_length);
                if(sig == NEXT_INSTRUCTION || sig == CONTINUE_LOOP) continue;
                if(sig == BREAK_LOOP) break;
                return RETURN_FUNCTION;
            }
        } break;
        case BREAK: {
            return BREAK_LOOP;
        } break;
        case CONTINUE: {
            return CONTINUE_LOOP;
        } break;

        case COPY: {
            Instruction_Copy* data = &i->data.copy_data;
            Frame* current_frame = vector_get(&r->frames, r->frames.size - 1);
            if(data->src == data->dest) { break; }
            IoliteValue* src = &current_frame->values[data->src];
            IoliteValue* dest = &current_frame->values[data->dest];
            if(src->type == REFERENCE) { src->value.ref->stack_reference_count += 1; }
            if(dest->type == REFERENCE) { src->value.ref->stack_reference_count -= 1; }
            *dest = *src;
        } break;

        case PUT_U8: {
            Instruction_PutU8* data = &i->data.put_u8_data;
            Frame* current_frame = vector_get(&r->frames, r->frames.size - 1);
            current_frame->values[data->dest] = (IoliteValue) { .type = U8, .value = { .u8 = data->value } };
        } break;
        case PUT_U16: {
            Instruction_PutU16* data = &i->data.put_u16_data;
            Frame* current_frame = vector_get(&r->frames, r->frames.size - 1);
            current_frame->values[data->dest] = (IoliteValue) { .type = U16, .value = { .u16 = data->value } };
        } break;
        case PUT_U32: {
            Instruction_PutU32* data = &i->data.put_u32_data;
            Frame* current_frame = vector_get(&r->frames, r->frames.size - 1);
            current_frame->values[data->dest] = (IoliteValue) { .type = U32, .value = { .u32 = data->value } };
        } break;
        case PUT_U64: {
            Instruction_PutU64* data = &i->data.put_u64_data;
            Frame* current_frame = vector_get(&r->frames, r->frames.size - 1);
            current_frame->values[data->dest] = (IoliteValue) { .type = U64, .value = { .u64 = data->value } };
        } break;
        case PUT_S8: {
            Instruction_PutS8* data = &i->data.put_s8_data;
            Frame* current_frame = vector_get(&r->frames, r->frames.size - 1);
            current_frame->values[data->dest] = (IoliteValue) { .type = S8, .value = { .s8 = data->value } };
        } break;
        case PUT_S16: {
            Instruction_PutS16* data = &i->data.put_s16_data;
            Frame* current_frame = vector_get(&r->frames, r->frames.size - 1);
            current_frame->values[data->dest] = (IoliteValue) { .type = S16, .value = { .s16 = data->value } };
        } break;
        case PUT_S32: {
            Instruction_PutS32* data = &i->data.put_s32_data;
            Frame* current_frame = vector_get(&r->frames, r->frames.size - 1);
            current_frame->values[data->dest] = (IoliteValue) { .type = S32, .value = { .s32 = data->value } };
        } break;
        case PUT_S64: {
            Instruction_PutS64* data = &i->data.put_s64_data;
            Frame* current_frame = vector_get(&r->frames, r->frames.size - 1);
            current_frame->values[data->dest] = (IoliteValue) { .type = S64, .value = { .s64 = data->value } };
        } break;
        case PUT_F32: {
            Instruction_PutF32* data = &i->data.put_f32_data;
            Frame* current_frame = vector_get(&r->frames, r->frames.size - 1);
            current_frame->values[data->dest] = (IoliteValue) { .type = F32, .value = { .f32 = data->value } };
        } break;
        case PUT_F64: {
            Instruction_PutF64* data = &i->data.put_f64_data;
            Frame* current_frame = vector_get(&r->frames, r->frames.size - 1);
            current_frame->values[data->dest] = (IoliteValue) { .type = F64, .value = { .f64 = data->value } };
        } break;

        case EQUALS: {
            Instruction_Equals* data = &i->data.equals_data;
            Frame* current_frame = vector_get(&r->frames, r->frames.size - 1);
            IoliteValue* a = &current_frame->values[data->a];
            IoliteValue* b = &current_frame->values[data->b];
            IoliteValue* result = &current_frame->values[data->dest];
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
            Frame* current_frame = vector_get(&r->frames, r->frames.size - 1);
            IoliteValue* a = &current_frame->values[data->a];
            IoliteValue* b = &current_frame->values[data->b];
            IoliteValue* result = &current_frame->values[data->dest];
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
            Frame* current_frame = vector_get(&r->frames, r->frames.size - 1);
            IoliteValue* a = &current_frame->values[data->a];
            IoliteValue* b = &current_frame->values[data->b];
            IoliteValue* result = &current_frame->values[data->dest];
            BI_OPERATION(<);
        } break;
        case GREATER_THAN: {
            Instruction_GreaterThan* data = &i->data.greater_than_data;
            Frame* current_frame = vector_get(&r->frames, r->frames.size - 1);
            IoliteValue* a = &current_frame->values[data->a];
            IoliteValue* b = &current_frame->values[data->b];
            IoliteValue* result = &current_frame->values[data->dest];
            BI_OPERATION(>);
        } break;
        case LESS_THAN_EQUALS: {
            Instruction_LessThanEquals* data = &i->data.less_than_equals_data;
            Frame* current_frame = vector_get(&r->frames, r->frames.size - 1);
            IoliteValue* a = &current_frame->values[data->a];
            IoliteValue* b = &current_frame->values[data->b];
            IoliteValue* result = &current_frame->values[data->dest];
            BI_OPERATION(<=);
        } break;
        case GREATER_THAN_EQUALS: {
            Instruction_GreaterThanEquals* data = &i->data.greater_than_equals_data;
            Frame* current_frame = vector_get(&r->frames, r->frames.size - 1);
            IoliteValue* a = &current_frame->values[data->a];
            IoliteValue* b = &current_frame->values[data->b];
            IoliteValue* result = &current_frame->values[data->dest];
            BI_OPERATION(>=);
        } break;
        case NOT: {
            Instruction_Not* data = &i->data.not_data;
            Frame* current_frame = vector_get(&r->frames, r->frames.size - 1);
            IoliteValue* x = &current_frame->values[data->x];
            IoliteValue* result = &current_frame->values[data->dest];
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
            Frame* current_frame = vector_get(&r->frames, r->frames.size - 1);
            IoliteValue* a = &current_frame->values[data->a];
            IoliteValue* b = &current_frame->values[data->b];
            IoliteValue* result = &current_frame->values[data->dest];
            BI_OPERATION(+);
        } break;
        case SUBTRACT: {
            Instruction_Subtract* data = &i->data.subtract_data;
            Frame* current_frame = vector_get(&r->frames, r->frames.size - 1);
            IoliteValue* a = &current_frame->values[data->a];
            IoliteValue* b = &current_frame->values[data->b];
            IoliteValue* result = &current_frame->values[data->dest];
            BI_OPERATION(-);
        } break;
        case MULTIPLY: {
            Instruction_Multiply* data = &i->data.multiply_data;
            Frame* current_frame = vector_get(&r->frames, r->frames.size - 1);
            IoliteValue* a = &current_frame->values[data->a];
            IoliteValue* b = &current_frame->values[data->b];
            IoliteValue* result = &current_frame->values[data->dest];
            BI_OPERATION(*);
        } break;
        case DIVIDE: {
            Instruction_Divide* data = &i->data.divide_data;
            Frame* current_frame = vector_get(&r->frames, r->frames.size - 1);
            IoliteValue* a = &current_frame->values[data->a];
            IoliteValue* b = &current_frame->values[data->b];
            IoliteValue* result = &current_frame->values[data->dest];
            BI_OPERATION(/);
        } break;
        case MODULO: {
            Instruction_Modulo* data = &i->data.modulo_data;
            Frame* current_frame = vector_get(&r->frames, r->frames.size - 1);
            IoliteValue* a = &current_frame->values[data->a];
            IoliteValue* b = &current_frame->values[data->b];
            IoliteValue* result = &current_frame->values[data->dest];
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
            Frame* current_frame = vector_get(&r->frames, r->frames.size - 1);
            IoliteValue* x = &current_frame->values[data->x];
            IoliteValue* result = &current_frame->values[data->dest];
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
            Frame* current_frame = vector_get(&r->frames, r->frames.size - 1);
            IoliteValue* x = &current_frame->values[data->x];
            uint8_t value;
            CONVERSION_TO(uint8_t);
            current_frame->values[data->dest] = (IoliteValue) { .type = U8, .value = { .u8 = value } };
        } break;
        case CONVERT_U16: {
            Instruction_ConvertU16* data = &i->data.convert_u16_data;
            Frame* current_frame = vector_get(&r->frames, r->frames.size - 1);
            IoliteValue* x = &current_frame->values[data->x];
            uint16_t value;
            CONVERSION_TO(uint16_t);
            current_frame->values[data->dest] = (IoliteValue) { .type = U16, .value = { .u16 = value } };
        } break;
        case CONVERT_U32: {
            Instruction_ConvertU32* data = &i->data.convert_u32_data;
            Frame* current_frame = vector_get(&r->frames, r->frames.size - 1);
            IoliteValue* x = &current_frame->values[data->x];
            uint32_t value;
            CONVERSION_TO(uint32_t);
            current_frame->values[data->dest] = (IoliteValue) { .type = U32, .value = { .u32 = value } };
        } break;
        case CONVERT_U64: {
            Instruction_ConvertU64* data = &i->data.convert_u64_data;
            Frame* current_frame = vector_get(&r->frames, r->frames.size - 1);
            IoliteValue* x = &current_frame->values[data->x];
            uint64_t value;
            CONVERSION_TO(uint64_t);
            current_frame->values[data->dest] = (IoliteValue) { .type = U64, .value = { .u64 = value } };
        } break;
        case CONVERT_S8: {
            Instruction_ConvertS8* data = &i->data.convert_s8_data;
            Frame* current_frame = vector_get(&r->frames, r->frames.size - 1);
            IoliteValue* x = &current_frame->values[data->x];
            int8_t value;
            CONVERSION_TO(int8_t);
            current_frame->values[data->dest] = (IoliteValue) { .type = S8, .value = { .s8 = value } };
        } break;
        case CONVERT_S16: {
            Instruction_ConvertS16* data = &i->data.convert_s16_data;
            Frame* current_frame = vector_get(&r->frames, r->frames.size - 1);
            IoliteValue* x = &current_frame->values[data->x];
            int16_t value;
            CONVERSION_TO(int16_t);
            current_frame->values[data->dest] = (IoliteValue) { .type = S16, .value = { .s16 = value } };
        } break;
        case CONVERT_S32: {
            Instruction_ConvertS32* data = &i->data.convert_s32_data;
            Frame* current_frame = vector_get(&r->frames, r->frames.size - 1);
            IoliteValue* x = &current_frame->values[data->x];
            int32_t value;
            CONVERSION_TO(int32_t);
            current_frame->values[data->dest] = (IoliteValue) { .type = S32, .value = { .s32 = value } };
        } break;
        case CONVERT_S64: {
            Instruction_ConvertS64* data = &i->data.convert_s64_data;
            Frame* current_frame = vector_get(&r->frames, r->frames.size - 1);
            IoliteValue* x = &current_frame->values[data->x];
            int64_t value;
            CONVERSION_TO(int64_t);
            current_frame->values[data->dest] = (IoliteValue) { .type = S64, .value = { .s64 = value } };
        } break;
        case CONVERT_F32: {
            Instruction_ConvertF32* data = &i->data.convert_f32_data;
            Frame* current_frame = vector_get(&r->frames, r->frames.size - 1);
            IoliteValue* x = &current_frame->values[data->x];
            float value;
            CONVERSION_TO(float);
            current_frame->values[data->dest] = (IoliteValue) { .type = F32, .value = { .f32 = value } };
        } break;
        case CONVERT_F64: {
            Instruction_ConvertF64* data = &i->data.convert_f64_data;
            Frame* current_frame = vector_get(&r->frames, r->frames.size - 1);
            IoliteValue* x = &current_frame->values[data->x];
            double value;
            CONVERSION_TO(double);
            current_frame->values[data->dest] = (IoliteValue) { .type = F64, .value = { .f64 = value } };
        } break;

        case RESOLVED_CALL: {
            Instruction_ResolvedCall* data = &i->data.resolved_call_data;
            Frame* current_frame = r->frames.size > 0? vector_get(&r->frames, r->frames.size - 1) : NULL;
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
            vector_push(&r->frames, &call_frame);
            execute_instructions(r, gc, data->function->body, data->function->body_length);
            vector_pop(&r->frames);
            if(r->returned_val != 0) {
                IoliteValue* returned_val = &call_frame.values[r->returned_val_var];
                IoliteValue* returned_dest = &current_frame->values[data->returned];
                if(returned_val->type == REFERENCE) { returned_val->value.ref->stack_reference_count += 1; }
                if(returned_dest->type == REFERENCE) { returned_dest->value.ref->stack_reference_count -= 1; }
                *returned_dest = *returned_val;
            }
            for(VarIdx var_index = 0; var_index < call_frame.size; var_index += 1) {
                IoliteValue* var_val = &call_frame.values[var_index];
                if(var_val->type == REFERENCE) { var_val->value.ref->stack_reference_count -= 1; }
            }
            free(call_frame.values);
        } break;
        case RESOLVED_EXTERNAL_CALL: {
            Instruction_ResolvedExternalCall* data = &i->data.resolved_external_call_data;
            Frame* current_frame = vector_get(&r->frames, r->frames.size - 1);
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
            Frame* current_frame = vector_get(&r->frames, r->frames.size - 1);
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
            Frame* current_frame = vector_get(&r->frames, r->frames.size - 1);
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
            Frame* current_frame = vector_get(&r->frames, r->frames.size - 1);
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
    }
    return NEXT_INSTRUCTION;
}

ExecutionSignal execute_instructions(Runtime* r, GC* gc, Instruction* instructions, InstrC instruction_count) {
    for(InstrC instruction_index = 0; instruction_index < instruction_count; instruction_index += 1) {
        ExecutionSignal sig = execute_instruction(r, gc, &instructions[instruction_index]);
        switch(sig) {
            case NEXT_INSTRUCTION: continue;
            default: return sig;
        }
    }
    return NEXT_INSTRUCTION;
}