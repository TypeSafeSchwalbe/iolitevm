
#include <stdio.h>
#include "../../iolitevm_api.h"

IoliteValue println_flt(IoliteValue* args) {
    printf("%f\n", args[0].value.flt);
    return IoliteUnitValue;
}






