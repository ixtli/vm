#include "includes/mmu.h"
#include "includes/cache.h"

MemoryCache::MemoryCache(MMU *mmu) : _mmu(mmu)
{
    
}

MemoryCache::~MemoryCache()
{
    
}

bool MemoryCache::init(reg_t size)
{
    if (!_mmu) return (true);
    
    _size = size;
    
    return (false);
}