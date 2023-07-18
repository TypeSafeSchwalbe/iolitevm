
#pragma once

#include "vector.h"


typedef struct {
    char* name;
    Vector values;
} CliArg;

#define ARG_MODULES "mods"
#define ARG_SLIBS "slibs"
#define ARG_START "start"


Vector parse_cli_args(int argc, char** argv);

void invalid_args();

void error(char* reason);