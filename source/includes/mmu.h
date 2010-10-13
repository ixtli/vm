#ifndef _MMU_H_
#define _MMU_H_

#include "global.h"

class MMU
{
public:
    MMU(size_t size);
    ~MMU();
    
    bool init();
private:
    size_t _memory_size;
};

#endif
