
#pragma once

#include "vector.h"
#include "../iolitevm_api.h"


typedef struct {
    Vector allocations;
    Vector unused;
} GC;


GC create_gc();

IoliteAllocation* gc_allocate(GC* gc, uint64_t size);

void gc_run(GC* gc);
