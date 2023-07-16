
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#include "vector.h"
#include "dlibs.h"
#include "module.h"
#include "runtime.h"


int main() {
    // load dynamic libraries
    DLibLoader dlibs = create_lib_loader();
    dlibs_load(&dlibs, "testdlib/out/testdlib.so");

    // manually create a test module, reading binaries is still a TODO
    /*
    MString main_s = (MString) { .length = 4, .data = "main" };
    MString println_f32_s = (MString) { .length = 11, .data = "println_f32" };
    Module test;
    test.body = (Instruction[]) {
        { .type = FUNCTION, .data = { .function_data = { .name = main_s, .argc = 0, .varc = 7, .body = (Instruction[]) {
            { .type = PUT_U64, .data = { .put_u64_data = { .value = 0, .dest = 0 } } },
            { .type = PUT_U64, .data = { .put_u64_data = { .value = 1, .dest = 1 } } },
            { .type = PUT_U64, .data = { .put_u64_data = { .value = 100000000, .dest = 2 } } },
            { .type = LOOP, .data = { .loop_data = { .body = (Instruction[]) {
                { .type = GREATER_THAN_EQUALS, .data = { .greater_than_equals_data = { .a = 0, .b = 2, .dest = 3 } } },
                { .type = IF, .data = { .if_data = { .condition = 3, .if_body = (Instruction[]) {
                    { .type = BREAK }
                }, .if_body_length = 1, .else_body = (Instruction[]) { 0 }, .else_body_length = 0 } } },
                { .type = ADD, .data = { .multiply_data = { .a = 0, .b = 1, .dest = 4 } } },
                { .type = COPY, .data = { .copy_data = { .src = 4, .dest = 0 } } },
            }, .body_length = 4 } } },
            { .type = PUT_F32, .data = { .put_f32_data = { .value = 6.28, .dest = 6 } } },
            { .type = EXTERNAL_CALL, .data = { .external_call_data = { .name = println_f32_s, .argc = 1, .argv = (VarIdx[]) { 6 }, .returned = 6 } } },
            { .type = RETURN_NOTHING }
        }, .body_length = 7 } } },

        { .type = ASYNC_CALL, .data = { .call_data = { .name = main_s, .argv = (VarIdx[]) {0}, .returned = 0 } } },
        { .type = ASYNC_CALL, .data = { .call_data = { .name = main_s, .argv = (VarIdx[]) {0}, .returned = 1 } } },
        { .type = ASYNC_CALL, .data = { .call_data = { .name = main_s, .argv = (VarIdx[]) {0}, .returned = 2 } } },
        { .type = ASYNC_CALL, .data = { .call_data = { .name = main_s, .argv = (VarIdx[]) {0}, .returned = 3 } } },
        { .type = ASYNC_CALL, .data = { .call_data = { .name = main_s, .argv = (VarIdx[]) {0}, .returned = 4 } } }
    }; 
    test.body_length = 6;
    */

    // Iolite source code approximation
    /*
        fun println_flt(float x) ext

        fun main(): 3.14 < 6.28
            fun() var0 = () ->
                f32 var1 = 6.28
                prirntln_flt(var1)
            (var0)()
    */

    MString main_s = (MString) { .length = 4, .data = "main" };
    MString println_flt_s = (MString) { .length = 11, .data = "println_flt" };
    Module test;
    test.body = (Instruction[]) {
        { .type = FUNCTION, .data = { .function_data = {
                .name = main_s, .argc = 0, .varc = 2,
                .condition_count = 1, .conditions = (Instruction*[]) { (Instruction[]) {

            { .type = PUT_FLT, .data = { .put_flt_data = { .value = 3.14, .dest = 0 } } },
            { .type = PUT_FLT, .data = { .put_flt_data = { .value = 6.28, .dest = 1 } } },
            { .type = LESS_THAN, .data = { .less_than_data = { .a = 0, .b = 1, .dest = 1 } } },
            { .type = ASSERT, .data = { .return_data = { .value = 1 } } }

        } }, .condition_lengths = (InstrC[]) { 4 }, .body = (Instruction[]) {

            { .type = PUT_CLOSURE, .data = { .put_closure_data = { .args_offset = 0, .body = (Instruction[]) {
                
                { .type = PUT_FLT, .data = { .put_flt_data = { .value = 6.28, .dest = 1 } } },
                { .type = EXTERNAL_CALL, .data = { .external_call_data = { .name = println_flt_s, .argc = 1, .argv = (VarIdx[]) { 1 }, .returned = 0 } } },
                { .type = RETURN_NOTHING }
            
            }, .body_length = 3, .dest = 0 } } },
            { .type = CLOSURE_CALL, .data = { .closure_call_data = { .called = 0, .argc = 0, .argv = (VarIdx[]) {0}, .returned = 0 } } },
            { .type = RETURN_NOTHING }
        
        }, .body_length = 3 } } },

        { .type = CALL, .data = { .call_data = { .name = main_s, .argv = (VarIdx[]) {0}, .returned = 0 } } },
    };
    test.body_length = 2;

    // flatten and combine all the modules into one long array of instructions
    Instruction* instructions;
    InstrC instruction_count;
    flatten_combine((Module[]) { test }, 1, &instructions, &instruction_count);

    // discover all symbols and resolve them
    Vector functions = create_vector(sizeof(Instruction_Function*));
    discover_symbols(instructions, instruction_count, &functions);
    resolve_symbols(&functions, &dlibs, instructions, instruction_count);

    // create a garbage collector and garbage collector
    GC gc = create_gc();
    ThreadPool tp = create_thread_pool();

    // execute the module
    IoliteAllocation* base_frame = gc_allocate(&gc, 0);
    execute(&gc, &tp, instructions, instruction_count, base_frame, NULL, 0);

    // wait for all threads to finish (and clean the thread pool up)
    threadpool_cleanup(&tp);

    // let the garbage collector run
    gc_run(&gc);

    // cleanup
    dlibs_free(&dlibs);
    free(instructions);
    gc_cleanup(&gc);
    return 0;
}






