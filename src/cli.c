
#include <stdio.h>
#include <stdlib.h>
#include "cli.h"

void error(char* reason) {
    printf("\nError! %s\n\n", reason);
    exit(1);
}

void invalid_args() {
    error("The provided arguments are incorrect!\n\n-mods <module-file> <module-file> ...\n    specifies what module files (.iob) to load\n-slibs <shared-lib> <shared-lib> ...\n    specified what shared libraries (.so / .dll) to load\n-start <main-function>\n    specifies the entry point of the program (shall not have any parameters or return values)");
}

Vector parse_cli_args(int argc, char** argv) {
    Vector pargs = create_vector(sizeof(CliArg));
    int argi = 1;
    CliArg* carg = NULL;
    if(argc <= 1) { invalid_args(); }
    while(argi < argc) {
        if(argi == 1 && argv[argi][0] != '-') { invalid_args(); }
        if(argv[argi][0] == '-') {
            CliArg new_arg;
            vector_push(&pargs, &new_arg);
            carg = vector_get(&pargs, pargs.size - 1);
            carg->name = argv[argi] + 1;
            carg->values = create_vector(sizeof(char*));
        } else {
            vector_push(&carg->values, &argv[argi]);
        }
        argi += 1;
    }
    return pargs;
}