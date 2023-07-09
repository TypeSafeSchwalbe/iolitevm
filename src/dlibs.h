
#pragma once

#include "vector.h"


typedef struct {
    Vector loaded;
} DLibLoader;


DLibLoader create_lib_loader();

void dlibs_load(DLibLoader* l, char* file);

void* dlibs_find(DLibLoader* l, char* name);

void dlibs_free(DLibLoader* l);