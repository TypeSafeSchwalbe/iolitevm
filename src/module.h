
#ifndef MODULE_HEADER
#define MODULE_HEADER


#include <stdint.h>
#include "vector.h"


typedef struct {
    uint8_t return_type_size;
    uint16_t body_size_bytes;
    char* body_ptr;
} Function;

typedef struct {
    char* file;
    long length;
    char* data;

    Vector functions;
} Module;


Module module_from_file(char* file);


#endif