
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
} Runtime;


Runtime create_runtime();

void discover_symbols(Runtime* r, Module* m);

void resolve_symbols(Runtime* r, DLibLoader* l, Instruction* instructions, InstrC instruction_count);

void flatten_combine(Module* modules, size_t module_count, Instruction** instructions, InstrC* instruction_count);

void execute(Runtime* r, GC* gc, Instruction* instructions, InstrC instruction_count);