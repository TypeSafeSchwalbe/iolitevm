
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cli.h"


void error(char* reason) {
    printf("\nError! %s\n\n", reason);
    exit(1);
}

void invalid_args() {
    error("The provided arguments are incorrect!\n\n-mods <module-file> <module-file> ...\n    specifies what module files (.iob) to load\n-slibs <shared-lib> <shared-lib> ...\n    specified what shared libraries (.so / .dll) to load\n-start <main-function>\n    specifies the entry point of the program (shall not have any parameters or return values)");
}

Vector parse_cli_args(int argc, char** argv) {
    Vector pargs = create_vector(sizeof(CliArg));
    int argi = 1;
    CliArg* carg = NULL;
    if(argc <= 1) { invalid_args(); }
    while(argi < argc) {
        if(argi == 1 && argv[argi][0] != '-') { invalid_args(); }
        if(argv[argi][0] == '-') {
            CliArg new_arg;
            vector_push(&pargs, &new_arg);
            carg = vector_get(&pargs, pargs.size - 1);
            carg->name = argv[argi] + 1;
            carg->values = create_vector(sizeof(char*));
        } else {
            vector_push(&carg->values, &argv[argi]);
        }
        argi += 1;
    }
    return pargs;
}


void display_instruction(Instruction* i) {
    switch(i->type) {
        case FUNCTION: printf("function ..."); break;
        case CALL: printf("call ..."); break;
        case ASYNC_CALL: printf("async-call ..."); break;
        case EXTERNAL_CALL: printf("external-call ..."); break;
        case CLOSURE_CALL: {
            Instruction_ClosureCall* data = &i->data.closure_call_data;
            printf("closure-call called=[%u] argc=%u argv={", data->called, data->argc);
            for(VarIdx arg = 0; arg < data->argc; arg += 1) {
                if(arg > 0) { printf(", "); }
                printf("[%u]", data->argv[arg]);
            }
            printf("} returned=[%u]", data->returned);
        } break;
        case RETURN: {
            Instruction_Return* data = &i->data.return_data;
            printf("return value=[%u]", data->value);
        } break;
        case RETURN_NOTHING: {
            printf("return-nothing");
        } break;
        case ASSERT: {
            Instruction_Assert* data = &i->data.assert_data;
            printf("assert value=[%u]", data->value);
        } break;
        case TRAIT: printf("trait ..."); break;
        case IMPLEMENTS: printf("implements ..."); break;
        case ADD_IMPLEMENTS: printf("add-implements ..."); break;
        case METHOD_CALL: printf("method-call ..."); break;
        case IF: printf("if ..."); break;
        case LOOP: printf("loop ..."); break;
        case BREAK: printf("break ..."); break;
        case CONTINUE: printf("continue ..."); break;
        case COPY: {
            Instruction_Copy* data = &i->data.copy_data;
            printf("copy src=[%u] dest=[%u]", data->src, data->dest);
        } break;
        case PUT_NAT: {
            Instruction_PutNat* data = &i->data.put_nat_data;
            printf("put-nat value=%u dest=[%u]", data->value, data->dest);
        } break;
        case PUT_INT: {
            Instruction_PutInt* data = &i->data.put_int_data;
            printf("put-int value=%d dest=[%u]", data->value, data->dest);
        } break;
        case PUT_FLT: {
            Instruction_PutFlt* data = &i->data.put_flt_data;
            printf("put-float value=%f dest=[%u]", data->value, data->dest);
        } break;
        case PUT_CLOSURE: {
            Instruction_PutClosure* data = &i->data.put_closure_data;
            printf("put-closure args_offset=[%u] body=... dest=[%u]", data->args_offset, data->dest);
        } break;
        case EQUALS: {
            Instruction_Equals* data = &i->data.equals_data;
            printf("equals a=[%u] b=[%u] dest=[%u]", data->a, data->b, data->dest);
        } break;
        case NOT_EQUALS: {
            Instruction_NotEquals* data = &i->data.not_equals_data;
            printf("not-equals a=[%u] b=[%u] dest=[%u]", data->a, data->b, data->dest);
        } break;
        case LESS_THAN: {
            Instruction_LessThan* data = &i->data.less_than_data;
            printf("less-than a=[%u] b=[%u] dest=[%u]", data->a, data->b, data->dest);
        } break;
        case GREATER_THAN: {
            Instruction_GreaterThan* data = &i->data.greater_than_data;
            printf("greater-than a=[%u] b=[%u] dest=[%u]", data->a, data->b, data->dest);
        } break;
        case LESS_THAN_EQUALS: {
            Instruction_LessThanEquals* data = &i->data.less_than_equals_data;
            printf("less-than-equals a=[%u] b=[%u] dest=[%u]", data->a, data->b, data->dest);
        } break;
        case GREATER_THAN_EQUALS: {
            Instruction_GreaterThanEquals* data = &i->data.greater_than_equals_data;
            printf("greater-than-equals a=[%u] b=[%u] dest=[%u]", data->a, data->b, data->dest);
        } break;
        case NOT: {
            Instruction_Not* data = &i->data.not_data;
            printf("not x=[%u] dest=[%u]", data->x, data->dest);
        } break;
        case ADD: {
            Instruction_Add* data = &i->data.add_data;
            printf("add a=[%u] b=[%u] dest=[%u]", data->a, data->b, data->dest);
        } break;
        case SUBTRACT: {
            Instruction_Subtract* data = &i->data.subtract_data;
            printf("subtract a=[%u] b=[%u] dest=[%u]", data->a, data->b, data->dest);
        } break;
        case MULTIPLY: {
            Instruction_Multiply* data = &i->data.multiply_data;
            printf("multiply a=[%u] b=[%u] dest=[%u]", data->a, data->b, data->dest);
        } break;
        case DIVIDE: {
            Instruction_Divide* data = &i->data.divide_data;
            printf("divide a=[%u] b=[%u] dest=[%u]", data->a, data->b, data->dest);
        } break;
        case MODULO: {
            Instruction_Modulo* data = &i->data.modulo_data;
            printf("modulo a=[%u] b=[%u] dest=[%u]", data->a, data->b, data->dest);
        } break;
        case NEGATE: {
            Instruction_Negate* data = &i->data.negate_data;
            printf("negate x=[%u] dest=[%u]", data->x, data->dest);
        } break;
        case CONVERT_TO_NAT: {
            Instruction_ConvertToNat* data = &i->data.convert_to_nat_data;
            printf("convert-to-nat x=[%u] dest=[%u]", data->x, data->dest);
        } break;
        case CONVERT_TO_INT: {
            Instruction_ConvertToInt* data = &i->data.convert_to_int_data;
            printf("convert-to-int x=[%u] dest=[%u]", data->x, data->dest);
        } break;
        case CONVERT_TO_FLT: {
            Instruction_ConvertToFlt* data = &i->data.convert_to_flt_data;
            printf("convert-to-float x=[%u] dest=[%u]", data->x, data->dest);
        } break;
        case MALLOC_DYNAMIC: {
            Instruction_MallocDynamic* data = &i->data.malloc_dynamic_data;
            printf("malloc-dynamic size=[%u] dest=[%u]", data->size, data->dest);
        } break;
        case MALLOC_FIXED: {
            Instruction_MallocFixed* data = &i->data.malloc_fixed_data;
            printf("malloc-dynamic size=%u dest=[%u]", data->size, data->dest);
        } break;
        case REF_GET_DYNAMIC: {
            Instruction_RefGetDynamic* data = &i->data.ref_get_dynamic_data;
            printf("ref-get-dynamic ref=[%u] index=[%u] dest=[%u]", data->ref, data->index, data->dest);
        } break;
        case REF_GET_FIXED: {
            Instruction_RefGetFixed* data = &i->data.ref_get_fixed_data;
            printf("ref-get-fixed ref=[%u] index=%u dest=[%u]", data->ref, data->index, data->dest);
        } break;
        case REF_SET_DYNAMIC: {
            Instruction_RefSetDynamic* data = &i->data.ref_set_dynamic_data;
            printf("ref-set-dynamic ref=[%u] index=[%u] value=[%u]", data->ref, data->index, data->value);
        } break;
        case REF_SET_FIXED: {
            Instruction_RefSetFixed* data = &i->data.ref_set_fixed_data;
            printf("ref-set-dynamic ref=[%u] index=%u value=[%u]", data->ref, data->index, data->value);
        } break;
    
        case RESOLVED_CALL: {
            Instruction_ResolvedCall* data = &i->data.resolved_call_data;
            char name_null_terminated[data->function->name.length + 1];
            memcpy(name_null_terminated, data->function->name.data, data->function->name.length);
            name_null_terminated[data->function->name.length] = '\0';
            printf("call name=\"%s\" argc=%u, argv={", name_null_terminated, data->function->argc);
            for(VarIdx arg = 0; arg < data->function->argc; arg += 1) {
                if(arg > 0) { printf(", "); }
                printf("[%u]", data->argv[arg]);
            }
            printf("} returned=[%u]", data->returned);
        } break;
        case RESOLVED_ASYNC_CALL: {
            Instruction_ResolvedAsyncCall* data = &i->data.resolved_async_call_data;
            char name_null_terminated[data->function->name.length + 1];
            memcpy(name_null_terminated, data->function->name.data, data->function->name.length);
            name_null_terminated[data->function->name.length] = '\0';
            printf("async-call name=\"%s\" argc=%u, argv={", name_null_terminated, data->function->argc);
            for(VarIdx arg = 0; arg < data->function->argc; arg += 1) {
                if(arg > 0) { printf(", "); }
                printf("[%u]", data->argv[arg]);
            }
            printf("} returned=[%u]", data->returned);
        } break;
        case RESOLVED_EXTERNAL_CALL: {
            Instruction_ResolvedExternalCall* data = &i->data.resolved_external_call_data;
            char name_null_terminated[data->name.length + 1];
            memcpy(name_null_terminated, data->name.data, data->name.length);
            name_null_terminated[data->name.length] = '\0';
            printf("call name=\"%s\" argc=%u, argv={", name_null_terminated, data->argc);
            for(VarIdx arg = 0; arg < data->argc; arg += 1) {
                if(arg > 0) { printf(", "); }
                printf("[%u]", data->argv[arg]);
            }
            printf("} returned=[%u]", data->returned);
        } break;
        case RESOLVED_IMPLEMENTS: printf("implements ..."); break;
        case RESOLVED_ADD_IMPLEMENTS: {
            Instruction_ResolvedAddImplements* data = &i->data.resolved_add_implements_data;
            char impl_name_null_terminated[data->impl->name.length + 1];
            memcpy(impl_name_null_terminated, data->impl->name.data, data->impl->name.length);
            impl_name_null_terminated[data->impl->name.length] = '\0';
            printf("add-implements value=[%u] name=\"%s\"", data->value, impl_name_null_terminated);
        } break;
        case RESOLVED_METHOD_CALL: {
            Instruction_ResolvedMethodCall* data = &i->data.resolved_method_call_data;
            char trait_name_null_terminated[data->trait_name.length + 1];
            memcpy(trait_name_null_terminated, data->trait_name.data, data->trait_name.length);
            trait_name_null_terminated[data->trait_name.length] = '\0';
            char method_name_null_terminated[data->method_name.length + 1];
            memcpy(method_name_null_terminated, data->method_name.data, data->method_name.length);
            method_name_null_terminated[data->method_name.length] = '\0';
            printf("method-call value=[%u] trait_name=\"%s\" method_name=\"%s\" argc=%u argv={", data->value, trait_name_null_terminated, method_name_null_terminated, data->argc);
            for(VarIdx arg = 0; arg < data->argc; arg += 1) {
                if(arg > 0) { printf(", "); }
                printf("[%u]", data->argv[arg]);
            }
            printf("} returned=[%u]", data->returned);
        } break;
        case JUMP: {
            Instruction_Jump* data = &i->data.jump_data;
            printf("jump dest=[0x%08X]", data->dest);
        } break;
        case CONDITIONAL_JUMP: {
            Instruction_ConditionalJump* data = &i->data.conditional_jump_data;
            printf("conditional-jump condition=[%u] if_dest=[0x%08X] else_dest=[0x%08X]", data->condition, data->if_dest, data->else_dest);
        } break;
    }
}

