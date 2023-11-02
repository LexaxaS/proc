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


error_t writeInFile(FILE* filedest, cmdel_t* codeArray, size_t arrLen);
error_t writeInFileBin(FILE* filedest, cmdel_t* codeArray, size_t arrLen);

error_t SetArg(String* line, cmdel_t* codeArray, cmdel_t code, size_t* position, size_t cmdLen, Label* labels, size_t* freelabel, bool isscndRun);
error_t parseLine(cmdel_t* codeArray, String* line, size_t* position, bool isscndRun, Label* labels, size_t* freelabel);

error_t regSearch(const char** strtptr, Arg* arg);
error_t immedSearch(const char** strtptr, Arg* arg);
error_t labelSearch(const char** strtptr, size_t linelen, Arg* arg, Label* labels, size_t* freelabel, bool isscndRun, size_t* pos);

error_t labelprocess(Label* labels, size_t* freelabel, char* labelstrt, char* labelend, size_t position);

error_t destruct(Text* codeStruct, cmdel_t* codeArr);

error_t encodeReg(int *regNum, char* reg)
    {
    if (strcmp(reg, "rax") == 0)
        *regNum = REG_RAX;
    else if (strcmp(reg, "rbx") == 0)
        *regNum = REG_RBX;
    else if (strcmp(reg, "rcx") == 0)
        *regNum = REG_RCX;
    else if (strcmp(reg, "rdx") == 0)
        *regNum = REG_RDX;
    return errno;
    }

error_t emitCode(Arg* arg, cmdel_t* codeArr, size_t* pos, cmdel_t code, bool isscndRun)
    { 
    printf("arg, %d\n", isscndRun);
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
    return errno;
    }

error_t emitCodeNoArg(cmdel_t* codeArr, size_t* pos, cmdel_t code, bool isscndRun)
    {
    printf("noarg, %d\n", isscndRun);
    if (isscndRun)
        codeArr[(*pos)++] = code;
    else
        (*pos)++;
    return errno;
    }

error_t emitCodeLabel(cmdel_t* codeArr, size_t* pos, cmdel_t code, cmdel_t label, bool isscndRun)
    {
    printf("labemit, %d\n", isscndRun);
    if (isscndRun)
        {
        codeArr[(*pos)++] = code;
        codeArr[(*pos)++] = label;
        }
    else
        {
        (*pos) += 2;
        }
    return errno;
    }

error_t oneRun(cmdel_t* codeArray, Text* codeStruct, bool scndrun, Label* labels, size_t* freelabel, size_t* arrLen)
    {
    size_t position = 0;

    for (size_t curLine = 0; curLine < codeStruct->nLines - 1; curLine++)
        {
        parseLine(codeArray, &codeStruct->lines[curLine], &position, scndrun, labels, freelabel);    
        printf("pos = %d\n", position);
        }
    *arrLen = position;
    return NO_ERROR;
    }

error_t compile(char* fpname, char* fpdestname)
    {

    FILE* fpdest = fileopenerW("notbin.txt");
    FILE* fpdestbin = fileopenerWB(fpdestname);

    Text codeStruct = setbuf(fpname);
    cmdel_t* codeArray = (cmdel_t*) calloc(CodeArrayMaxLen, sizeof(*codeArray));

    Label labels[MaxLabelNum] = {};
    size_t freelabel = 0;

    size_t arrLen = 0;

    oneRun(codeArray, &codeStruct, FIRST_RUN, labels, &freelabel, &arrLen);

    oneRun(codeArray, &codeStruct, SECOND_RUN, labels, &freelabel, &arrLen);
    
    for (size_t i = 0; i < arrLen; i++)
        {
        printf("%d ", codeArray[i]);
        }

    writeInFile(fpdest, codeArray, arrLen);
    writeInFileBin(fpdestbin, codeArray, arrLen);
    size_t i = 0;
    printf("\n");
    while (labels[i].codepos != 0)   
        {
        printf("lab = |%s|, %d\n", labels[i].label, labels[i].codepos);
        i++;
        }
    destruct(&codeStruct, codeArray);
    return errno;
    }

