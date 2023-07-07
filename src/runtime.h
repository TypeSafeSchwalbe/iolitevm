
#ifndef RUNTIME_HEADER
#define RUNTIME_HEADER


#define INSTR_MALLOC 0x00
#define INSTR_MEM_SET 0x01
#define INSTR_MEM_GET 0x02
#define INSTR_ENTER 0x03
#define INSTR_EXIT 0x04
#define INSTR_COPY 0x05
#define INSTR_FUNCTION 0x06
#define INSTR_CALL 0x07
#define INSTR_RETURN 0x08
#define INSTR_IF 0x09
#define INSTR_LOOP 0x0A
#define INSTR_BREAK 0x0B
#define INSTR_CONTINUE 0x0C
#define INSTR_PUT_UINT 0x0D
#define INSTR_ADD_UINT 0x0E
#define INSTR_SUBTRACT_UINT 0x0F
#define INSTR_MULTIPLY_UINT 0x10
#define INSTR_DIVIDE_UINT 0x11
#define INSTR_MODULO_UINT 0x12
#define INSTR_PUT_SINT 0x13
#define INSTR_ADD_SINT 0x14
#define INSTR_SUBTRACT_SINT 0x15
#define INSTR_MULTIPLY_SINT 0x16
#define INSTR_DIVIDE_SINT 0x17
#define INSTR_MODULO_SINT 0x18
#define INSTR_PUT_F32 0x19
#define INSTR_ADD_F32 0x1A
#define INSTR_SUBTRACT_F32 0x1B
#define INSTR_MULTIPLY_F32 0x1C
#define INSTR_DIVIDE_F32 0x1D
#define INSTR_MODULO_F32 0x1E
#define INSTR_PUT_F64 0x1F
#define INSTR_ADD_F64 0x20
#define INSTR_SUBTRACT_F64 0x21
#define INSTR_MULTIPLY_F64 0x22
#define INSTR_DIVIDE_F64 0x23
#define INSTR_MODULO_F64 0x24


#include <stdint.h>
#include <stdio.h>
#include "vector.h"
#include "module.h"


typedef struct {
    Vector frames;

    uint8_t return_size;
    uint16_t return_var;
} Runtime;

Runtime create_runtime();

#define NOT_IMPLEMENTED printf("Opcode 0x%02x is not yet implemented!\n", opcode); exit(1);

void execute_instructions(Runtime* r, Module* m, char* instr, size_t count);


#endif