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
        _tag[i] = -1;
    
    printf("Done.\n");
    return (false);
}

bool isCached(reg_t addr)
{
    
}

void cache(reg_t addr)
{
    
}
