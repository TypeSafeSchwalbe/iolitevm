
#pragma once

#include <stdint.h>
#include <stdio.h>
#include "vector.h"
#include "dlibs.h"
#include "module.h"
#include "gc.h"
#include "threads.h"


typedef struct {
    Vector functions;
} Runtime;


Runtime create_runtime();

void discover_symbols(Runtime* r, Module* m);

void resolve_symbols(Runtime* r, DLibLoader* l, Instruction* instructions, InstrC instruction_count);

void flatten_combine(Module* modules, size_t module_count, Instruction** instructions, InstrC* instruction_count);

void execute(Runtime* r, GC* gc, ThreadPool* tp, Instruction* instructions, InstrC instruction_count, IoliteAllocation* base_frame, IoliteValue* base_return, InstrC current_index);

void runtime_cleanup(Runtime* r);