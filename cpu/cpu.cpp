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
    #include "commands.txt"
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
     CODEARR_ERROR = 1};

int unknownvm(int linenum);


error_t cpuCreate(Cpu* cpu)
    {
    MY_ASSERT_HARD(cpu);

    Stack* stk = (Stack*) calloc(1, sizeof(Stack)); 
    stackCreate(stk);
    cpu->stk = stk;

    Stack* callStack = (Stack*) calloc(1, sizeof(Stack));
    stackCreate(callStack);
    cpu->callStack = callStack;

    cmdel_t ram[ramlen] = {};
    cpu->ram = ram;
    return NO_ERROR;
    }

cmdel_t* setvmbuf(char filename_i[])
    {
    MY_ASSERT_HARD(filename_i);

    FILE *fp = fileopenerRB(filename_i);
    size_t filelen = fileLen(filename_i);

    cmdel_t* codeArr = (cmdel_t*) calloc(filelen, sizeof(*codeArr));
    fread(codeArr, sizeof(*codeArr), filelen, fp);    

    // for (size_t i = 0; i < filelen + 1; i++)
    //     printf("%d ", codeArr[i]);

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

    while (codeArr[elnum] != CMD_HLT)
        {
        // printf("%d\n", cpu->stk->dataptr[cpu->stk->size - 1]);
        com = codeArr[elnum];
        switch (com & ~(7 << 5))
            {

            #include "commands.txt"

            default: 
                unknownvm(elnum);
                return CODEARR_ERROR;
                break;
            }

        elnum++;
        }

    #undef DEF_CMD
    return errno;
    }


int unknownvm(int linenum)
    {
    printf("mistake on element %d\n", linenum);
    return 0;
    }
