#ifndef GLOBAL_H_
#define GLOBAL_H_

// Keep this file SPARSE!
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

// Keep this list even sparser :P
#define DISALLOW_COPY_AND_ASSIGN(TypeName)\
TypeName(const TypeName&);\
void operator=(const TypeName&)

#endif
