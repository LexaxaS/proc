#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "assembler.hpp"
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
     SYNTAX_ERROR = 1,
     OVER_MAX_LABELS = 2};

enum RunNums
    {FIRST_RUN = 0,
     SECOND_RUN = 1};

struct Arg
    {
    cmdel_t immed;
    cmdel_t regnum;
    cmdel_t argFormat;
    };

struct Label
    {
    char* label;
    cmdel_t codepos;
    };


error_t _writeInFile(FILE* filedest, cmdel_t* codeArray, size_t arrLen);
error_t _writeInFileBin(FILE* filedest, cmdel_t* codeArray, size_t arrLen);

error_t _oneRun(cmdel_t* codeArray, Text* codeStruct, bool scndrun, Label* labels, size_t* freelabel, size_t* arrLen);
error_t _parseLine(cmdel_t* codeArray, String* line, size_t* position, bool isscndRun, Label* labels, size_t* freelabel);
error_t _labelprocess(Label* labels, size_t* freelabel, char* labelstrt, char* labelend, size_t position);
error_t _SetArg(String* line, cmdel_t* codeArray, cmdel_t code, size_t* position, size_t cmdLen, Label* labels, size_t* freelabel, bool isscndRun);

error_t _ramSearch(const char** strtptr, Arg* arg);
error_t _regSearch(const char** strtptr, Arg* arg);
error_t _immedSearch(const char** strtptr, Arg* arg);
error_t _labelSearch(const char** strtptr, size_t linelen, Arg* arg, Label* labels, size_t* freelabel, bool isscndRun, size_t* pos);

error_t _destruct(Text* codeStruct, cmdel_t* codeArr);

error_t _encodeReg(int *regNum, char* reg)
    {
    MY_ASSERT_HARD(regNum);
    MY_ASSERT_HARD(reg);

    if (strcmp(reg, "rax") == 0)
        *regNum = REG_RAX;
    else if (strcmp(reg, "rbx") == 0)
        *regNum = REG_RBX;
    else if (strcmp(reg, "rcx") == 0)
        *regNum = REG_RCX;
    else if (strcmp(reg, "rdx") == 0)
        *regNum = REG_RDX;

    return NO_ERROR;
    }

error_t _emitCode(Arg* arg, cmdel_t* codeArr, size_t* pos, cmdel_t code, bool isscndRun)
    { 
    MY_ASSERT_HARD(arg);
    MY_ASSERT_HARD(codeArr);
    MY_ASSERT_HARD(pos)

    if (isscndRun)
        codeArr[(*pos)++] = code;
    else
        (*pos)++;

    if (arg->argFormat & ARG_FORMAT_REG)
        {
        if (isscndRun)
            codeArr[(*pos)++] = arg->regnum;
        else
            (*pos)++;
        }

    if (arg->argFormat & ARG_FORMAT_IMMED)
        {
        if (isscndRun)
            codeArr[(*pos)++] = arg->immed;
        else
            (*pos)++;
        }

    return NO_ERROR;
    }

error_t _emitCodeNoArg(cmdel_t* codeArr, size_t* pos, cmdel_t code, bool isscndRun)
    {
    MY_ASSERT_HARD(codeArr);
    MY_ASSERT_HARD(pos);

    if (isscndRun)
        codeArr[(*pos)++] = code;
    else
        (*pos)++;

    return NO_ERROR;
    }

error_t _emitCodeLabel(cmdel_t* codeArr, size_t* pos, cmdel_t code, cmdel_t label, bool isscndRun)
    {
    MY_ASSERT_HARD(codeArr);
    MY_ASSERT_HARD(pos);

    if (isscndRun)
        {
        codeArr[(*pos)++] = code;
        codeArr[(*pos)++] = label;
        }
    else
        {
        (*pos) += 2;
        }

    return NO_ERROR;
    }

error_t compile(char* fpname, char* fpdestname)
    {
    MY_ASSERT_HARD(fpname);
    MY_ASSERT_HARD(fpdestname);

    FILE* fpdest = fileopenerW("notbin.txt");
    FILE* fpdestbin = fileopenerWB(fpdestname);

    Text codeStruct = setbuf(fpname);
    cmdel_t* codeArray = (cmdel_t*) calloc(CodeArrayMaxLen, sizeof(*codeArray));

    Label labels[MaxLabelNum] = {};
    size_t freelabel = 0;

    size_t arrLen = 0;

    _oneRun(codeArray, &codeStruct, FIRST_RUN, labels, &freelabel, &arrLen);

    _oneRun(codeArray, &codeStruct, SECOND_RUN, labels, &freelabel, &arrLen);
    
    for (size_t i = 0; i < arrLen; i++)
        {
        printf("%d ", codeArray[i]);
        }

    _writeInFile(fpdest, codeArray, arrLen);
    _writeInFileBin(fpdestbin, codeArray, arrLen);

    _destruct(&codeStruct, codeArray);
    return NO_ERROR;
    }

