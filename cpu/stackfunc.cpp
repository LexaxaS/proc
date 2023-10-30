#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "tools.hpp"
#include "stackfunc.hpp"


error_t _stackRealloc(Stack* stk, size_t capacity)
    {
    stackDump_t(stk);

    stk->dataptr = (elem_t*) (realloc((void*)stk->dataptr - sizeof(chicken_t), capacity * sizeof(elem_t) + 2 * sizeof(chicken_t)) + sizeof(chicken_t));
    
    stk->capacity = capacity;
    for (size_t i = stk->size + 1; i < stk->capacity; i++)
        {
        stk->dataptr[i] = POISON;
        }
    *(elem_t*)((void*)stk->dataptr + sizeof(elem_t) * stk->capacity) = CHICKEN;

    stackDump_t(stk);
    return NoMistakes;
    }

int ptrverify(elem_t* ptr)
    {
    if (ptr)
        return 0;
    else
        return 1;
    }

error_t stackCreate(Stack* stk)
    {
    assert(stk);
    stk->capacity = 8;
    stk->size = 0;
    stk->dataptr = (elem_t*) (calloc(2 * sizeof(chicken_t) + stk->capacity * sizeof(elem_t), 1) + sizeof(chicken_t));
    for (size_t i = stk->size; i < stk->capacity; i++)
        stk->dataptr[i] = POISON;
    stackDump_t(stk);
    return NoMistakes;
    }

error_t stackDel(Stack* stk)
    {
    stackDump_t(stk);

    elem_t* dta = stk->dataptr;
    for (size_t i = 0; i < stk->size; i++)
        dta[i] = POISON;
    free((void*)((char*) stk->dataptr - 8));
    return NoMistakes;
    }


error_t stackPush(Stack* stk, elem_t value)
    {
    stackDump_t(stk);

    if (stk->capacity == stk->size)
        _stackRealloc(stk, stk->capacity * 2);
    stk->dataptr[stk->size++] = value;
    stackDump_t(stk);

    return NoMistakes;
    }

elem_t stackPop(Stack* stk)
    {
    stackDump_t(stk);

    if (stk->size * 4 <= stk->capacity & stk->capacity > 8)
        _stackRealloc(stk, stk->capacity / 2);
    elem_t value = *(elem_t*)((void*)stk->dataptr + sizeof(elem_t) * (stk->size - 1));
    *(elem_t*)((void*)stk->dataptr + sizeof(elem_t) * (stk->size - 1)) = POISON;
    stk->size--;

    stackDump_t(stk);
    return value;
    }


error_t verifyStack(Stack* stk)
    {
    error_t error = 0;

    if (stk->size < 0)                                                      {error |= SizeNegative;}    
    if (stk->size > stk->capacity)                                          {error |= SizeTooBig;}    
    if (stk->capacity < 0)                                                  {error |= CapacityNegative;}    
    if (ptrverify(stk->dataptr))                                            {error |= PointerNoValid;}   
    if (stk->capacity >= MAXCAPACITY)                                       {error |= ArrayTooBig;}

    return error;
    }


const char* errorStr(int code)
    {
    #define CASE_(code) case code: return #code;

    switch (code)
        {
        CASE_ (SizeNegative)
        CASE_ (SizeTooBig)
        CASE_ (CapacityNegative)
        CASE_ (PointerNoValid)
        }

    return "**UNKNOWN**";

    #undef CASE_
    }



error_t stackDump(Stack* stk, error_t error, char* vfile, const char* vfunc, int vline, char* stackname)
    {
    FILE* logfile = stderr;
    if (PRINTLOGS != "stderr")
        logfile = fileopenerW(PRINTLOGS);

    fprintf(logfile, "\n\nStack[%p] \"%s\" from file: [%s] (%d) from function: [%s]\n", stk, stackname, vfile, vline, vfunc);
    fprintf(logfile, "Error codes: ");
    size_t i = 10;
    bool structerr = false;
    if ((StackHashWrong & error) > 0)
        {
        structerr = true;
        }
    while (i != 0)
        {
        if (((1 << i) & error) > 0)
            {
            fprintf(logfile, "#%d (%s) ", 2 << (i - 1), errorStr(2 << (i - 1)));
            }
        i -= 1;
        }
    fprintf(logfile, "\n");
    fprintf(logfile, "    {\n");
    fprintf(logfile, "    size = %d\n", stk->size);
    fprintf(logfile, "    capacity = %d\n", stk->capacity);
    fprintf(logfile, "    data[%p]\n", stk->dataptr);

    if (structerr == false)
        {
        fprintf(logfile, "        {\n");

        if (stk->capacity >= max_elements_dump)
            {
            for (size_t i = 0; i < 100; i++)
                {
                if (i < stk->size)
                    fprintf(logfile, "        *[%d] = <%"prELEM_T">\n", i, (stk->dataptr[i]));
                else
                    fprintf(logfile, "         [%d] = <%"prELEM_T"> (POISON)\n", i, stk->dataptr[i]);
                } 
            fprintf(logfile, "         ...\n");
            fprintf(logfile, "         %d more elements\n", stk->capacity - 128);
            fprintf(logfile, "         ...\n");
            for (size_t i = stk->capacity - 28; i < stk->capacity; i++)
                {
                if (i < stk->size)
                    fprintf(logfile, "        *[%d] = <%"prELEM_T">\n", i, (stk->dataptr[i]));
                else
                    fprintf(logfile, "         [%d] = <%"prELEM_T"> (POISON)\n", i, stk->dataptr[i]);
                }        
            }
        else
            for (size_t i = 0; i < stk->capacity; i++)
                {
                if (i < stk->size)
                    fprintf(logfile, "        *[%d] = <%"prELEM_T">\n", i, (stk->dataptr[i]));
                else
                    fprintf(logfile, "         [%d] = <%"prELEM_T"> (POISON)\n", i, stk->dataptr[i]);
                }
        fprintf(logfile, "        }\n");
        }
    else
        {
        fprintf(logfile, "    {Error in the structure, its dangerous ro print the array}");
        }
    fprintf(logfile, "    }\n");
    return error;
    }


