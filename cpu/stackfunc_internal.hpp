#ifndef STACKFUNC_INTERNAL_HPP
#define STACKFUNC_INTERNAL_HPP

#define CHICKEN 0xBADDED32
#define POISON 123456789
#define MAXCAPACITY (SIZE_MAX / 2)
// #define PRINTLOGS "logs.txt"

#ifndef PRINTLOGS
#define PRINTLOGS "stderr"
#endif

#endif