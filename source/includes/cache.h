#ifndef _CACHE_H_
#define _CACHE_H_

#include "global.h"

enum CacheConstants
{
    kLineSize = 4
};

typedef struct CacheDescription
{
    reg_t size;
    char ways;
    char len;
    cycle_t time;
};

// Forward class definitions
class MMU;

class MemoryCache
{
public:
    MemoryCache();
    ~MemoryCache();
    
    bool init(MMU *mmu, CacheDescription &desc, char level, bool debug = false);
    
    bool isCached(reg_t addr, size_t &index);
    bool cache(reg_t &addr, bool write = false);
    
    inline cycle_t accessTime()
    {
        return (_access_time);
    }
    
private:
    MMU *_mmu;
    bool _debug;
    
    // Cache metadata
    reg_t _size;
    char _ways, _level, _line_length;
    char _offset_bits, _index_bits, _tag_bits;
    cycle_t _access_time;
    
    reg_t _tag_mask, _index_mask, _offset_mask;
    
    // Cache data
    reg_t *_tag;
    bool *_dirty;
    char *_lru;
    
};

#endif // Include Guard
