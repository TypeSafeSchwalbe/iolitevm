
#include <stdio.h>
#include <stdlib.h>

#include "vector.h"
#include "module.h"
#include "runtime.h"


int main() {
    // Load the test module
    Module test = module_from_file("test.iob");
    Runtime r = create_runtime();
    execute_instructions(&r, &test, test.data, test.length);
    
    // Execute the first function in the module and display its return value
    Function* main = vector_get(&test.functions, 0);
    char* base_frame = malloc(main->return_type_size); // we only need space for the return type
    vector_push(&r.frames, &base_frame);
    r.return_size = main->return_type_size; // set current return type
    r.return_var = 0; // set current return value variable offset
    execute_instructions(&r, &test, main->body_ptr, main->body_size_bytes); // execute the function body    
    printf("Main function returned %d\n", *base_frame); // Display the returned value
    free(base_frame); // free the base frame

    return 0;
}






