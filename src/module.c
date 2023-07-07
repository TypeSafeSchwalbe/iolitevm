
#include <stdio.h>
#include <stdint.h>
#include "vector.h"
#include "module.h"


void handle_io_error(char* file) {
    printf("Unable to read file '%s'!\n", file);
    exit(1);
}

void read_module_file(Module* m) {
    FILE* f = fopen(m->file, "rb");
    if(f == NULL || fseek(f, 0, SEEK_END)) {
        handle_io_error(m->file);
        return;
    }
    m->length = ftell(f);
    rewind(f);
    if(m->length == -1) {
        handle_io_error(m->file);
        return;
    }
    m->data = malloc(m->length);
    if(m->data == NULL || fread(m->data, 1, m->length, f) != m->length) {
        handle_io_error(m->file);
        return;
    }
}

Module module_from_file(char* file) {
    Module m;
    m.file = file;
    read_module_file(&m);
    m.functions = create_vector(sizeof(Function));
    return m;
}