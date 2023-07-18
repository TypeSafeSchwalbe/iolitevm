
#pragma once

#include "vector.h"
#include "gc.h"
#include "module.h"


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


typedef struct {
    InstrC index;
    MString* name;
} CallInfo;

void break_down(GC* gc, Vector* call_history, Vector* frames, Instruction* instructions, InstrC current_instruction, char* reason);