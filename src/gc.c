
#include <stdlib.h>
#include <stdio.h>
#include "gc.h"
#include "cli.h"


GC create_gc() {
    GC gc;
    gc.allocations = create_vector(sizeof(IoliteAllocation));
    gc.unused = create_vector(sizeof(IoliteAllocation*));
    gc.running = 0;
    return gc;
}

#define GC_REACHABLE 1
#define GC_USED 2
#define GC_COLLECTABLE 4

IoliteAllocation* gc_allocate(GC* gc, uint64_t size) {
    // create allocation struct
    IoliteAllocation a;
    a.size = size;
    a.values = malloc(sizeof(IoliteValue) * size);
    a.gc_flags = GC_USED;
    if(!gc->running) { a.gc_flags |= GC_COLLECTABLE; }
    a.stack_reference_count = 1;
    printf("[GC debug] created allocation of %d elements at address %p\n", size, a.values);
    // set the types of each allocated value to UNIT
    // this is to make sure that the (currently garbage) data doesn't get interpreted as a reference,
    // which the runtime reference counting would try to change the reference count of
    for(uint64_t var_index = 0; var_index < size; var_index += 1) {
        a.values[var_index].type = UNIT;
    }
    // reuse places in the allocations array where old allocations (now free) are stored
    if(gc->unused.size > 0) {
        IoliteAllocation** reused = vector_get(&gc->unused, gc->unused.size - 1);
        vector_pop(&gc->unused);
        **reused = a;
    }
    // else push it onto the vector
    else {
        size_t allocation_index = gc->allocations.size;
        vector_push(&gc->allocations, &a);
        return vector_get(&gc->allocations, allocation_index);
    }
}

void gc_mark_reachable(IoliteAllocation* a) {
    // mark as reachable
    a->gc_flags |= GC_REACHABLE;
    // mark all values in the allocation that are references as reachable, recursively
    for(size_t contained_value_index = 0; contained_value_index < a->size; contained_value_index += 1) {
        IoliteValue* v = &a->values[contained_value_index];
        switch(v->type) {
            case REFERENCE: {
                if(v->value.ref == NULL) { continue; }
                if((v->value.ref->gc_flags & GC_REACHABLE) > 0) { continue; }
                gc_mark_reachable(v->value.ref);
            } break;
            case CLOSURE: {
                if((v->value.closure.frame->gc_flags & GC_REACHABLE) > 0) { continue; }
                gc_mark_reachable(v->value.closure.frame);
            } break;
            default: {}
        }
    }
}

void gc_run(GC* gc) {
    gc->running = 1;
    size_t allocation_count = gc->allocations.size; // in case the allocations array grows during GC
    // mark all used allocations with a stack reference count of over 0 as reachable
    for(size_t allocation_index = 0; allocation_index < allocation_count; allocation_index += 1) {
        IoliteAllocation* a = vector_get(&gc->allocations, allocation_index);
        if((a->gc_flags & GC_USED) <= 0) { continue; }
        if((a->stack_reference_count) <= 0) { continue; }
        gc_mark_reachable(a);
    }
    // free all used allocations that aren't marked as reachable and mark them as unused
    for(size_t allocation_index = 0; allocation_index < allocation_count; allocation_index += 1) {
        IoliteAllocation* a = vector_get(&gc->allocations, allocation_index);
        if((a->gc_flags & GC_USED) <= 0) { continue; }
        if(((a->gc_flags & GC_REACHABLE) > 0) || ((a->gc_flags & GC_COLLECTABLE) <= 0)) {
            // mark surviving allocation as not reachable (for next GC cycle) and collectable
            a->gc_flags &= ~GC_REACHABLE;
            a->gc_flags |= GC_COLLECTABLE;
            continue;
        }
        // mark as unused so the array index can be reused
        a->gc_flags &= ~GC_USED;
        vector_push(&gc->unused, &a);
        printf("[GC debug] freeing allocation of %d elements at address %p (GC run)\n", a->size, a->values);
        free(a->values);
    }
    gc->running = 0;
}

void gc_cleanup(GC* gc) {
    for(size_t allocation_index = 0; allocation_index < gc->allocations.size; allocation_index += 1) {
        IoliteAllocation* a = vector_get(&gc->allocations, allocation_index);
        if((a->gc_flags & GC_USED) <= 0) { continue; }
        printf("[GC debug] freeing allocation of %d elements at address %p (cleanup)\n", a->size, a->values);
        free(a->values);
    }
    vector_cleanup(&gc->allocations);
    vector_cleanup(&gc->unused);
}