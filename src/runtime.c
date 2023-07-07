
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
            case MALLOC: { NOT_IMPLEMENTED } break;
            case MEM_SET: { NOT_IMPLEMENTED } break;
            case MEM_GET: { NOT_IMPLEMENTED } break;
            case ENTER: {
                uint16_t frame_size_bytes = *((uint16_t*) instr);
                char* allocation = malloc(frame_size_bytes);
                vector_push(&r->frames, &allocation);
                instr += 2;
            } break;
            case EXIT: {
                char* current = *((char**) vector_get(&r->frames, r->frames.size - 1));
                free(current);
                vector_pop(&r->frames);
            } break;
            case COPY: {
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
            case FUNCTION: {
                Function f;
                f.return_type_size = *((uint8_t*) instr);
                instr += 1;
                f.body_size_bytes = *((uint16_t*) instr);
                instr += 2;
                f.body_ptr = instr;
                instr += f.body_size_bytes;
                vector_push(&m->functions, &f);
            } break;
            case CALL: {
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
            case RETURN: {
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
            case IF: { NOT_IMPLEMENTED } break;
            case LOOP: { NOT_IMPLEMENTED } break;
            case BREAK: { NOT_IMPLEMENTED } break;
            case CONTINUE: { NOT_IMPLEMENTED } break;
            case PUT_UINT: {
                uint8_t val_size = *((uint8_t*) instr);
                instr += 1;
                char* current = *((char**) vector_get(&r->frames, r->frames.size - 1));
                memcpy(current + *((uint16_t*) (instr + val_size)), instr, val_size);
                instr += val_size;
                instr += 2;
            } break;
            case ADD_UINT: { NOT_IMPLEMENTED } break;
            case SUBTRACT_UINT: { NOT_IMPLEMENTED } break;
            case MULTIPLY_UINT: { NOT_IMPLEMENTED } break;
            case DIVIDE_UINT: { NOT_IMPLEMENTED } break;
            case MODULO_UINT: { NOT_IMPLEMENTED } break;
            case PUT_SINT: {
                uint8_t val_size = *((uint8_t*) instr);
                instr += 1;
                char* current = *((char**) vector_get(&r->frames, r->frames.size - 1));
                memcpy(current + *((uint16_t*) (instr + val_size)), instr, val_size);
                instr += val_size;
                instr += 2;
            } break;
            case ADD_SINT: { NOT_IMPLEMENTED } break;
            case SUBTRACT_SINT: {
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
            case MULTIPLY_SINT: { NOT_IMPLEMENTED } break;
            case DIVIDE_SINT: { NOT_IMPLEMENTED } break;
            case MODULO_SINT: { NOT_IMPLEMENTED } break;
            case PUT_F32: { NOT_IMPLEMENTED } break;
            case ADD_F32: { NOT_IMPLEMENTED } break;
            case SUBTRACT_F32: { NOT_IMPLEMENTED } break;
            case MULTIPLY_F32: { NOT_IMPLEMENTED } break;
            case DIVIDE_F32: { NOT_IMPLEMENTED } break;
            case MODULO_F32: { NOT_IMPLEMENTED } break;
            case PUT_F64: { NOT_IMPLEMENTED } break;
            case ADD_F64: { NOT_IMPLEMENTED } break;
            case SUBTRACT_F64: { NOT_IMPLEMENTED } break;
            case MULTIPLY_F64: { NOT_IMPLEMENTED } break;
            case DIVIDE_F64: { NOT_IMPLEMENTED } break;
            case MODULO_F64: { NOT_IMPLEMENTED } break;
            default: {
                printf("Byte 0x%02x in module '%s' is not a valid opcode!\n", opcode, m->file);
                exit(1);
            }
        }
    }
}
