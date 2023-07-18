

#pragma once

#include "dlibs.h"
#include "module.h"
#include "gc.h"
#include "threads.h"


void flatten_combine(Module* modules, size_t module_count, Instruction** instructions, InstrC* instruction_count);

void resolve_symbols(DLibLoader* l, Instruction* instructions, InstrC instruction_count, Vector* functions);

void execute(GC* gc, ThreadPool* tp, Instruction* instructions, InstrC instruction_count, IoliteAllocation* base_frame, IoliteValue* base_return, InstrC current_index, MString* current_call_name);