#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "assembler.hpp"
#include "tools.hpp"


int main()
    {

    cmdel_t* codeArray = (cmdel_t*) calloc(CodeArrayMaxLen, sizeof(*codeArray));
    // compile("data.txt", codeArray);
    // compile("quadratic.txt", codeArray);
    // compile("fact.txt", codeArray);

    return 0;
    }
