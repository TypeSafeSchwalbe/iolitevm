
#include <stdlib.h>
#include <string.h>
#include "vector.h"


Vector create_vector(size_t e_size) {
    const size_t DEFAULT_ALLOCATION_SIZE = 16;
    Vector a;
    a.data = (char*) malloc(e_size * DEFAULT_ALLOCATION_SIZE);
    a.e_size = e_size;
    a.size = 0;
    a.alloc_size = DEFAULT_ALLOCATION_SIZE;
    return a;
}

inline void* vector_get(Vector* a, size_t index) {
    return (void*) (a->data + index * a->e_size);
}

inline void vector_set(Vector* a, size_t index, void* value) {
    memcpy(a->data + index * a->e_size, value, a->e_size);
}

void vector_push(Vector* a, void* value) {
    if(a->size + 1 >= a->alloc_size) { // not enough space! double allocation size
        a->alloc_size *= 2;
        a->data = (char*) realloc(a->data, a->alloc_size * a->e_size);
    }
    // put in new element
    memcpy(a->data + a->size * a->e_size, value, a->e_size);
    a->size += 1;
}

inline void vector_pop(Vector* a) {
    a->size -= 1;
}

void vector_cleanup(Vector* v) {
    free(v->data);
}
