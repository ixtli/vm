#ifndef GLOBAL_H_
#define GLOBAL_H_

// Keep this file SPARSE!
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

// Keep this list even sparser :P
#define DISALLOW_COPY_AND_ASSIGN(TypeName)\
TypeName(const TypeName&);\
void operator=(const TypeName&)

// Size of the machine's registers
typedef unsigned int reg_t;

// Type to keep track of cycles, this should be as big as the host machine
// can support
typedef size_t cycle_t;

// Calculate the machines word length (PROTIP: Keep this 32 bits)
#define kRegSize   sizeof(reg_t)

#endif
