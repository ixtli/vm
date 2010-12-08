#include "includes/util.h"
#include "includes/mmu.h"
#include "includes/cache.h"

MemoryCache::MemoryCache()
{
    _tag = NULL;
    _dirty = NULL;
    _lru = NULL;
}

MemoryCache::~MemoryCache()
{
    if (_tag) free(_tag);
    if (_dirty) free(_dirty);
    if (_lru) free(_lru);
}

bool MemoryCache::init(MMU *mmu, CacheDescription &desc, char level)
{
    if (!mmu) return (true);
    
    // Line is this many WORDS long
    _length = kLineSize;
    
    // Initialize members
    _mmu = mmu;
    _level = level;
    _size = desc.size;
    _ways = desc.ways;
    _access_time = desc.time;
    
    // Cache should not be more than half the size of memory
    if (_size > _mmu->memorySize() >> 1)
    {
        fprintf(stderr, "Warning: level-%i size > half memory size.\n", level);
        _size = _mmu->memorySize() >> 1;
        desc.size = _size;
    }
    
    // Sizes must be powers of two
    _size = _nearest_power_of_two(_size);
    if ( _size != desc.size)
    {
        fprintf(stderr,
            "Warning: level-%i size changed to nearest power of two.\n", level);
        
        desc.size = _size;
    }
    
    // ways must divide in to the size evenly
    if ( _size % _ways )
    {
        fprintf(stderr, "Warning: level-%i size not divisble by ways.\n", level);
        do {
            _ways--;
        } while (_size % _ways);
        
        desc.ways = _ways;
    }
    
    // Initialize cache
    printf("%ub %u-way level-%u cache... ", _size, _ways, _level);
    
    _tag = (reg_t *)malloc(_size * sizeof(reg_t));
    _dirty = (bool *)calloc(_size, sizeof(bool));
    _lru = (char *)calloc(_size, sizeof(char));
    
    if (!_tag || !_dirty || !_lru)
    {
        printf("Data allocation error.\n");
        return (true);
    }
    
    // initialize tag to -1 for "empty" because it can't get that big
    for (reg_t i = 0; i < _size; i++)
        _tag[i] = kCacheMaxTagValue;
    
    // create the mask for the tag part of an address
    reg_t temp = (_size / _ways);
    char bits = 0;
    while (temp)
    {
        bits++;
        temp >>= 1;
    }
    
    // The _tag_mask now selects the bits of an address with 
    _tag_mask = kCacheMaxTagValue << bits;
    _index_mask = ~_tag_mask;
    
    // TODO: Calculate this when implementing variable line length
    _offset_mask = 0xF;
    
    printf("Done.\n");
    return (false);
}

bool MemoryCache::isCached(reg_t addr)
{
    reg_t index = addr & _index_mask;
    reg_t tag = addr & _tag_mask;
    
    // locate
    
    return (false);
}

bool MemoryCache::cache(reg_t &addr, bool write)
{
    // if addr present in cache
        
        // update LRU state
        
        // if write, mark as dirty
        
        // return false
        
    // else if empty lines
        
        // cache value
        
        // update LRU state
        
        // if write, mark dirty
        
    // else (no empty lines)
        
        // if LRU line is dirty, save victim's addr
        
        // replace victim with addr
        
        // update LRU state
        
        // if write, mark line as dirty
        
        // if evicted line was dirty, change addr to victim's addr
    
    // return true
    
}
