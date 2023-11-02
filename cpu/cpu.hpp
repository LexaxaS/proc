#ifndef CPU_HPP
#define CPU_HPP

#include <stdio.h>
#include "stackfunc.hpp"

typedef int cmdel_t;

const size_t ramwidth = 60;
const size_t ramheight = 30;
const size_t ramlen = ramwidth * ramheight;

const size_t regslen = 5;
const cmdel_t multiplier = 100;


struct Cpu
    {
    Stack* stk;
    cmdel_t* regs;
    cmdel_t* ram;
    Stack* callStack;
    };

int ispProgr(Cpu* cpu, cmdel_t* codeArr);
cmdel_t* setvmbuf(char filename_i[]);
error_t cpuCreate(Cpu* cpu);

#endif