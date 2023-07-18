
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#include "cli.h"
#include "vector.h"
#include "dlibs.h"
#include "module.h"
#include "runtime.h"


void gc_worker(void* args) {
    GC* gc = (GC*) ((void**) args)[0];
    uint8_t* running = (uint8_t*) ((void**) args)[1];
    while(*running) {
        gc_run(gc);
    }
}

int main(int argc, char** argv) {
    // parse arguments
    Vector args = parse_cli_args(argc, argv);

    // load dynamic libraries
    DLibLoader dlibs = create_lib_loader();
    for(size_t argi = 0; argi < args.size; argi += 1) {
        CliArg* arg = vector_get(&args, argi);
        if(strcmp(arg->name, ARG_SLIBS) != 0) { continue; }
        for(size_t dlibi = 0; dlibi < arg->values.size; dlibi += 1) {
            char** dlib = vector_get(&arg->values, dlibi);
            dlibs_load(&dlibs, *dlib);
        }
    }

    // load modules, flatten and combine all the modules into one long array of instructions, resolve all symbols
    Vector modules = create_vector(sizeof(Module));
    for(size_t argi = 0; argi < args.size; argi += 1) {
        CliArg* arg = vector_get(&args, argi);
        if(strcmp(arg->name, ARG_MODULES) != 0) { continue; }
        for(size_t modi = 0; modi < arg->values.size; modi += 1) {
            char** module_name = vector_get(&arg->values, modi);
            Module module = create_module(*module_name);
            vector_push(&modules, &module);
        }
    }
    Instruction* instructions;
    InstrC instruction_count;
    flatten_combine((Module*) modules.data, modules.size, &instructions, &instruction_count);
    Vector found_functions;
    resolve_symbols(&dlibs, instructions, instruction_count, &found_functions);

    // create a garbage collector and thread pool
    GC gc = create_gc();
    ThreadPool tp = create_thread_pool();

    // start the GC worker on a new thread
    uint8_t gc_running = 1;
    void** gc_worker_args = (void*[]) { &gc, &gc_running };
    threadpool_do(&tp, &gc_worker, gc_worker_args);

    // execute the main function
    for(size_t argi = 0; argi < args.size; argi += 1) {
        CliArg* arg = vector_get(&args, argi);
        if(strcmp(arg->name, ARG_START) != 0) { continue; }
        if(arg->values.size != 1) { invalid_args(); }
        char* search_function_name = *((char**) vector_get(&arg->values, 0));
        size_t search_function_name_length = strlen(search_function_name);
        uint8_t found = 0;
        for(size_t functioni = 0; functioni < found_functions.size; functioni += 1) {
            Instruction_Function* function = *((Instruction_Function**) vector_get(&found_functions, functioni));
            if(search_function_name_length != function->name.length ||
            memcmp(search_function_name, function->name.data, search_function_name_length) != 0) { continue; }

            if(function->argc != 0) { invalid_args(); }
            IoliteAllocation* base_frame = gc_allocate(&gc, function->varc);
            for(size_t var = 0; var < base_frame->size; var += 1) {
                base_frame->values[var].type = UNIT;
            }
            execute(&gc, &tp, instructions, instruction_count, base_frame, NULL, function->body_instruction_index, &function->name);

            found = 1;
            break;
        }
        if(found) { break; }
        invalid_args();
    }

    // wait for all threads to finish (and clean the thread pool up)
    gc_running = 0;
    threadpool_cleanup(&tp);

    // cleanup
    dlibs_free(&dlibs);
    free(instructions);
    gc_cleanup(&gc);
    vector_cleanup(&found_functions);
    return 0;
}






