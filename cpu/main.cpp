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

    // cmdel_t regs[regslen] = {};         
    // cpu.regs = regs;

    // Stack stk = {}; 
    // stackCreate(&stk);
    // cpu.stk = &stk;

    // Stack callStack = {};
    // stackCreate(&callStack);
    // cpu.callStack = &callStack;

    // cmdel_t ram[ramlen] = {};
    // cpu.ram = ram;

    cmdel_t* codeArray = setvmbuf("../assembler/assemblerfile.bin");
    ispProgr(&cpu, codeArray);
    return 0;
    }