error_t _oneRun(cmdel_t* codeArray, Text* codeStruct, bool scndrun, Label* labels, size_t* freelabel, size_t* arrLen)
    {
    MY_ASSERT_HARD(codeArray);
    MY_ASSERT_HARD(codeStruct);
    MY_ASSERT_HARD(labels);
    MY_ASSERT_HARD(freelabel);
    MY_ASSERT_HARD(arrLen);

    size_t position = 0;

    for (size_t curLine = 0; curLine < codeStruct->nLines - 1; curLine++)
        {
        _parseLine(codeArray, &codeStruct->lines[curLine], &position, scndrun, labels, freelabel);    
        // printf("pos = %d\n", position);
        }

    *arrLen = position;
    return NO_ERROR;
    }

error_t _parseLine(cmdel_t* codeArray, String* line, size_t* position, bool isscndRun, Label* labels, size_t* freelabel)
    {
    MY_ASSERT_HARD(codeArray);
    MY_ASSERT_HARD(line);
    MY_ASSERT_HARD(position);
    MY_ASSERT_HARD(labels);
    MY_ASSERT_HARD(freelabel);

    char* lineptr = line->linePtr;
    if (*lineptr == '\n' || *lineptr == '\r')
        return NO_ERROR;

    char* commentptr = (char*)strchr(lineptr, ';');
    if (commentptr)
        *commentptr = '\0';

    char* labelend = strchr(line->linePtr, ':');
    if (labelend)
        {
        if (isscndRun)
            return NO_ERROR;
        else
            _labelprocess(labels, freelabel, lineptr, labelend, *position);
        return NO_ERROR;
        }

    char cmd[5] = "";
    size_t cmdlen = 0;


    #define DEF_CMD(name, num, isarg, ...)                                                                                      \
        if (strcasecmp(cmd, #name) == 0)                                                                                        \
            {                                                                                                                   \
            if (isarg)                                                                                                          \
                _SetArg(line, codeArray, CMD_##name, position, cmdlen, labels, freelabel, isscndRun);                           \
            else                                                                                                                \
                _emitCodeNoArg(codeArray, position, CMD_##name, isscndRun);                                                     \
            }                                                                                                                   \
        else


    if (sscanf(line->linePtr, "%s%n", cmd, &cmdlen) != 1)
        return SYNTAX_ERROR;

    // printf("\n<%s>\n", cmd);

    #include "../commands.txt"

    /*else*/  printf("unknown\n");
    
    #undef DEF_CMD
    return NO_ERROR;
    }

error_t _labelprocess(Label* labels, size_t* freelabel, char* labelstrt, char* labelend, size_t position)
    {
    MY_ASSERT_HARD(labels);
    MY_ASSERT_HARD(freelabel);
    MY_ASSERT_HARD(labelstrt);
    MY_ASSERT_HARD(labelend);

    if (*freelabel > MaxLabelNum - 1)
        return OVER_MAX_LABELS;

    while (isspace(*labelstrt) && labelstrt < labelend)
        labelstrt++;
    
    size_t labellen = labelend - labelstrt;
    if (labellen < 1)
        return SYNTAX_ERROR;
    
    char* label = (char*) calloc(labellen + 1, sizeof(*label));
    strncpy(label, labelstrt, labellen);
    label[labellen] = '\0';
    
    labels[*freelabel].label = label;
    labels[*freelabel].codepos = position;
    (*freelabel)++;

    return NO_ERROR;
    }


error_t _SetArg(String* line, cmdel_t* codeArray, cmdel_t code, size_t* position, size_t cmdLen, Label* labels, size_t* freelabel, bool isscndRun)
    {
    MY_ASSERT_HARD(line);
    MY_ASSERT_HARD(codeArray);
    MY_ASSERT_HARD(position);
    MY_ASSERT_HARD(labels);
    MY_ASSERT_HARD(freelabel);

    Arg arg = {};
    const char* strtptr = line->linePtr + cmdLen;
    size_t scanlen = 0;

    _ramSearch(&strtptr, &arg);
    _regSearch(&strtptr, &arg);
    _immedSearch(&strtptr, &arg);
    _labelSearch(&strtptr, line->length, &arg, labels, freelabel, isscndRun, position);

    code |= arg.argFormat;
    _emitCode(&arg, codeArray, position, code, isscndRun);

    return NO_ERROR;
    }

error_t _ramSearch(const char** strtptr, Arg* arg)
    {
    const char* stBracketPtr = strchr(*strtptr, '[');
    const char* clBracketPtr = strchr(*strtptr, ']');

    if (stBracketPtr or clBracketPtr)
        {
        if (!stBracketPtr || !clBracketPtr)
            {
            return SYNTAX_ERROR;
            }
        *strtptr = stBracketPtr + 1;
        arg->argFormat |= ARG_FORMAT_RAM;
        }
    return NO_ERROR;
    }

error_t _regSearch(const char** strtptr, Arg* arg)
    {
    MY_ASSERT_HARD(strtptr);
    MY_ASSERT_HARD(*strtptr);
    MY_ASSERT_HARD(arg);

    size_t scanlen = 0;
    cmdel_t reg = 0;

    if (sscanf(*strtptr, " r%cx %n", &reg, &scanlen) == 1)
        {
        if (scanlen >= 3)
            {
            arg->argFormat |= ARG_FORMAT_REG;
            int regNum = reg - 'a' + 1;
            // encodeReg(&regNum, reg);
            arg->regnum = regNum;
            *strtptr += scanlen;
            }
        }

    return NO_ERROR;
    }

error_t _immedSearch(const char** strtptr, Arg* arg)
    {
    MY_ASSERT_HARD(strtptr);
    MY_ASSERT_HARD(*strtptr);
    MY_ASSERT_HARD(arg);

    size_t scanlen = 0;
    cmdel_t immed = 0;

    if (**strtptr == '+')
        *strtptr += 1;

    if (sscanf(*strtptr, " %"prELEM" %n", &immed, &scanlen) == 1)
        {
        arg->argFormat |= ARG_FORMAT_IMMED;
        arg->immed = immed;
        *strtptr += scanlen;
        }

    return NO_ERROR;
    }

error_t _labelSearch(const char** strtptr, size_t linelen, Arg* arg, Label* labels, size_t* freelabel, bool isscndRun, size_t* pos)
    {
    MY_ASSERT_HARD(strtptr);
    MY_ASSERT_HARD(*strtptr);
    MY_ASSERT_HARD(arg);
    MY_ASSERT_HARD(labels);
    MY_ASSERT_HARD(freelabel);
    MY_ASSERT_HARD(pos);

    size_t scanlen = 0;
    char* label = (char*) calloc(linelen, sizeof(*label));

    if (sscanf(*strtptr, "%s%n", label, &scanlen) == 1 and *label != ']')
        {
        *strtptr += scanlen;

        for (size_t i = 0; i < *freelabel; i++) 
            {
            if (strcmp(label, labels[i].label) == 0)
                {
                arg->argFormat |= ARG_FORMAT_IMMED;
                arg->immed += labels[i].codepos;
                return NO_ERROR;
                }
            }

        if (isscndRun)
            return SYNTAX_ERROR;
        else
            *pos += 1;
        }

    return NO_ERROR;
    }

error_t _writeInFile(FILE* filedest, cmdel_t* codeArray, size_t arrLen)
    {
    MY_ASSERT_HARD(filedest);
    MY_ASSERT_HARD(codeArray);

    for (size_t i = 0; i < arrLen; i++)
        {
        fprintf(filedest, "%d :   %d\n", i + 1, codeArray[i]); 
        }

    fclose(filedest);
    return errno;
    }

error_t _writeInFileBin(FILE* filedest, cmdel_t* codeArray, size_t arrLen)
    {
    MY_ASSERT_HARD(filedest);
    MY_ASSERT_HARD(codeArray);

    fwrite(codeArray, sizeof(*codeArray), arrLen, filedest);

    for (size_t i = 0; i < arrLen; i++)
        printf("%d ", codeArray[i]);
    printf("\n");
    
    fclose(filedest);
    return errno;
    }

error_t _destruct(Text* codeStruct, cmdel_t* codeArr)
    {
    MY_ASSERT_HARD(codeStruct);
    MY_ASSERT_HARD(codeArr);

    free(codeStruct->bufPtr);
    free(codeStruct->lines);
    free(codeArr);
    return errno;
    }