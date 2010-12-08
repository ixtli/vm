#include "includes/util.h"
#include "includes/virtualmachine.h"
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

bool MemoryCache::init(MMU *mmu, CacheDescription &desc, char level, bool debug)
{
    if (!mmu) return (true);
    
    _debug = debug;
    
    // Initialize members
    _mmu = mmu;
    _level = level;
    _size = desc.size;
    _ways = desc.ways;
    _line_length = desc.len;
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
    
    // Line length must be powers of two
    _line_length = _nearest_power_of_two(_line_length);
    if ( _line_length != desc.len)
    {
        fprintf(stderr,
            "Warning: level-%i line length changed to nearest power of two.\n",
            level);
        
        desc.len = _line_length;
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
    printf("%u word long %u line, %u-way level-%u cache... ",
            _line_length, _size, _ways, _level);
    
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
        _tag[i] = kWordMask;
    
    // compute how many bits the offset will take up
    reg_t temp = _line_length << 2;  // How many bytes total? (words * 4)
    _offset_bits = 0;
    while (temp)
    {
        _offset_bits++;
        temp >>= 1;
    }
    
    // compute how many bits the index will take
    temp = (_size / _ways);
    _index_bits = 0;
    while (temp)
    {
        _index_bits++;
        temp >>= 1;
    }
    
    // compute the bit length of the tag
    _tag_bits = kRegBits - _index_bits - _offset_bits;
    
    if (_debug)
    {
        printf("\n   %u bit tag, %u bit index, %u bit offset \n",
                _tag_bits, _index_bits, _offset_bits);
    }
    
    // The _tag_mask now selects an address' tag
    _tag_mask = kWordMask << _index_bits + _offset_bits;
    // _index_mask now selects the index's index
    _index_mask = (~_tag_mask) << _offset_bits;
    // _offset_mask now selects the least significant offsetBits of an address
    _offset_mask = ~(kWordMask << _offset_bits);
    
    printf("Done.\n");
    return (false);
}

bool MemoryCache::isCached(reg_t addr, size_t &index)
{
    reg_t tag = addr & _tag_mask;
    
    // Is the tag in our cache?
    for (size_t i = 0; i < _size; i++)
    {
        if (tag == _tag[i])
        {
            index = i;
            return (true);
        }
    }
    
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
