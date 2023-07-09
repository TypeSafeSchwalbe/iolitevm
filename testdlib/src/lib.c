
#include <stdio.h>
#include "../../iolitevm_api.h"

IoliteValue println_f32(IoliteValue* args) {
    printf("%f\n", args[0].value.f32);
    return IoliteUnitValue;
}






