#ifndef _CACHE_H_
#define _CACHE_H_

#include "global.h"


enum CacheConstants
{
    kIgnoredBits = 2,
    kIgnoredBitsMask = 0x3,
    kLineSize = 4
};

typedef struct CacheDescription
{
    reg_t size;
    char ways;
    char len;
    cycle_t time;
    char level;
    bool debug;
};

// Forward class definitions
class MMU;

class MemoryCache
{
public:
    MemoryCache();
    ~MemoryCache();
    
    bool init(MMU *mmu, CacheDescription &desc, MemoryCache *parent);
    
    cycle_t write(reg_t addr, reg_t val);
    cycle_t read(reg_t &addr);
    
    inline cycle_t accessTime()
    {
        return (_access_time);
    }
    
    inline bool storesValues()
    {
        return (_store);
    }
    
private:
    reg_t lru(reg_t set);
    bool isCached(reg_t addr, reg_t &index);
    cycle_t cache(reg_t addr, reg_t &index, bool write = false);
    void use(reg_t index);
    reg_t valueAtOffset(reg_t index, reg_t offset);
    
    MMU *_mmu;
    bool _debug;
    
    // Cache metadata
    reg_t _size;
    char _ways, _level, _line_length;
    char _offset_bits, _index_bits, _tag_bits;
    cycle_t _access_time;
    MemoryCache *_parent;
    reg_t _tag_mask, _index_mask, _offset_mask;
    
    // Cache data
    reg_t *_tag;
    bool *_dirty;
    char *_lru;
    
    // do we actually save data?
    bool _store;
};

#endif // Include Guard
