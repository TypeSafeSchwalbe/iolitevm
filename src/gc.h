
#pragma once

#include <stdatomic.h>
#include "vector.h"
#include "../iolitevm_api.h"


typedef struct {
    Vector allocations;
    Vector unused;
    _Atomic(uint8_t) running;
} GC;


GC create_gc();

IoliteAllocation* gc_allocate(GC* gc, uint64_t size);

void gc_run(GC* gc);

void gc_cleanup(GC* gc);
