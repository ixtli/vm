#ifndef _CACHE_H_
#define _CACHE_H_

#include "global.h"

// Forward class definitions
class MMU;

class MemoryCache
{
public:
    MemoryCache(MMU *mmu);
    ~MemoryCache();
    
    bool init(reg_t size);
    
private:
    MMU *_mmu;
    reg_t _size;
};

#endif // Include Guard
