#ifndef STACKFUNC_HPP
#define STACKFUNC_HPP

#include <stdio.h>
#include "stackfunc_internal.hpp"

typedef unsigned long long error_t;
typedef unsigned long long hash_t;
typedef size_t chicken_t;
typedef int cmdel_t;
typedef cmdel_t reg_t; 

const size_t max_elements_dump = 256;

#define INT 0
#define SIZE_T 1
#define DOUBLE 2
#define STR 3
#define CHAR 4

#define TYPE INT

#if (TYPE == INT)
    typedef int elem_t;
    #define prELEM_T "d"
#endif

#if (TYPE == SIZE_T)
    typedef size_t elem_t;
    #define prELEM_T "llu"
#endif

#if (TYPE == DOUBLE)
    typedef double elem_t;
    #define prELEM_T "lg"
#endif

#if (TYPE == STR)
    typedef char* elem_t;
    #define prELEM_T "s"
#endif


#if (TYPE == CHAR)
    typedef char elem_t;
    #define prELEM_T "c"
#endif

#ifndef TYPE
    typedef int elem_t;
    #define prELEM_T "d"
#endif


struct Stack
    {
    elem_t* dataptr;
    size_t capacity;
    size_t size;
    };



#define stackDumpTest_t(stk, err)                                                               \
do                                                                                              \
{                                                                                               \
    printf("\nTEST DUMP!!");                                                                    \
    stackDump(stk, err, __FILE__, __func__, __LINE__, #stk);                                    \
} while (0)         


#define stackDump_t(stk)                                                                                    \
do                                                                                                          \
{                                                                                                           \
    error_t error = verifyStack(stk);                                                                       \
    if (error != 0) {stackDump(stk, error, __FILE__, __func__, __LINE__, #stk); abort(); return error;};    \
} while (0);    

enum ERRORS 
    {LeftChickenStackDied = 1,
     RightChickenStackDied = 2,
     SizeNegative = 4,
     SizeTooBig = 8, 
     CapacityNegative = 16,
     PointerNoValid = 32,
     LeftChickenArrayDied = 64,
     RightChickenArrayDied = 128,
     ArrayTooBig = 256,
     StackHashWrong = 512,
     ArrayHashWrong = 1024,
     NoMistakes = 0};



hash_t hashCount(void* source, size_t sizeb);
error_t stackCreate(Stack* stk);
error_t stackPush(Stack* stk, elem_t value);
error_t stackDel(Stack* stk);
elem_t stackPop(Stack* stk);
error_t stackDump(Stack* stk, error_t error, char* vfile, const char* vfunc, int vline, char* stackname);
error_t verifyStack(Stack* stk);

#endif