#include <string.h>
#include "includes/util.h"

unsigned int _binary_to_int(const char *str)
{
    size_t length = strlen(str);
    unsigned int ret = 0;
    
    for (int i = 0; i < length; i++)
    {
        if (str[i] != '0')
            ret |= (1 << (length - 1 - i));
    }
    
    return (ret);
}

unsigned int _hex_to_int(const char *str)
{
    unsigned int ret;
    
    size_t len = strlen(str);
    size_t index = 0;
    
    // check to see if they started with '0x'
    if (len > 1)
    {
        if (str[0] == '0' && str[1] == 'x')
        {
            // if so, skip it
            index = 2;
        }
    }
    
    // parse
    unsigned int charval = 0;
    for (int i = 0;index < len; index++)
    {
        if (str[index] > 47 && str[index] < 58 )
        {
            // [0-9]
            charval = str[index] - 48;
        } else if (str[index] > 64 && str[index] < 91) {
            // [A-Z]
            charval = str[index] - 65 + 10;
            
        } else if (str[index] > 96 && str[index] < 123) {
            // [a-z]
            charval = str[index] - 97 + 10;
        }
        // i is our index into the returned number
        ret |= (charval << (4 * i));
        i++;
    }
    
    return (ret);
}

// Remember to free the result!
char *_int_to_binary(unsigned int val, char **start)
{
    size_t size = sizeof(unsigned int) * 8;
    // dont forget \0
    char *ret = (char *)malloc(sizeof(char) * size + 1);
    size_t loc = 0;
    
    for (int i = 0; i < size; i++)
    {
        if (val & (1 << i))
        {
            ret[size-1-i] = '1';
            loc = size-1-i;
        } else {
            ret[size-1-i] = '0';
        }
    }
    
    if (start)
        *start = &ret[loc];
    
    // Nowadays malloc will put this in, but we'll still be
    // careful
    ret[size] = '\0';
    return (ret);
}
