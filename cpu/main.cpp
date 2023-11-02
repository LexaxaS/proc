#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include "cpu.hpp"
#include "tools.hpp"

int main()
    {
    Cpu cpu = {};
    cpuCreate(&cpu);
    cmdel_t* codeArray = setvmbuf("../assembler/assemblerfile.bin");
    ispProgr(&cpu, codeArray);
    return 0;
    }
