
#pragma once

#include <stdint.h>
#include <stdio.h>
#include "vector.h"
#include "dlibs.h"
#include "module.h"
#include "gc.h"
#include "threads.h"


void flatten_combine(Module* modules, size_t module_count, Instruction** instructions, InstrC* instruction_count);


void discover_symbols(Instruction* instructions, InstrC instruction_count, Vector* functions);

void resolve_symbols(Vector* functions, DLibLoader* l, Instruction* instructions, InstrC instruction_count);


void execute(GC* gc, ThreadPool* tp, Instruction* instructions, InstrC instruction_count, IoliteAllocation* base_frame, IoliteValue* base_return, InstrC current_index);