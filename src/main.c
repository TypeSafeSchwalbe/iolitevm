
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

    /*
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
    */

    // Equivalent Iolite code
    /*
        fun println_flt(float x) ext

        trait Addable
            add(self other) -> self
        
        impl float: Addable
            add(self other) -> self
                self + other

        pub fun main()
            float var0 = 3.14
            float var1 = 3.14
            float var2 = var0.add(var1)
            println_flt(var2)
    */
    /*
    MString main_s = (MString) { .length = 4, .data = "main" };
    MString println_flt_s = (MString) { .length = 11, .data = "println_flt" };
    MString add_s = (MString) { .length = 3, .data = "add" };
    MString addable_s = (MString) { .length = 7, .data = "addable" };
    MString flt_impl_s = (MString) { .length = 8, .data = "flt$impl" };
    Module test;
    test.body = (Instruction[]) {
        { .type = TRAIT, .data = { .trait_data = { .name = addable_s, .method_count = 1, .method_names = (MString[]) { add_s } } } },
        { .type = FUNCTION, .data = { .function_data = {
            .name = add_s, .argc = 2, .varc = 1,
            .condition_count = 0, .conditions = (Instruction*[]) { (Instruction[]) {0} }, .condition_lengths = (InstrC[]) {0}, .body = (Instruction[]) {

            { .type = ADD, .data = { .add_data = { .a = 0, .b = 1, .dest = 2 } } },
            { .type = RETURN, .data = { .return_data = { .value = 2 } } }

        }, .body_length = 2 } } },
        { .type = IMPLEMENTS, .data = { .implements_data = { .name = flt_impl_s, .trait_count = 1, .trait_names = (MString[]) { addable_s }, .trait_impl_function_names = (MString*[]) { (MString[]) { add_s } } } } },

        { .type = FUNCTION, .data = { .function_data = {
                .name = main_s, .argc = 0, .varc = 3,
                .condition_count = 0, .conditions = (Instruction*[]) { (Instruction[]) {0} }, .condition_lengths = (InstrC[]) {0}, .body = (Instruction[]) {

            { .type = PUT_FLT, .data = { .put_flt_data = { .value = 3.14, .dest = 0 } } },
            { .type = ADD_IMPLEMENTS, .data = { .add_implements_data = { .value = 0, .impl_name = flt_impl_s } } },
            { .type = PUT_FLT, .data = { .put_flt_data = { .value = 3.14, .dest = 1 } } },
            { .type = METHOD_CALL, .data = { .method_call_data = { .value = 0, .trait_name = addable_s, .method_name = add_s, .argv = (VarIdx[]) { 0, 1 }, .returned = 2 } } },
            { .type = EXTERNAL_CALL, .data = { .external_call_data = { .name = println_flt_s, .argc = 1, .argv = (VarIdx[]) { 2 }, .returned = 0 } } },
            { .type = RETURN_NOTHING }
        
        }, .body_length = 6 } } },

        { .type = CALL, .data = { .call_data = { .name = main_s, .argv = (VarIdx[]) {0}, .returned = 0 } } },
    };
    test.body_length = 5;
    */

    /*
        # strings

        05 00 00 00                       # 5 strings
        04 00                             # length 4
           6D 61 69 6E                       # "main"
        0B 00                             # length 11
           70 72 69 6E 74 6C 6E 5F
           66 6C 74                          # "println_flt"
        03 00                             # length 3
           61 64 64                          # "add"
        07 00                             # length 7
           61 64 64 61 62 6C 65              # "addable"
        08 00                             # length 8
           66 6C 74 24 69 6D 70 6C           # "flt$impl"
        
        # program

        08                                # trait
           03 00 00 00                       # trait name ([3]="addable")
           01 00                             # method count
              02 00 00 00                       # method name ([2]="add")
        00                                # function
           02 00 00 00                       # function name ([2]="add")
           02 00                             # param count
           01 00                             # variable count
           00 00                             # condition count
           02 00 00 00 00 00 00 00           # body length
              1C                                # add
                 00 00                             # a var
                 01 00                             # b var
                 02 00                             # dest var
              05                                # return
                 02 00                             # return val var
        09                                # implements
           04 00 00 00                       # implement name ([4]="flt$impl")
           01 00                             # trait count
              03 00 00 00                       # trait name ([3]="addable")
              01 00                             # trait method count
                 02 00 00 00                       # impl function name ([2]="add")
        00                                # function
           00 00 00 00                       # function name ([0]="main")
           00 00                             # param count
           03 00                             # variable count
           00 00                             # condition count
           06 00 00 00 00 00 00 00           # body length
              13                                # put float
                 1F 85 EB 51 B8 1E 09 40           # 3.14
                 00 00                             # dest var
              0A                                # add implements
                 00 00                             # value var
                 04 00 00 00                       # implements name ([4]="flt$impl")
              13                                # put float
                 1F 85 EB 51 B8 1E 09 40           # 3.14
                 01 00                             # dest var
              0B                                # method call
                 00 00                             # value var
                 03 00 00 00                       # trait name ([3]="addable")
                 02 00 00 00                       # method name ([2]="add")
                 02 00                             # param count
                    00 00                             # param var
                    01 00                             # param var
                 02 00                             # return val dest var
              03                                # extern call
                 01 00 00 00                       # function name ([1]="println_flt")
                 01 00                             # param count
                    02 00                             # param var
                 00 00                             # return val dest var
              06                                # return nothing
        01                                # call
           00 00 00 00                       # function name ([0]="main")
           00 00                             # param count
           00 00                             # return val dest var
    */
    Module test = create_module("test.iob");

    // flatten and combine all the modules into one long array of instructions, resolve all symbols
    Instruction* instructions;
    InstrC instruction_count;
    flatten_combine((Module[]) { test }, 1, &instructions, &instruction_count);
    resolve_symbols(&dlibs, instructions, instruction_count);

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






