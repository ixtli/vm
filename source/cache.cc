#include "includes/util.h"
#include "includes/mmu.h"
#include "includes/cache.h"

MemoryCache::MemoryCache()
{
    _tag = NULL;
    _dirty = NULL;
}

MemoryCache::~MemoryCache()
{
    if (_tag) free(_tag);
    if (_dirty) free(_dirty);
}

bool MemoryCache::init(MMU *mmu, CacheDescription &desc, char level)
{
    if (!mmu) return (true);
    
    // Initialize members
    _mmu = mmu;
    _level = level;
    _size = desc.size;
    _ways = desc.ways;
    _access_time = desc.time;
    
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
    
    _tag = (reg_t *)malloc(sizeof(reg_t) * _size);
    _dirty = (bool *)malloc(sizeof(bool) * _size);
    _lru = (char *)malloc(sizeof(char) * _size);
    
    if (!_tag || !_dirty || !_lru)
    {
        printf("Data allocation error.\n");
        return (true);
    }
    
    // initialize 
    
    printf("Done.\n");
    return (false);
}