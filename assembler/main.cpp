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

    // compile("data.txt", "assemblerfile.bin");
    // compile("quadratic.txt", "assemblerfile.bin");
    // compile("fact.txt", "assemblerfile.bin");
    compile("circle.txt", "assemblerfile.bin");

    return 0;
    }
