#ifndef _CACHE_H_
#define _CACHE_H_

#include "global.h"

typedef struct CacheDescription
{
    reg_t size;
    char ways;
    cycle_t time;
};

// Forward class definitions
class MMU;

class MemoryCache
{
public:
    MemoryCache();
    ~MemoryCache();
    
    bool init(MMU *mmu, CacheDescription &desc, char level);
    
    bool isCached(reg_t addr);
    void cache(reg_t addr);
    
    inline cycle_t accessTime()
    {
        return (_access_time);
    }
    
private:
    MMU *_mmu;
    
    // Cache metadata
    reg_t _size;
    char _ways, _level;
    cycle_t _access_time;
    
    // Cache data
    reg_t *_tag;
    bool *_dirty;
    char *_lru;
    
};

#endif // Include Guard