void display_value(IoliteValue* v) {
    switch(v->type) {
        case NATURAL: printf("<nat> %u", v->value.natural); break;
        case INTEGER: printf("<int> %d", v->value.integer); break;
        case FLOAT: printf("<float> %f", v->value.flt); break;
        case REFERENCE: printf("<ref> %p", v->value.ref); break;
        case CLOSURE: printf("<closure> 0x%08X", v->value.closure.instruction_index); break;
        case UNIT: printf("<unit>"); break;
    }
}

void collect_references(IoliteAllocation* r, Vector* refs) {
    for(size_t refi = 0; refi < refs->size; refi += 1) {
        IoliteAllocation** ref = vector_get(refs, refi);
        if(*ref == r) { return; }
    }
    vector_push(refs, &r);
    for(uint64_t vali = 0; vali < r->size; vali += 1) {
        IoliteValue* val = &r->values[vali];
        if(val->type == REFERENCE) {
            collect_references(val->value.ref, refs);
        }
    }
}

void break_down(GC* gc, Vector* call_history, Vector* frames, Instruction* instructions, InstrC current_instruction, char* reason) {
    printf("\nError! The VM broke down.\n\n");
    // Reason
    printf("Reason\n    %s\n\n", reason);
    // Instruction execution history
    printf("Executed\n");
    CallInfo* last_call = vector_get(call_history, call_history->size - 1);
    for(InstrC instri = last_call->index; instri < current_instruction + 1; instri += 1) {
        if(instri == current_instruction) { printf(" -> "); }
        else { printf("    "); }
        printf("[0x%08X] ", instri);
        display_instruction(&instructions[instri]);
        if(instri == current_instruction) { printf(" <-\n"); }
        else { printf("\n"); }
    }
    printf("\n");
    // Stack
    printf("Stack\n");
    for(size_t stacki = 0; stacki < call_history->size; stacki += 1) {
        CallInfo* call = vector_get(call_history, stacki);
        char call_name_null_terminated[call->name->length + 1];
        memcpy(call_name_null_terminated, call->name->data, call->name->length);
        call_name_null_terminated[call->name->length] = '\0';
        printf("    [%u] %s\n", stacki, call_name_null_terminated);
    }
    IoliteAllocation* current_frame = *((IoliteAllocation**) vector_get(frames, frames->size - 1));
    for(size_t vari = 0; vari < current_frame->size; vari += 1) {
        IoliteValue* var = &current_frame->values[vari];
        if(var->type == UNIT) { continue; }
        printf("        [%u] ", vari);
        display_value(var);
        printf("\n");
    }
    printf("\n");
    // Heap
    printf("Heap\n");
    Vector reachable = create_vector(sizeof(IoliteAllocation*));
    for(size_t vari = 0; vari < current_frame->size; vari += 1) {
        IoliteValue* var = &current_frame->values[vari];
        if(var->type == REFERENCE) {
            collect_references(var->value.ref, &reachable);
        }
    }
    uint8_t displayed = 0;
    for(size_t alloci = 0; alloci < gc->allocations.size; alloci += 1) {
        IoliteAllocation* alloc = vector_get(&gc->allocations, alloci);
        uint8_t reachable_contains_ref = 0;
        for(size_t refi = 0; refi < reachable.size; refi += 1) {
            IoliteAllocation** ref = vector_get(&reachable, refi);
            if(*ref == alloc) {
                reachable_contains_ref = 1;
                break;
            }
        }
        if(!reachable_contains_ref) { continue; }
        printf("    [%u] ptr=%p size=%u\n", alloci, alloc, alloc->size);
        for(uint64_t vali = 0; vali < alloc->size; vali += 1) {
            IoliteValue* val = &alloc->values[vali];
            if(val->type == UNIT) { continue; }
            printf("        [%u] ", vali);
            display_value(val);
            printf("\n");
        }
        displayed = 1;
    }
    if(!displayed) { printf("    ...\n"); }
    vector_cleanup(&reachable);
    printf("\n");
    
    exit(1);
}

/*

Error! The VM broke down.

Reason
    I hate addition. All my homies use subtractation and negation.

Executed
    [0x00000005] put_float value=3.14 dest=[0]
    [0x00000006] put_float value=3.14 dest=[1]
 -> [0x00000007] add a=[0] b=[1] dest=[2] <-

Stack
    [0] main
    [1] add
        [0] <var value>
        [1] <var value>
    ...

Heap
    [0] ptr=0xFFFFFFFF size=1
        [0] <var value>
        [1] <var value>
    ...

*/