
#pragma once

#include <stdint.h>
#include <stdio.h>
#include "vector.h"
#include "dlibs.h"
#include "module.h"
#include "gc.h"


typedef struct {
    IoliteValue* values;
    VarIdx size;
} Frame;

typedef struct {
    Vector functions;
    Vector frames;
    Vector allocations;

    uint8_t returned_val;
    VarIdx returned_val_var;
} Runtime;


Runtime create_runtime();

void discover_symbols(Runtime* r, Module* m);

void resolve_symbols(Runtime* r, DLibLoader* l, Instruction* instructions, InstrC instruction_count);


typedef uint8_t ExecutionSignal;
#define NEXT_INSTRUCTION 0
#define CONTINUE_LOOP 1
#define BREAK_LOOP 2
#define RETURN_FUNCTION 3

ExecutionSignal execute_instruction(Runtime* r, GC* gc, Instruction* i);

ExecutionSignal execute_instructions(Runtime* r, GC* gc, Instruction* instructions, InstrC instruction_count);