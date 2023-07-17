
#include <string.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include "runtime.h"
#include "vector.h"


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
                // [CONDITION BODY] (* condition_count)
                // [FUNCTION BODY]
                length += 1
                    + 1;
                for(uint16_t condition_index = 0; condition_index < data->condition_count; condition_index += 1) {
                    length += calculate_flat_length(data->conditions[condition_index], data->condition_lengths[condition_index]);
                }
                length += calculate_flat_length(data->body, data->body_length);
            } break;
            case PUT_CLOSURE: {
                Instruction_PutClosure* data = &i->data.put_closure_data;
                // [PUT CLOSURE]
                // ----------------
                // <put closure>
                // <jump>
                // [CLOSURE BODY]
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
                InstrC* jump_dest = &dest_instructions[*absolute_index].data.jump_data.dest;
                *absolute_index += 1;
                dest_instructions[*absolute_index] = *i;
                InstrC* body_idx = &dest_instructions[*absolute_index].data.function_data.body_instruction_index;
                *absolute_index += 1;
                *body_idx = *absolute_index;
                for(uint16_t condition_index = 0; condition_index < data->condition_count; condition_index += 1) {
                    flatten_instructions(data->conditions[condition_index], data->condition_lengths[condition_index], dest_instructions, absolute_index, parent_loop_absolute_start, parent_loop_absolute_post);
                }
                flatten_instructions(data->body, data->body_length, dest_instructions, absolute_index, parent_loop_absolute_start, parent_loop_absolute_post);
                *jump_dest = *absolute_index;
            } break;
            case PUT_CLOSURE: {
                Instruction_PutClosure* data = &i->data.put_closure_data;
                dest_instructions[*absolute_index] = *i;
                InstrC* body_idx = &dest_instructions[*absolute_index].data.put_closure_data.instruction_index;
                *absolute_index += 1;
                dest_instructions[*absolute_index] = (Instruction) { .type = JUMP, .data = { .jump_data = {
                    .dest = 0
                } } };
                InstrC* jump_dest = &dest_instructions[*absolute_index].data.jump_data.dest;
                *absolute_index += 1;
                *body_idx = *absolute_index;
                flatten_instructions(data->body, data->body_length, dest_instructions, absolute_index, parent_loop_absolute_start, parent_loop_absolute_post);
                *jump_dest = *absolute_index;
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
                dest_instructions[*absolute_index] = *i;
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


char* as_null_terminated(char* src, size_t length) {
    char* null_terminated = malloc(length + 1);
    memcpy(null_terminated, src, length);
    null_terminated[length] = '\0';
    return null_terminated;
}


void resolve_symbols(DLibLoader* l, Instruction* instructions, InstrC instruction_count) {
    // discover functions and traits
    Vector functions = create_vector(sizeof(Instruction_Function*));
    Vector traits = create_vector(sizeof(Instruction_Trait*));
    for(InstrC instruction_index = 0; instruction_index < instruction_count; instruction_index += 1) {
        Instruction* instruction = &instructions[instruction_index];
        switch(instruction->type) {
            case FUNCTION: {
                Instruction_Function* function_data = &instruction->data.function_data;
                vector_push(&functions, &function_data);
            } break;
            case TRAIT: {
                Instruction_Trait* trait_data = &instruction->data.trait_data;
                trait_data->trait_id = traits.size;
                vector_push(&traits, &trait_data);
            }

            default: {}
        }
    }
    // resolve function and trait usages and discover implements
    Vector implements = create_vector(sizeof(Instruction_ResolvedImplements*));
    for(InstrC instruction_index = 0; instruction_index < instruction_count; instruction_index += 1) {
        Instruction* instruction = &instructions[instruction_index];
        switch(instruction->type) {
            case CALL: case ASYNC_CALL: {
                MString* name;
                if(instruction->type == CALL) {
                    Instruction_Call* data = &instruction->data.call_data;
                    name = &data->name;
                } else {
                    Instruction_AsyncCall* data = &instruction->data.async_call_data;
                    name = &data->name;
                }
                // try to find the function in all loaded functions
                uint8_t found = 0;
                for(size_t function_index = 0; function_index < functions.size; function_index += 1) {
                    Instruction_Function** function = vector_get(&functions, function_index);
                    if((*function)->name.length == name->length
                    && memcmp((*function)->name.data, name->data, name->length) == 0) {
                        // replace with resolved
                        if(instruction->type == CALL) {
                            Instruction_Call* data = &instruction->data.call_data;
                            *instruction = (Instruction) {
                                .type = RESOLVED_CALL,
                                .data = { .resolved_call_data = {
                                    .function = *function,
                                    .argv = data->argv,
                                    .returned = data->returned
                                } }
                            };
                        } else {
                            Instruction_AsyncCall* data = &instruction->data.async_call_data;
                            *instruction = (Instruction) {
                                .type = RESOLVED_ASYNC_CALL,
                                .data = { .resolved_async_call_data = {
                                    .function = *function,
                                    .argv = data->argv,
                                    .returned = data->returned
                                } }
                            };
                        }
                        found = 1;
                        break;
                    }
                }
                if(found) { continue; }
                // still not found? -> error
                char* name_null_terminated = as_null_terminated(name->data, name->length);
                printf("No function with the name '%s' could be found in any loaded module.\n", name_null_terminated);
                free(name_null_terminated);
                exit(1);
            } break;
            case EXTERNAL_CALL: {
                Instruction_ExternalCall* data = &instruction->data.external_call_data;
                // try to find the function in all loaded shared libraries
                char* name_null_terminated = as_null_terminated(data->name.data, data->name.length);
                void* external_function = dlibs_find(l, name_null_terminated);
                if(external_function != NULL) {
                    // replace with resolved
                    *instruction = (Instruction) {
                        .type = RESOLVED_EXTERNAL_CALL,
                        .data = { .resolved_external_call_data = {
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

            case IMPLEMENTS: {
                Instruction_Implements* data = &instruction->data.implements_data;
                Instruction_Trait** contained_trait_pointers = malloc(sizeof(Instruction_Trait*) * data->trait_count);
                Instruction_Function*** contained_method_impl_pointers = malloc(sizeof(Instruction_Function**) * data->trait_count);
                // find all implemented traits
                for(uint16_t contained_trait_index = 0; contained_trait_index < data->trait_count; contained_trait_index += 1) {
                    MString* contained_trait_name = &data->trait_names[contained_trait_index];
                    uint8_t trait_found = 0;
                    // try to find the contained trait
                    for(size_t loaded_trait_index = 0; loaded_trait_index < traits.size; loaded_trait_index += 1) {
                        Instruction_Trait* loaded_trait = *((Instruction_Trait**) vector_get(&traits, loaded_trait_index));
                        if(contained_trait_name->length != loaded_trait->name.length
                        || memcmp(loaded_trait->name.data, contained_trait_name->data, contained_trait_name->length) != 0) { continue; }
                        // found!
                        contained_trait_pointers[contained_trait_index] = loaded_trait;
                        Instruction_Function** trait_method_impl_pointers = malloc(sizeof(Instruction_Function*) * loaded_trait->method_count);
                        contained_method_impl_pointers[contained_trait_index] = trait_method_impl_pointers;
                        // find all method implementations
                        for(size_t contained_function_index = 0; contained_function_index < loaded_trait->method_count; contained_function_index += 1) {
                            MString* contained_impl_function_name = data->trait_impl_function_names[contained_function_index];
                            uint8_t function_found = 0;
                            // try to find the implementation function
                            for(size_t loaded_function_index = 0; loaded_function_index < functions.size; loaded_function_index += 1) {
                                Instruction_Function* loaded_function = *((Instruction_Function**) vector_get(&functions, loaded_function_index));
                                if(contained_impl_function_name->length != loaded_function->name.length
                                || memcmp(loaded_function->name.data, contained_impl_function_name->data, contained_impl_function_name->length) != 0) { continue; }
                                // found!
                                trait_method_impl_pointers[contained_function_index] = loaded_function;

                                function_found = 1;
                                break;
                            }
                            if(function_found) { continue; }
                            // not found? -> error
                            char* contained_impl_function_name_null_terminated = as_null_terminated(contained_impl_function_name->data, contained_impl_function_name->length);
                            printf("No function with the name '%s' could be found in any loaded module.\n", contained_impl_function_name_null_terminated);
                            free(contained_impl_function_name_null_terminated);
                            exit(1);
                        }
                        trait_found = 1;
                        break; 
                    }
                    // not found? -> error
                    if(!trait_found) {
                        char* contained_trait_name_null_terminated = as_null_terminated(contained_trait_name->data, contained_trait_name->length);
                        printf("No trait with the name '%s' could be found in any loaded module.\n", contained_trait_name_null_terminated);
                        free(contained_trait_name_null_terminated);
                        exit(1);
                    }
                    // replace with resolved
                    *instruction = (Instruction) {
                        .type = RESOLVED_IMPLEMENTS,
                        .data = { .resolved_implements_data = {
                            .name = data->name,
                            .trait_count = data->trait_count,
                            .traits = contained_trait_pointers,
                            .trait_impl_functions = contained_method_impl_pointers
                        } }
                    };
                    Instruction_ResolvedImplements* resolved_implements_data = &instruction->data.resolved_implements_data;
                    vector_push(&implements, &resolved_implements_data);
                }
            } break;
            case METHOD_CALL: {
                Instruction_MethodCall* data = &instruction->data.method_call_data;
                // try to find the trait
                uint8_t trait_found = 0;
                for(size_t loaded_trait_index = 0; loaded_trait_index < traits.size; loaded_trait_index += 1) {
                    Instruction_Trait* loaded_trait = *((Instruction_Trait**) vector_get(&traits, loaded_trait_index));
                    if(data->trait_name.length != loaded_trait->name.length
                    || memcmp(loaded_trait->name.data, data->trait_name.data, data->trait_name.length) != 0) { continue; }
                    // found!
                    // try to find the index of the method
                    uint8_t method_found = 0;
                    for(uint16_t method_index = 0; method_index < loaded_trait->method_count; method_index += 1) {
                        MString* searched_method_name = &loaded_trait->method_names[method_index];
                        if(data->method_name.length != searched_method_name->length
                        || memcmp(searched_method_name->data, data->method_name.data, data->method_name.length) != 0) { continue; }
                        // found!
                        // replace with resolved
                        *instruction = (Instruction) {
                            .type = RESOLVED_METHOD_CALL,
                            .data = { .resolved_method_call_data = {
                                .value = data->value,
                                .trait_id = loaded_trait->trait_id,
                                .method_index = method_index,
                                .argv = data->argv,
                                .returned = data->returned
                            } }
                        };
                        method_found = 1;
                        break;
                    }
                    // not found? -> error
                    if(!method_found) {
                        char* trait_name_null_terminated = as_null_terminated(data->trait_name.data, data->trait_name.length);
                        char* method_name_null_terminated = as_null_terminated(data->method_name.data, data->method_name.length);
                        printf("The trait '%s' has no method '%s'.\n", trait_name_null_terminated, method_name_null_terminated);
                        free(trait_name_null_terminated);
                        free(method_name_null_terminated);
                        exit(1);
                    }
                    trait_found = 1;
                    break; 
                }
                if(trait_found) { continue; }
                // not found? -> error
                char* trait_name_null_terminated = as_null_terminated(data->trait_name.data, data->trait_name.length);
                printf("No trait with the name '%s' could be found in any loaded module.\n", trait_name_null_terminated);
                free(trait_name_null_terminated);
                exit(1);
            } break;

            default: {}
        }
    }
    // resolve implements usage
    for(InstrC instruction_index = 0; instruction_index < instruction_count; instruction_index += 1) {
        Instruction* instruction = &instructions[instruction_index];
        switch(instruction->type) {
            case ADD_IMPLEMENTS: {
                Instruction_AddImplements* data = &instruction->data.add_implements_data;
                // try to find the implements
                uint8_t found = 0;
                for(size_t loaded_implements_index = 0; loaded_implements_index < implements.size; loaded_implements_index += 1) {
                    Instruction_ResolvedImplements* loaded_implements = *((Instruction_ResolvedImplements**) vector_get(&implements, loaded_implements_index));
                    if(data->impl_name.length != loaded_implements->name.length
                    || memcmp(loaded_implements->name.data, data->impl_name.data, data->impl_name.length) != 0) { continue; }
                    // found!
                    // replace with resolved
                    *instruction = (Instruction) {
                        .type = RESOLVED_ADD_IMPLEMENTS,
                        .data = { .resolved_add_implements_data = {
                            .impl = loaded_implements,
                            .value = data->value
                        } }
                    };
                    found = 1;
                    break;
                }
                if(found) { continue; }
                // not found? -> error
                char* implements_name_null_terminated = as_null_terminated(data->impl_name.data, data->impl_name.length);
                printf("No implementations with the name '%s' could be found in any loaded module.\n", implements_name_null_terminated);
                free(implements_name_null_terminated);
                exit(1);
            } break;

            default: {}
        }
    }
    // clean up
    vector_cleanup(&functions);
    vector_cleanup(&traits);
    vector_cleanup(&implements);
}


void async_execute(void* args) {
    GC* gc = (GC*) ((void**) args)[0];
    ThreadPool* tp = (ThreadPool*) ((void**) args)[1];
    Instruction* instructions = (Instruction*) ((void**) args)[2];
    InstrC instruction_count = (InstrC) ((void**) args)[3];
    IoliteAllocation* base_frame = (IoliteAllocation*) ((void**) args)[4];
    IoliteValue* base_return = (IoliteValue*) ((void**) args)[5];
    InstrC current_index = (InstrC) ((void**) args)[6];
    execute(gc, tp, instructions, instruction_count, base_frame, base_return, current_index);
    free(args);
}

void execute(GC* gc, ThreadPool* tp, Instruction* instructions, InstrC instruction_count, IoliteAllocation* base_frame, IoliteValue* base_return, InstrC current_index) {
    Vector frames = create_vector(sizeof(IoliteAllocation*));
    vector_push(&frames, base_frame);
    Vector return_idx = create_vector(sizeof(InstrC));
    IoliteAllocation* current_frame = base_frame;
    VarIdx return_val_dest_var = 0;
    Instruction* i;
    while(current_index < instruction_count) {
        i = &instructions[current_index];
        switch(i->type) {
            case FUNCTION: /* nothing to do, functions have already been loaded */ break; 
            case CALL: /* call should be resolved, we shouldn't ever encounter this instruction */ break;
            case ASYNC_CALL: /* call should be resolved, we shouldn't ever encounter this instruction */ break;
            case EXTERNAL_CALL: /* call should be resolved, we shouldn't ever encounter this instruction */ break;
            case CLOSURE_CALL: {
                Instruction_ClosureCall* data = &i->data.closure_call_data;
                IoliteClosure* closure = &current_frame->values[data->called].value.closure;
                for(VarIdx arg_index = 0; arg_index < data->argc; arg_index += 1) {
                    IoliteValue* arg = &current_frame->values[data->argv[arg_index]];
                    IoliteValue* dest = &closure->frame->values[closure->args_offset + arg_index];
                    if(arg->type == REFERENCE) { arg->value.ref->stack_reference_count += 1; }
                    if(dest->type == REFERENCE) { dest->value.ref->stack_reference_count -= 1; }
                    *dest = *arg;
                }
                closure->frame->stack_reference_count += 1;
                vector_push(&frames, &closure->frame);
                InstrC next_index = current_index + 1;
                vector_push(&return_idx, &next_index);
                current_index = closure->instruction_index;
                return_val_dest_var = data->returned;
                current_frame = closure->frame;
                continue;
            } break;
            case RETURN: case RETURN_NOTHING: {
                vector_pop(&frames);
                IoliteAllocation* old_frame = frames.size > 0? *((IoliteAllocation**) vector_get(&frames, frames.size - 1)) : NULL;
                if(i->type == RETURN) {
                    Instruction_Return* data = &i->data.return_data;
                    IoliteValue* return_val = &current_frame->values[data->value];
                    IoliteValue* dest = &old_frame->values[return_val_dest_var];
                    if(old_frame == NULL) { dest = base_return; }
                    if(return_val->type == REFERENCE) { return_val->value.ref->stack_reference_count += 1; }
                    if(dest->type == REFERENCE) { dest->value.ref->stack_reference_count -= 1; }
                    *dest = *return_val;
                }
                for(VarIdx var = 0; var < current_frame->size; var += 1) {
                    IoliteValue* val = &current_frame->values[var];
                    if(val->type == REFERENCE) { val->value.ref->stack_reference_count -= 1; }
                }
                current_frame->stack_reference_count -= 1;
                if(old_frame == NULL) {
                    current_index = instruction_count;
                    continue;
                }
                current_frame = old_frame;
                InstrC* new_index = vector_get(&return_idx, return_idx.size - 1);
                current_index = *new_index;
                vector_pop(&return_idx);
                continue;
            } break;
            case ASSERT: {
                Instruction_Assert* data = &i->data.assert_data;
                IoliteValue* cond = &current_frame->values[data->value];
                uint8_t condition_met = 0;
                switch(cond->type) {
                    case NATURAL: condition_met = cond->value.natural != 0; break;
                    case INTEGER: condition_met = cond->value.integer != 0; break;
                    case FLOAT: condition_met = cond->value.flt != 0.0; break;
                    case REFERENCE: condition_met = cond->value.ref != NULL; break;
                    case UNIT: case CLOSURE: {}
                }
                if(!condition_met) {
                    printf("Function condition unmet.\n");
                    exit(1);
                }
            } break;
            case TRAIT: /* nothing to do, traits have already been loaded */ break;
            case IMPLEMENTS: /* implements should be resolved, we shouldn't ever encounter this instruction */ break;
            case ADD_IMPLEMENTS: /* implements add should be resolved, we shouldn't ever encounter this instruction */ break;
            case METHOD_CALL: /* method calls should be resolved, we shouldn't ever encounter this instruction */ break;
            
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

            case PUT_NAT: {
                Instruction_PutNat* data = &i->data.put_nat_data;
                IoliteValue* dest = &current_frame->values[data->dest];
                if(dest->type == REFERENCE) { dest->value.ref->stack_reference_count -= 1; }
                *dest = (IoliteValue) { .type = NATURAL, .value = { .natural = data->value } };
            } break;
            case PUT_INT: {
                Instruction_PutInt* data = &i->data.put_int_data;
                IoliteValue* dest = &current_frame->values[data->dest];
                if(dest->type == REFERENCE) { dest->value.ref->stack_reference_count -= 1; }
                *dest = (IoliteValue) { .type = INTEGER, .value = { .integer = data->value } };
            } break;
            case PUT_FLT: {
                Instruction_PutFlt* data = &i->data.put_flt_data;
                IoliteValue* dest = &current_frame->values[data->dest];
                if(dest->type == REFERENCE) { dest->value.ref->stack_reference_count -= 1; }
                *dest = (IoliteValue) { .type = FLOAT, .value = { .flt = data->value } };
            } break;
            case PUT_CLOSURE: {
                Instruction_PutClosure* data = &i->data.put_closure_data;
                IoliteValue* dest = &current_frame->values[data->dest];
                if(dest->type == REFERENCE) { dest->value.ref->stack_reference_count -= 1; }
                *dest = (IoliteValue) { .type = CLOSURE, .value = { .closure = {
                    .frame = current_frame,
                    .instruction_index = data->instruction_index,
                    .args_offset = data->args_offset
                } } };
            } break;

            case EQUALS: {
                Instruction_Equals* data = &i->data.equals_data;
                IoliteValue* a = &current_frame->values[data->a];
                IoliteValue* b = &current_frame->values[data->b];
                IoliteValue* result = &current_frame->values[data->dest];
                if(result->type == REFERENCE) { result->value.ref->stack_reference_count -= 1; }
                char result_value = 0;
                switch(a->type) {
                    case NATURAL: result_value = a->value.natural == b->value.natural; break;
                    case INTEGER: result_value = a->value.integer == b->value.integer; break;
                    case FLOAT: result_value = a->value.flt == b->value.flt; break;
                    case REFERENCE: result_value = a->value.ref == b->value.ref; break;
                    case UNIT: result_value = 1; break;
                }
                *result = (IoliteValue) { .type = NATURAL, .value = { .natural = result_value } };
            } break;
            case NOT_EQUALS: {
                Instruction_NotEquals* data = &i->data.not_equals_data;
                IoliteValue* a = &current_frame->values[data->a];
                IoliteValue* b = &current_frame->values[data->b];
                IoliteValue* result = &current_frame->values[data->dest];
                if(result->type == REFERENCE) { result->value.ref->stack_reference_count -= 1; }
                char result_value = 0;
                switch(a->type) {
                    case NATURAL: result_value = a->value.natural != b->value.natural; break;
                    case INTEGER: result_value = a->value.integer != b->value.integer; break;
                    case FLOAT: result_value = a->value.flt != b->value.flt; break;
                    case REFERENCE: result_value = a->value.ref != b->value.ref; break;
                    case UNIT: result_value = 0; break;
                }
                *result = (IoliteValue) { .type = NATURAL, .value = { .natural = result_value } };
            } break;
            case LESS_THAN: {
                Instruction_LessThan* data = &i->data.less_than_data;
                IoliteValue* a = &current_frame->values[data->a];
                IoliteValue* b = &current_frame->values[data->b];
                IoliteValue* result = &current_frame->values[data->dest];
                if(result->type == REFERENCE) { result->value.ref->stack_reference_count -= 1; }
                char result_value = 0;
                switch(a->type) {
                    case NATURAL: result_value = a->value.natural < b->value.natural; break;
                    case INTEGER: result_value = a->value.integer < b->value.integer; break;
                    case FLOAT: result_value = a->value.flt < b->value.flt; break;
                    default: {
                        printf("Cannot compare a non-number value!\n");
                        exit(1);
                    }
                }
                *result = (IoliteValue) { .type = NATURAL, .value = { .natural = result_value } };
            } break;
            case GREATER_THAN: {
                Instruction_GreaterThan* data = &i->data.greater_than_data;
                IoliteValue* a = &current_frame->values[data->a];
                IoliteValue* b = &current_frame->values[data->b];
                IoliteValue* result = &current_frame->values[data->dest];
                if(result->type == REFERENCE) { result->value.ref->stack_reference_count -= 1; }
                char result_value = 0;
                switch(a->type) {
                    case NATURAL: result_value = a->value.natural > b->value.natural; break;
                    case INTEGER: result_value = a->value.integer > b->value.integer; break;
                    case FLOAT: result_value = a->value.flt > b->value.flt; break;
                    default: {
                        printf("Cannot compare a non-number value!\n");
                        exit(1);
                    }
                }
                *result = (IoliteValue) { .type = NATURAL, .value = { .natural = result_value } };
            } break;
            case LESS_THAN_EQUALS: {
                Instruction_LessThanEquals* data = &i->data.less_than_equals_data;
                IoliteValue* a = &current_frame->values[data->a];
                IoliteValue* b = &current_frame->values[data->b];
                IoliteValue* result = &current_frame->values[data->dest];
                if(result->type == REFERENCE) { result->value.ref->stack_reference_count -= 1; }
                char result_value = 0;
                switch(a->type) {
                    case NATURAL: result_value = a->value.natural <= b->value.natural; break;
                    case INTEGER: result_value = a->value.integer <= b->value.integer; break;
                    case FLOAT: result_value = a->value.flt <= b->value.flt; break;
                    default: {
                        printf("Cannot compare a non-number value!\n");
                        exit(1);
                    }
                }
                *result = (IoliteValue) { .type = NATURAL, .value = { .natural = result_value } };
            } break;
            case GREATER_THAN_EQUALS: {
                Instruction_GreaterThanEquals* data = &i->data.greater_than_equals_data;
                IoliteValue* a = &current_frame->values[data->a];
                IoliteValue* b = &current_frame->values[data->b];
                IoliteValue* result = &current_frame->values[data->dest];
                if(result->type == REFERENCE) { result->value.ref->stack_reference_count -= 1; }
                char result_value = 0;
                switch(a->type) {
                    case NATURAL: result_value = a->value.natural >= b->value.natural; break;
                    case INTEGER: result_value = a->value.integer >= b->value.integer; break;
                    case FLOAT: result_value = a->value.flt >= b->value.flt; break;
                    default: {
                        printf("Cannot compare a non-number value!\n");
                        exit(1);
                    }
                }
                *result = (IoliteValue) { .type = NATURAL, .value = { .natural = result_value } };
            } break;
            case NOT: {
                Instruction_Not* data = &i->data.not_data;
                IoliteValue* x = &current_frame->values[data->x];
                IoliteValue* result = &current_frame->values[data->dest];
                if(result->type == REFERENCE) { result->value.ref->stack_reference_count -= 1; }
                *result = (IoliteValue) { .type = NATURAL, .value = { .natural = !x->value.natural } };
            } break;

            case ADD: {
                Instruction_Add* data = &i->data.add_data;
                IoliteValue* a = &current_frame->values[data->a];
                IoliteValue* b = &current_frame->values[data->b];
                IoliteValue* result = &current_frame->values[data->dest];
                if(result->type == REFERENCE) { result->value.ref->stack_reference_count -= 1; }
                switch(a->type) {
                    case NATURAL: *result = (IoliteValue) { .type = NATURAL, .value = { .natural = a->value.natural + b->value.natural } }; break;
                    case INTEGER: *result = (IoliteValue) { .type = INTEGER, .value = { .integer = a->value.integer + b->value.integer } }; break;
                    case FLOAT: *result = (IoliteValue) { .type = FLOAT, .value = { .flt = a->value.flt + b->value.flt } }; break;
                    default: {
                        printf("Cannot add non-number values!\n");
                        exit(1);
                    }
                }
            } break;
            case SUBTRACT: {
                Instruction_Subtract* data = &i->data.subtract_data;
                IoliteValue* a = &current_frame->values[data->a];
                IoliteValue* b = &current_frame->values[data->b];
                IoliteValue* result = &current_frame->values[data->dest];
                if(result->type == REFERENCE) { result->value.ref->stack_reference_count -= 1; }
                switch(a->type) {
                    case NATURAL: *result = (IoliteValue) { .type = NATURAL, .value = { .natural = a->value.natural - b->value.natural } }; break;
                    case INTEGER: *result = (IoliteValue) { .type = INTEGER, .value = { .integer = a->value.integer - b->value.integer } }; break;
                    case FLOAT: *result = (IoliteValue) { .type = FLOAT, .value = { .flt = a->value.flt - b->value.flt } }; break;
                    default: {
                        printf("Cannot subtract non-number values!\n");
                        exit(1);
                    }
                }
            } break;
            case MULTIPLY: {
                Instruction_Multiply* data = &i->data.multiply_data;
                IoliteValue* a = &current_frame->values[data->a];
                IoliteValue* b = &current_frame->values[data->b];
                IoliteValue* result = &current_frame->values[data->dest];
                if(result->type == REFERENCE) { result->value.ref->stack_reference_count -= 1; }
                switch(a->type) {
                    case NATURAL: *result = (IoliteValue) { .type = NATURAL, .value = { .natural = a->value.natural * b->value.natural } }; break;
                    case INTEGER: *result = (IoliteValue) { .type = INTEGER, .value = { .integer = a->value.integer * b->value.integer } }; break;
                    case FLOAT: *result = (IoliteValue) { .type = FLOAT, .value = { .flt = a->value.flt * b->value.flt } }; break;
                    default: {
                        printf("Cannot multiply non-number values!\n");
                        exit(1);
                    }
                }
            } break;
            case DIVIDE: {
                Instruction_Divide* data = &i->data.divide_data;
                IoliteValue* a = &current_frame->values[data->a];
                IoliteValue* b = &current_frame->values[data->b];
                IoliteValue* result = &current_frame->values[data->dest];
                if(result->type == REFERENCE) { result->value.ref->stack_reference_count -= 1; }
                switch(a->type) {
                    case NATURAL: *result = (IoliteValue) { .type = NATURAL, .value = { .natural = a->value.natural / b->value.natural } }; break;
                    case INTEGER: *result = (IoliteValue) { .type = INTEGER, .value = { .integer = a->value.integer / b->value.integer } }; break;
                    case FLOAT: *result = (IoliteValue) { .type = FLOAT, .value = { .flt = a->value.flt / b->value.flt } }; break;
                    default: {
                        printf("Cannot divide non-number values!\n");
                        exit(1);
                    }
                }
            } break;
            case MODULO: {
                Instruction_Modulo* data = &i->data.modulo_data;
                IoliteValue* a = &current_frame->values[data->a];
                IoliteValue* b = &current_frame->values[data->b];
                IoliteValue* result = &current_frame->values[data->dest];
                if(result->type == REFERENCE) { result->value.ref->stack_reference_count -= 1; }
                switch(a->type) {
                    case NATURAL: *result = (IoliteValue) { .type = NATURAL, .value = { .natural = a->value.natural % b->value.natural } }; break;
                    case INTEGER: *result = (IoliteValue) { .type = INTEGER, .value = { .integer = a->value.integer % b->value.integer } }; break;
                    case FLOAT: *result = (IoliteValue) { .type = FLOAT, .value = { .flt = fmod(a->value.flt, b->value.flt) } }; break;
                    default: {
                        printf("Cannot perfrom modulo on non-number values!\n");
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
                    case NATURAL: *result = (IoliteValue) { .type = NATURAL, .value = { .natural = -(x->value.natural) } }; break;
                    case INTEGER: *result = (IoliteValue) { .type = INTEGER, .value = { .integer = -(x->value.integer) } }; break;
                    case FLOAT: *result = (IoliteValue) { .type = FLOAT, .value = { .flt = -(x->value.flt) } }; break;
                    default: {
                        printf("Cannot negate non-number values!\n");
                        exit(1);
                    }
                }
            } break;

            case CONVERT_TO_NAT: {
                Instruction_ConvertToNat* data = &i->data.convert_to_nat_data;
                IoliteValue* x = &current_frame->values[data->x];
                uint64_t value;
                switch(x->type) {
                    case NATURAL: value = x->value.natural; break;
                    case INTEGER: value = (uint64_t) x->value.integer; break;
                    case FLOAT: value = (uint64_t) x->value.flt; break;
                    default: {
                        printf("Cannot convert non-numbers to a natural number!\n");
                        exit(1);
                    }
                }
                IoliteValue* dest = &current_frame->values[data->dest];
                if(dest->type == REFERENCE) { dest->value.ref->stack_reference_count -= 1; }
                *dest = (IoliteValue) { .type = NATURAL, .value = { .natural = value } };
            } break;
            case CONVERT_TO_INT: {
                Instruction_ConvertToInt* data = &i->data.convert_to_int_data;
                IoliteValue* x = &current_frame->values[data->x];
                int64_t value;
                switch(x->type) {
                    case NATURAL: value = (int64_t) x->value.natural; break;
                    case INTEGER: value = x->value.integer; break;
                    case FLOAT: value = (int64_t) x->value.flt; break;
                    default: {
                        printf("Cannot convert non-numbers to an integer!\n");
                        exit(1);
                    }
                }
                IoliteValue* dest = &current_frame->values[data->dest];
                if(dest->type == REFERENCE) { dest->value.ref->stack_reference_count -= 1; }
                *dest = (IoliteValue) { .type = INTEGER, .value = { .integer = value } };
            } break;
            case CONVERT_TO_FLT: {
                Instruction_ConvertToFlt* data = &i->data.convert_to_flt_data;
                IoliteValue* x = &current_frame->values[data->x];
                double value;
                switch(x->type) {
                    case NATURAL: value = (double) x->value.natural; break;
                    case INTEGER: value = (double) x->value.integer; break;
                    case FLOAT: value = x->value.flt; break;
                    default: {
                        printf("Cannot convert non-numbers to a floating point number!\n");
                        exit(1);
                    }
                }
                IoliteValue* dest = &current_frame->values[data->dest];
                if(dest->type == REFERENCE) { dest->value.ref->stack_reference_count -= 1; }
                *dest = (IoliteValue) { .type = FLOAT, .value = { .flt = value } };
            } break;

            case MALLOC_DYNAMIC: case MALLOC_FIXED: {
                uint64_t size;
                VarIdx dest;
                if(i->type == MALLOC_DYNAMIC) {
                    Instruction_MallocDynamic* data = &i->data.malloc_dynamic_data;
                    size = current_frame->values[data->size].value.natural;
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
                    index = current_frame->values[data->index].value.natural;
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
                    index = current_frame->values[data->index].value.natural;
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

            case RESOLVED_CALL: {
                Instruction_ResolvedCall* data = &i->data.resolved_call_data;
                IoliteAllocation* call_frame = gc_allocate(gc, data->function->argc + data->function->varc);
                for(VarIdx var_index = 0; var_index < call_frame->size; var_index += 1) {
                    call_frame->values[var_index].type = UNIT;
                }
                for(VarIdx arg_index = 0; arg_index < data->function->argc; arg_index += 1) {
                    IoliteValue* arg = &current_frame->values[data->argv[arg_index]];
                    if(arg->type == REFERENCE) { arg->value.ref->stack_reference_count += 1; }
                    call_frame->values[arg_index] = *arg;
                }
                vector_push(&frames, &call_frame);
                InstrC next_index = current_index + 1;
                vector_push(&return_idx, &next_index);
                current_index = data->function->body_instruction_index;
                return_val_dest_var = data->returned;
                current_frame = call_frame;
                continue;
            } break;
            case RESOLVED_ASYNC_CALL: {
                Instruction_ResolvedAsyncCall* data = &i->data.resolved_async_call_data;
                IoliteAllocation* call_frame = gc_allocate(gc, data->function->argc + data->function->varc);
                for(VarIdx var_index = 0; var_index < call_frame->size; var_index += 1) {
                    call_frame->values[var_index].type = UNIT;
                }
                for(VarIdx arg_index = 0; arg_index < data->function->argc; arg_index += 1) {
                    IoliteValue* arg = &current_frame->values[data->argv[arg_index]];
                    if(arg->type == REFERENCE) { arg->value.ref->stack_reference_count += 1; }
                    call_frame->values[arg_index] = *arg;
                }
                IoliteValue* dest = &current_frame->values[data->returned];
                if(dest->type == REFERENCE) { dest->value.ref->stack_reference_count -= 1; }
                IoliteAllocation* return_value_holder = gc_allocate(gc, 2);
                *dest = (IoliteValue) { .type = REFERENCE, .value = { .ref = return_value_holder } };
                return_value_holder->values[0] = (IoliteValue) { .type = REFERENCE, .value = { .ref = NULL } };
                void** args = malloc(sizeof(void*) * 7);
                args[0] = gc;
                args[1] = tp;
                args[2] = instructions;
                args[3] = (void*) instruction_count;
                args[4] = call_frame;
                args[5] = &return_value_holder->values[0];
                args[6] = (void*) data->function->body_instruction_index;
                return_value_holder->values[1] = (IoliteValue) { .type = NATURAL, .value = {
                    .natural = threadpool_do(tp, &async_execute, args)
                } };
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
            case RESOLVED_IMPLEMENTS: /* nothing to do, implements have already been loaded */ break;
            case RESOLVED_ADD_IMPLEMENTS: {
                Instruction_ResolvedAddImplements* data = &i->data.resolved_add_implements_data;
                IoliteValue* value = &current_frame->values[data->value];
                value->methods = data->impl;
            } break;
            case RESOLVED_METHOD_CALL: {
                Instruction_ResolvedMethodCall* data = &i->data.resolved_method_call_data;
                IoliteValue* value = &current_frame->values[data->value];
                Instruction_ResolvedImplements* methods = (Instruction_ResolvedImplements*) value->methods;
                for(uint16_t trait_index = 0; trait_index < methods->trait_count; trait_index += 1) {
                    Instruction_Trait* trait = methods->traits[trait_index];
                    if(trait->trait_id != data->trait_id) { continue; }
                    Instruction_Function* method_impl = methods->trait_impl_functions[trait_index][data->method_index];

                    IoliteAllocation* call_frame = gc_allocate(gc, method_impl->argc + method_impl->varc);
                    for(VarIdx var_index = 0; var_index < call_frame->size; var_index += 1) {
                        call_frame->values[var_index].type = UNIT;
                    }
                    for(VarIdx arg_index = 0; arg_index < method_impl->argc; arg_index += 1) {
                        IoliteValue* arg = &current_frame->values[data->argv[arg_index]];
                        if(arg->type == REFERENCE) { arg->value.ref->stack_reference_count += 1; }
                        call_frame->values[arg_index] = *arg;
                    }
                    vector_push(&frames, &call_frame);
                    InstrC next_index = current_index + 1;
                    vector_push(&return_idx, &next_index);
                    current_index = method_impl->body_instruction_index;
                    return_val_dest_var = data->returned;
                    current_frame = call_frame;

                    break;
                }
                continue;
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
                    case NATURAL: cond_true = cond->value.natural != 0; break;
                    case INTEGER: cond_true = cond->value.integer != 0; break;
                    case FLOAT: cond_true = cond->value.flt != 0; break;
                    case REFERENCE: cond_true = cond->value.ref != NULL; break;
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

    vector_cleanup(&frames);
    vector_cleanup(&return_idx);
}