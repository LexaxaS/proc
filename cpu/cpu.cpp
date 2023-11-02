#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include "cpu.hpp"
#include "tools.hpp"

enum CMD_Commands
    {
    #define DEF_CMD(name, num, ...)\
        CMD_##name = num,
    #include "../commands.txt"
    #undef DEF_CMD
    };

enum REG_nums
    {REG_RAX = 1,
     REG_RBX = 2,
     REG_RCX = 3,
     REG_RDX = 4};

enum ArgFormats
    {ARG_FORMAT_IMMED = (1 << 5),
     ARG_FORMAT_REG = (1 << 6),
     ARG_FORMAT_RAM = (1 << 7)};

enum Errors
    {NO_ERROR = 0,
     CODEARR_ERROR = 1,
     RAM_NEG_ARG = 2};

struct Arg
    {
    cmdel_t* argval;
    bool isram;
    };

int unknownvm(int linenum);
error_t argdecode(Cpu* cpu, size_t* elnum, cmdel_t* codeArr, cmdel_t com, Arg* arg);

error_t cpuCreate(Cpu* cpu)
    {
    MY_ASSERT_HARD(cpu);

    Stack* stk = (Stack*) calloc(1, sizeof(Stack)); 
    stackCreate(stk);
    cpu->stk = stk;

    Stack* callStack = (Stack*) calloc(1, sizeof(Stack));
    stackCreate(callStack);
    cpu->callStack = callStack;

    cmdel_t* ram = (cmdel_t*) calloc(ramlen, sizeof(*ram));
    cpu->ram = ram;

    cmdel_t* regs = (cmdel_t*) calloc(regslen, sizeof(*regs));
    cpu->regs = regs;

    return NO_ERROR;
    }

cmdel_t* setvmbuf(char* filename_i)
    {
    MY_ASSERT_HARD(filename_i);

    FILE *fp = fileopenerRB(filename_i);
    size_t filelen = fileLen(filename_i);

    cmdel_t* codeArr = (cmdel_t*) calloc(filelen, sizeof(*codeArr));
    fread(codeArr, sizeof(*codeArr), filelen, fp);    

    // for (size_t i = 0; i < filelen + 1; i++)
    //     printf("%d[%d] ", codeArr[i], i);
    // printf("\n");

    fclose(fp);      
    return codeArr;
    }  

int ispProgr(Cpu* cpu, cmdel_t* codeArr)
    {
    MY_ASSERT_HARD(cpu);
    MY_ASSERT_HARD(codeArr);

    #define DEF_CMD(name, num, isarg, ...)   \
        case CMD_##name:                     \
            __VA_ARGS__                      \
            break;

    size_t elnum = 0;
    int com = 0;

    bool ishlt = false;
    while (elnum < INT_MAX && !ishlt)
        {
        com = codeArr[elnum];
        // printf("%d\n", com);

        switch (com & ~(7 << 5))
            {

            #include "../commands.txt"

            default: 
                unknownvm(elnum);
                return CODEARR_ERROR;
                break;
            }

        // printf("\n");
        // for (size_t i = 0; i < cpu->stk->size; i++)
        //     printf("%d\n", cpu->stk->dataptr[i]);
        // printf("\n");
        // printf("\n%d\n\n", cpu->regs[2]);

        elnum++;
        }

    #undef DEF_CMD
    return NO_ERROR;
    }

error_t argdecode(Cpu* cpu, size_t* elnum, cmdel_t* codeArr, cmdel_t com, Arg* arg)
    {
    MY_ASSERT_HARD(cpu);
    MY_ASSERT_HARD(elnum);
    MY_ASSERT_HARD(codeArr);
    MY_ASSERT_HARD(arg);

    if (com & ARG_FORMAT_REG)
        {
        cpu->regs[0] = cpu->regs[codeArr[++(*elnum)]];
        arg->argval = &(cpu->regs[codeArr[*elnum]]);
        }

    if (com & ARG_FORMAT_IMMED)
        {
        cpu->regs[0] += codeArr[++(*elnum)] * multiplier;
        }

    if (com & ARG_FORMAT_RAM)
        {
        arg->argval = &(cpu->ram[(int) (cpu->regs[0] / multiplier)]);
        }

    else if (com & ARG_FORMAT_IMMED)
        {
        arg->argval = &(cpu->regs[0]);
        }

    return NO_ERROR;
    }

int unknownvm(int linenum)
    {
    printf("mistake on element %d\n", linenum);
    return 0;
    }
