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
error_t SetArg(Text* codeStruct, cmdel_t* codeArray, size_t curLine, cmdel_t code, size_t* position, size_t cmdLen, Label* labels, size_t* freelabel, bool isscndRun);
error_t parseLine(cmdel_t* codeArray, Text* codeStruct, size_t curLine, size_t* position, bool scndRun, Label* labels, size_t* freelabel);
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

    for (size_t curLine = 0; curLine < codeStruct->nLines; curLine++)
        {
        parseLine(codeArray, codeStruct, curLine, &position, scndrun, labels, freelabel);    
        }
    *arrLen = position;
    return NO_ERROR;
    }


error_t compile(char* fpname, cmdel_t* codeArray)
    {

    FILE* fpdest = fileopenerW("notbin.txt");
    FILE* fpdestbin = fileopenerWB("assemblerfile.bin");
    Text codeStruct = setbuf(fpname);

    Label labels[MaxLabelNum] = {};
    size_t freelabel = 0;

    size_t arrLen = 0;

    oneRun(codeArray, &codeStruct, FIRST_RUN, labels, &freelabel, &arrLen);
    printf("hyu3\n\n\n");
    oneRun(codeArray, &codeStruct, SECOND_RUN, labels, &freelabel, &arrLen);
    
    for (size_t i = 0; i < arrLen; i++)
        {
        printf("%d ", codeArray[i]);
        }

    writeInFile(fpdest, codeArray, arrLen);
    writeInFileBin(fpdestbin, codeArray, arrLen);   
    printf("lab = |%s|, %d\n", labels->label, labels->codepos);
    destruct(&codeStruct, codeArray);
    return errno;
    }

error_t parseLine(cmdel_t* codeArray, Text* codeStruct, size_t curLine, size_t* position, bool isscndRun, Label* labels, size_t* freelabel)
    {
    char* lineptr = codeStruct->lines[curLine].linePtr;
    if (*lineptr == '\n' || *lineptr == '\r')
        return NO_ERROR;
    #define DEF_CMD(name, num, isarg, ...)                                                                                      \
        if (strcasecmp(cmd, #name) == 0)                                                                                        \
            {                                                                                                                   \
            if (isarg)                                                                                                          \
                SetArg(codeStruct, codeArray, curLine, CMD_##name, position, cmdlen, labels, freelabel, isscndRun);             \
            else                                                                                                                \
                emitCodeNoArg(codeArray, position, CMD_##name, isscndRun);                                                      \
            }                                                                                                                   \
        else

    char* endptr = (char*)strchr(lineptr, '\n');
    if (endptr)
        *endptr = '\0';

    char* commentptr = (char*)strchr(lineptr, ';');
    if (commentptr)
        *commentptr = '\0';


    char* labelend = strchr(codeStruct->lines[curLine].linePtr, ':');
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
    if (sscanf(codeStruct->lines[curLine].linePtr, "%s%n", cmd, &cmdlen) != 1)
        return SYNTAX_ERROR;
    printf("\n<%s>\n", cmd);

    #include "commands.txt"

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


error_t SetArg(Text* codeStruct, cmdel_t* codeArray, size_t curLine, cmdel_t code, size_t* position, size_t cmdLen, Label* labels, size_t* freelabel, bool isscndRun)
    {
    Arg arg = {};
 
    const char* strtptr = codeStruct->lines[curLine].linePtr + cmdLen;
    const char* stBracketPtr = strchr(strtptr, '[');
    const char* clBracketPtr = strchr(strtptr, ']');

    printf("%s\n", strtptr);
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

    cmdel_t reg = 0;
    if (sscanf(strtptr, " r%cx %n", &reg, &scanlen) == 1)
        {
        if (scanlen >= 3)
            {
            arg.argFormat |= ARG_FORMAT_REG;
            int regNum = reg - 'a' + 1;
            // encodeReg(&regNum, reg);
            arg.regnum = regNum;
            strtptr += scanlen;
            }
        }

    int immed = 0;
    if (*strtptr == '+')
        strtptr += 1;
    if (sscanf(strtptr, " %"prELEM" %n", &immed, &scanlen) == 1)
        {
        arg.argFormat |= ARG_FORMAT_IMMED;
        arg.immed = immed;
        strtptr += scanlen;
        }
    printf("cd = %d\n", arg.argFormat);
    if (arg.argFormat == 0)
        {
        char* label = (char*) calloc(codeStruct->lines[curLine].length, sizeof(*label));
        if (sscanf(strtptr, "%s%n", label, &scanlen) == 1)
            {
            strtptr += scanlen;
            if (isscndRun)
                {
                for (size_t i = 0; i < *freelabel; i++)
                    if (strcmp(label, labels[i].label) == 0)
                        {
                        printf("code = %d\n", code);
                        emitCodeLabel(codeArray, position, code, labels[i].codepos, isscndRun);
                        return NO_ERROR;
                        }
                }
            else
                {
                (*position) += 2; 
                return SYNTAX_ERROR;
                }
            }
        }
    code |= arg.argFormat;
    emitCode(&arg, codeArray, position, code, isscndRun);
    return errno;
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