error_t parseLine(cmdel_t* codeArray, String* line, size_t* position, bool isscndRun, Label* labels, size_t* freelabel)
    {
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
            labelprocess(labels, freelabel, lineptr, labelend, *position);
        return NO_ERROR;
        }

    char cmd[5] = "";
    size_t cmdlen = 0;


    #define DEF_CMD(name, num, isarg, ...)                                                                                      \
        if (strcasecmp(cmd, #name) == 0)                                                                                        \
            {                                                                                                                   \
            if (isarg)                                                                                                          \
                SetArg(line, codeArray, CMD_##name, position, cmdlen, labels, freelabel, isscndRun);                            \
            else                                                                                                                \
                emitCodeNoArg(codeArray, position, CMD_##name, isscndRun);                                                      \ 
            }                                                                                                                   \
        else


    if (sscanf(line->linePtr, "%s%n", cmd, &cmdlen) != 1)
        return SYNTAX_ERROR;
    printf("\n<%s>\n", cmd);

    #include "../commands.txt"

    /*else*/  printf("unknown\n");
    
    #undef DEF_CMD
    return errno;
    }

error_t labelprocess(Label* labels, size_t* freelabel, char* labelstrt, char* labelend, size_t position)
    {
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


error_t SetArg(String* line, cmdel_t* codeArray, cmdel_t code, size_t* position, size_t cmdLen, Label* labels, size_t* freelabel, bool isscndRun)
    {
    Arg arg = {};
    // printf("|%s|, %d\n", line->linePtr, line->length);
    const char* strtptr = line->linePtr + cmdLen;
    const char* stBracketPtr = strchr(strtptr, '[');
    const char* clBracketPtr = strchr(strtptr, ']');

    if (stBracketPtr or clBracketPtr)
        {
        if (!stBracketPtr || !clBracketPtr)
            {
            return SYNTAX_ERROR;
            }
        strtptr = stBracketPtr + 1;
        arg.argFormat |= ARG_FORMAT_RAM;
        }

    size_t scanlen = 0;
    regSearch(&strtptr, &arg);
    immedSearch(&strtptr, &arg);
    labelSearch(&strtptr, line->length, &arg, labels, freelabel, isscndRun, position);

    code |= arg.argFormat;
    emitCode(&arg, codeArray, position, code, isscndRun);
    return errno;
    }

error_t regSearch(const char** strtptr, Arg* arg)
    {
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

error_t immedSearch(const char** strtptr, Arg* arg)
    {
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

error_t labelSearch(const char** strtptr, size_t linelen, Arg* arg, Label* labels, size_t* freelabel, bool isscndRun, size_t* pos)
    {
    size_t scanlen = 0;
    char* label = (char*) calloc(linelen, sizeof(*label));
    if (sscanf(*strtptr, "%s%n", label, &scanlen) == 1 and *label != ']')
        {
        // printf("                    label = |%s|\n", label);
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

error_t writeInFile(FILE* filedest, cmdel_t* codeArray, size_t arrLen)
    {
    printf("\n\n\n");

    for (size_t i = 0; i < arrLen; i++)
        {
        fprintf(filedest, "%d :   %d\n", i + 1, codeArray[i]); 
        }
    fclose(filedest);
    return errno;
    }

error_t writeInFileBin(FILE* filedest, cmdel_t* codeArray, size_t arrLen)
    {
    printf("\nbin\n\n");

    fwrite(codeArray, sizeof(*codeArray), arrLen, filedest);
    for (size_t i = 0; i < arrLen; i++)
        printf("<%d> ", codeArray[i]);
    fclose(filedest);
    return errno;
    }

error_t destruct(Text* codeStruct, cmdel_t* codeArr)
    {
    free(codeStruct->bufPtr);
    free(codeStruct->lines);
    free(codeArr);
    return errno;
    }