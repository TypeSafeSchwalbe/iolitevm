
#include <stdio.h>
#include <stdlib.h>

#include "vector.h"
#include "dlibs.h"
#include "module.h"
#include "runtime.h"


int main() {
    // load dynamic libraries
    DLibLoader dlibs = create_lib_loader();
    dlibs_load(&dlibs, "testdlib/out/testdlib.so");

    // manually create a test module, reading binaries is still a TODO
    MString main_s = (MString) { .length = 4, .data = "main" };
    MString println_f32_s = (MString) { .length = 11, .data = "println_f32" };
    Module test;
    test.body = (Instruction[]) {
        { .type = FUNCTION, .data = { .function_data = { .name = main_s, .argc = 0, .varc = 6, .body = (Instruction[]) {
            // var 0 = current num
            // var 1 = 1
            // var 2 = print until num
            // var 3 = comparison result
            // var 4 = current num as f32, then pointer to alloc
            // var 5 = current num + 1
            { .type = PUT_U64, .data = { .put_u64_data = { .value = 0, .dest = 0 } } },
            { .type = PUT_U64, .data = { .put_u64_data = { .value = 1, .dest = 1 } } },
            { .type = PUT_U64, .data = { .put_u64_data = { .value = 16, .dest = 2 } } },
            { .type = LOOP, .data = { .loop_data = { .body = (Instruction[]) {
                { .type = GREATER_THAN_EQUALS, .data = { .greater_than_equals_data = { .a = 0, .b = 2, .dest = 3 } } },
                { .type = IF, .data = { .if_data = { .condition = 3, .if_body = (Instruction[]) {
                    { .type = BREAK }
                }, .if_body_length = 1, .else_body = (Instruction[]) { 0 }, .else_body_length = 0 } } },
                { .type = CONVERT_F32, .data = { .convert_f32_data = { .x = 0, .dest = 4 } } },
                { .type = EXTERNAL_CALL, .data = { .external_call_data = { .name = println_f32_s, .argc = 1, .argv = (VarIdx[]) { 4 }, .returned = 0 } } },
                { .type = MALLOC_FIXED, .data = { .malloc_fixed_data = { .size = 16, .dest = 4 } } },
                { .type = ADD, .data = { .multiply_data = { .a = 0, .b = 1, .dest = 5 } } },
                { .type = COPY, .data = { .copy_data = { .src = 5, .dest = 0 } } },
            }, .body_length = 7 } } },
            { .type = RETURN_NOTHING }
        }, .body_length = 5 } } },

        { .type = CALL, .data = { .call_data = { .name = main_s, .argv = (VarIdx[]) {0}, .returned = 0 } } }
    }; 
    test.body_length = 2;

    // create a runtime, discover all symbols and resolve them
    Runtime r = create_runtime();
    discover_symbols(&r, &test);
    resolve_symbols(&r, &dlibs, test.body, test.body_length);

    // create a garbage collector
    GC gc = create_gc();

    // execute the module
    execute_instructions(&r, &gc, test.body, test.body_length);

    // let the garbage collector run
    gc_run(&gc);

    dlibs_free(&dlibs);
    return 0;
}






