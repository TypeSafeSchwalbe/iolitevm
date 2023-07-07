
#ifndef RUNTIME_HEADER
#define RUNTIME_HEADER


#include <stdint.h>
#include <stdio.h>
#include "vector.h"
#include "module.h"


typedef enum {
    MALLOC = 0x00,
    MEM_SET = 0x01,
    MEM_GET = 0x02,
    ENTER = 0x03,
    EXIT = 0x04,
    COPY = 0x05,
    FUNCTION = 0x06,
    CALL = 0x07,
    RETURN = 0x08,
    IF = 0x09,
    LOOP = 0x0A,
    BREAK = 0x0B,
    CONTINUE = 0x0C,
    PUT_UINT = 0x0D,
    ADD_UINT = 0x0E,
    SUBTRACT_UINT = 0x0F,
    MULTIPLY_UINT = 0x10,
    DIVIDE_UINT = 0x11,
    MODULO_UINT = 0x12,
    PUT_SINT = 0x13,
    ADD_SINT = 0x14,
    SUBTRACT_SINT = 0x15,
    MULTIPLY_SINT = 0x16,
    DIVIDE_SINT = 0x17,
    MODULO_SINT = 0x18,
    PUT_F32 = 0x19,
    ADD_F32 = 0x1A,
    SUBTRACT_F32 = 0x1B,
    MULTIPLY_F32 = 0x1C,
    DIVIDE_F32 = 0x1D,
    MODULO_F32 = 0x1E,
    PUT_F64 = 0x1F,
    ADD_F64 = 0x20,
    SUBTRACT_F64 = 0x21,
    MULTIPLY_F64 = 0x22,
    DIVIDE_F64 = 0x23,
    MODULO_F64 = 0x24
} Instruction;

typedef struct {
    Vector frames;

    uint8_t return_size;
    uint16_t return_var;
} Runtime;

Runtime create_runtime();

#define NOT_IMPLEMENTED printf("Opcode 0x%02x is not yet implemented!\n", opcode); exit(1);

void execute_instructions(Runtime* r, Module* m, char* instr, size_t count);


#endif