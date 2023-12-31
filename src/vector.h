
#pragma once

#include <stdlib.h>


typedef struct {
    char* data;
    size_t e_size; 
    size_t size;
    size_t alloc_size;
} Vector;


Vector create_vector(size_t e_size);

void* vector_get(Vector* v, size_t index);

void vector_set(Vector* v, size_t index, void* value);

void vector_push(Vector* v, void* value);

void vector_pop(Vector* v);

void vector_cleanup(Vector* v);