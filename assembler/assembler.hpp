#ifndef ASSEMBLER_HPP
#define ASSEMBLER_HPP

#include <stdio.h>

typedef int cmdel_t;
typedef unsigned long long error_t;

#define prELEM "d"

const size_t CodeArrayMaxLen = 1000;
const size_t MaxLabelNum = 100;

error_t compile(char* fpname, char* fpdestname);

#endif