#ifndef _UTIL_H_
#define _UTIL_H_

#include "global.h"

unsigned int _binary_to_int(const char *str);
unsigned int _hex_to_int(const char *str);
char *_int_to_binary(unsigned int val, char **start);
unsigned int _nearest_power_of_two(unsigned int val);

#endif