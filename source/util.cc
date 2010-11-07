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
    const char *index = str;
    size_t len = strlen(str);
    
    // check to see if they started with '0x'
    if (len > 1)
    {
        if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X'))
        {
            // if so, skip it
            index += 2;
            len = strlen(index);
        }
    }
    
    // parse
    unsigned int charval = 0;
    for (int i = 0; i < len; i++)
    {
        if (index[i] > 47 && index[i] < 58 )
        {
            // [0-9]
            charval = index[i] - 48;
        } else if (index[i] > 64 && index[i] < 91) {
            // [A-Z]
            charval = index[i] - 65 + 10;
            
        } else if (index[i] > 96 && index[i] < 123) {
            // [a-z]
            charval = index[i] - 97 + 10;
        }
        // i is our index into the returned number
        ret |= (charval << (4 * (len - i -1)));
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
