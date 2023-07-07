
#include <string.h>
#include "runtime.h"


Runtime create_runtime() {
    Runtime r;
    r.frames = create_vector(sizeof(char*));
    r.return_size = 0;
    r.return_var = 0;
    return r;
}

void execute_instructions(Runtime* r, Module* m, char* instr, size_t count) {
    char* end = instr + count;
    while(instr < end) {
        char opcode = *instr;
        instr += 1;
        switch(opcode) {
            case INSTR_MALLOC: { NOT_IMPLEMENTED } break;
            case INSTR_MEM_SET: { NOT_IMPLEMENTED } break;
            case INSTR_MEM_GET: { NOT_IMPLEMENTED } break;
            case INSTR_ENTER: {
                uint16_t frame_size_bytes = *((uint16_t*) instr);
                char* allocation = malloc(frame_size_bytes);
                vector_push(&r->frames, &allocation);
                instr += 2;
            } break;
            case INSTR_EXIT: {
                char* current = *((char**) vector_get(&r->frames, r->frames.size - 1));
                free(current);
                vector_pop(&r->frames);
            } break;
            case INSTR_COPY: {
                char* src = *((char**) vector_get(&r->frames, r->frames.size - (1 + *((uint8_t*) instr))));
                instr += 1;
                src += *((uint16_t*) instr);
                instr += 2;
                char* dest = *((char**) vector_get(&r->frames, r->frames.size - (1 + *((uint8_t*) instr))));
                instr += 1;
                dest += *((uint16_t*) instr);
                instr += 2;
                uint8_t data_size = *((uint8_t*) instr);
                instr += 1;
                memcpy(dest, src, data_size);
            } break;
            case INSTR_FUNCTION: {
                Function f;
                f.return_type_size = *((uint8_t*) instr);
                instr += 1;
                f.body_size_bytes = *((uint16_t*) instr);
                instr += 2;
                f.body_ptr = instr;
                instr += f.body_size_bytes;
                vector_push(&m->functions, &f);
            } break;
            case INSTR_CALL: {
                // keep track of this frame's return info
                uint8_t pre_return_size = r->return_size;
                uint16_t pre_return_var = r->return_var;
                // get the new return info
                Function* function = vector_get(&m->functions, *((uint32_t*) instr));
                r->return_size = function->return_type_size;
                instr += 4;
                r->return_var = *((uint16_t*) instr);
                instr += 2;
                // execute the body
                execute_instructions(r, m, function->body_ptr, function->body_size_bytes);
                // overwrite with the old return info
                r->return_size = pre_return_size;
                r->return_var = pre_return_var;
            } break;
            case INSTR_RETURN: {
                uint16_t returned_val_var = *((uint16_t*) instr);
                instr += 2;
                char* current = *((char**) vector_get(&r->frames, r->frames.size - 1));
                vector_pop(&r->frames);
                char* pre_call = *((char**) vector_get(&r->frames, r->frames.size - 1));
                if(r->return_size > 0) {
                    memcpy(pre_call + r->return_var, current + returned_val_var, r->return_size);
                }
                free(current);
            } break;
            case INSTR_IF: { NOT_IMPLEMENTED } break;
            case INSTR_LOOP: { NOT_IMPLEMENTED } break;
            case INSTR_BREAK: { NOT_IMPLEMENTED } break;
            case INSTR_CONTINUE: { NOT_IMPLEMENTED } break;
            case INSTR_PUT_UINT: {
                uint8_t val_size = *((uint8_t*) instr);
                instr += 1;
                char* current = *((char**) vector_get(&r->frames, r->frames.size - 1));
                memcpy(current + *((uint16_t*) (instr + val_size)), instr, val_size);
                instr += val_size;
                instr += 2;
            } break;
            case INSTR_ADD_UINT: { NOT_IMPLEMENTED } break;
            case INSTR_SUBTRACT_UINT: { NOT_IMPLEMENTED } break;
            case INSTR_MULTIPLY_UINT: { NOT_IMPLEMENTED } break;
            case INSTR_DIVIDE_UINT: { NOT_IMPLEMENTED } break;
            case INSTR_MODULO_UINT: { NOT_IMPLEMENTED } break;
            case INSTR_PUT_SINT: {
                uint8_t val_size = *((uint8_t*) instr);
                instr += 1;
                char* current = *((char**) vector_get(&r->frames, r->frames.size - 1));
                memcpy(current + *((uint16_t*) (instr + val_size)), instr, val_size);
                instr += val_size;
                instr += 2;
            } break;
            case INSTR_ADD_SINT: { NOT_IMPLEMENTED } break;
            case INSTR_SUBTRACT_SINT: {
                uint8_t val_size = *((uint8_t*) instr);
                instr += 1;
                char* current = *((char**) vector_get(&r->frames, r->frames.size - 1));
                int64_t a_val = 0;
                memcpy(&a_val, current + *((uint16_t*) instr), val_size);
                instr += 2;
                int64_t b_val = 0;
                memcpy(&b_val, current + *((uint16_t*) instr), val_size);
                instr += 2;
                int64_t r_val = a_val - b_val;
                memcpy(current + *((uint16_t*) instr), &r_val, val_size);
                instr += 2;
            } break;
            case INSTR_MULTIPLY_SINT: { NOT_IMPLEMENTED } break;
            case INSTR_DIVIDE_SINT: { NOT_IMPLEMENTED } break;
            case INSTR_MODULO_SINT: { NOT_IMPLEMENTED } break;
            case INSTR_PUT_F32: { NOT_IMPLEMENTED } break;
            case INSTR_ADD_F32: { NOT_IMPLEMENTED } break;
            case INSTR_SUBTRACT_F32: { NOT_IMPLEMENTED } break;
            case INSTR_MULTIPLY_F32: { NOT_IMPLEMENTED } break;
            case INSTR_DIVIDE_F32: { NOT_IMPLEMENTED } break;
            case INSTR_MODULO_F32: { NOT_IMPLEMENTED } break;
            case INSTR_PUT_F64: { NOT_IMPLEMENTED } break;
            case INSTR_ADD_F64: { NOT_IMPLEMENTED } break;
            case INSTR_SUBTRACT_F64: { NOT_IMPLEMENTED } break;
            case INSTR_MULTIPLY_F64: { NOT_IMPLEMENTED } break;
            case INSTR_DIVIDE_F64: { NOT_IMPLEMENTED } break;
            case INSTR_MODULO_F64: { NOT_IMPLEMENTED } break;
            default: {
                printf("Byte 0x%02x in module '%s' is not a valid opcode!\n", opcode, m->file);
                exit(1);
            }
        }
    }
}
