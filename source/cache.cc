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

bool MemoryCache::init(MMU *mmu, CacheDescription &desc, MemoryCache *parent)
{
    if (!mmu) return (true);
    
    _parent = parent;
    
    // TODO: make this variable
    _store = false;
    
    // Initialize members
    _mmu = mmu;
    _level = desc.level;
    _size = desc.size;
    _ways = desc.ways;
    _line_length = desc.len;
    _access_time = desc.time;
    _debug = desc.debug;
    
    // Cache should not be more than half the size of memory
    if (_size > _mmu->memorySize() >> 1)
    {
        fprintf(stderr, "Warning: level-%i size > half memory size.\n", _level);
        _size = _mmu->memorySize() >> 1;
        desc.size = _size;
    }
    
    // Sizes must be powers of two
    _size = _nearest_power_of_two(_size);
    if ( _size != desc.size)
    {
        fprintf(stderr,
            "Warning: level-%i size changed to nearest power of two.\n",_level);
        
        desc.size = _size;
    }
    
    // Line length must be powers of two
    _line_length = _nearest_power_of_two(_line_length);
    if ( _line_length != desc.len)
    {
        fprintf(stderr,
            "Warning: level-%i line length changed to nearest power of two.\n",
            _level);
        
        desc.len = _line_length;
    }
    
    // ways must divide in to the size evenly
    if ( _size % _ways )
    {
        fprintf(stderr, "Warning: level-%i size not divisble by ways.\n",_level);
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
    reg_t temp = _line_length;  // How many WORDS total?
    _offset_bits = 0;
    while (temp)
    {
        _offset_bits++;
        temp >>= 1;
    }
    
    // compute how many bits the index will take
    temp = _ways;
    _index_bits = 0;
    while (temp)
    {
        _index_bits++;
        temp >>= 1;
    }
    
    // compute the bit length of the tag
    _tag_bits = kRegBits - _index_bits - _offset_bits;
    
    if (_debug)
        printf("\nCACHE: %u bit tag, %u bit index, %u bit offset \n",
                _tag_bits, _index_bits, _offset_bits);
    
    /*  In the folowing computations, kIgnoredBits always represents the fact
        that we want to IGNORE which byte an address asks for and only select
        words this means that we have to ignore the least significant 2 bits of
        an address.  The IgnoredBitsMask masks those two lowest bits.
    */
    
    // The _tag_mask now selects an address' tag
    _tag_mask = kWordMask << _index_bits + _offset_bits + kIgnoredBits;
    
    // Make the mask for the offset
    _offset_mask = ~(kWordMask << _offset_bits);
    // For the reasons explained above, do not select the lowest 2 bits 
    _offset_mask <<= kIgnoredBits;
    
    // _index_mask selects everything that hasn't already been selected
    _index_mask = ~(_tag_mask | _offset_mask | kIgnoredBitsMask);
    
    if (_debug)
        printf("CACHE: Masks: tag: %#010x index: %#010x offset: %#010x\n",
               _tag_mask, _index_mask, _offset_mask);
    
    printf("Done.\n");
    return (false);
}

bool MemoryCache::isCached(reg_t addr, reg_t &set_index)
{
    // get our tag
    reg_t tag = addr & _tag_mask;
    
    // calculate the index
    reg_t index = addr & _index_mask;
    // shift it into the lower end of the word
    index >>= _offset_bits + kIgnoredBits;
    
    // go to the beginning of the set that corresponds to this index
    index *= _ways;
    
    // This is like the selector in an associative cache
    for (int i = 0; i < _ways; i++)
    {
        if (tag == _tag[index + i])
        {
            set_index = i + index;
            return (true);
        }
    }
    
    return (false);
}

reg_t MemoryCache::valueAtOffset(reg_t index, reg_t offset)
{
    // This function would return the value of the requested word
    // at specified offset in the specified cache line
    
    // Instead we simply return the requested offset for now
    return ((reg_t)offset);
}

void MemoryCache::use(reg_t index)
{
    // "Use" the cache line at index, updating the rest of the lru
    // values for the set
    
    reg_t target = index % _ways;
    reg_t start = index - target;
    
    for (reg_t i = 0; i < _ways; i++)
    {
        if (i == target)
            _lru[i] = 0;
        else
            _lru[i]++;
    }
}

cycle_t MemoryCache::cache(reg_t addr, reg_t &index, bool write)
{
    cycle_t ret = accessTime();
    reg_t valToWrite = index;
    
    if (addr & kIgnoredBitsMask)
    {
        fprintf(stderr, "CACHE: Lookup address not word aligned.\n");
        return (0);
    }
    
    if (_debug)
    {
        if (write)
            printf("CACHE: Writing address %u from ", addr);
        else
            printf("CACHE: Reading address %u from ", addr);
        printf("level %u cache... ", _level);
    }
    
    if (isCached(addr, index))
    {
        // This is a cache hit!
        // index now points to the index in our cache where the val is
        if (_debug)
            printf("cache hit %u ", index);
        
        // update LRU state
        use(index);
        
        // if write, mark as dirty
        if (write)
        {
            _dirty[index] = true;
            
            // actually write the value
        } else {
            // return the value
            index = 0;
        }
        
        // inform caller that there were no evictions
        if (_debug) printf("\n");
        return (ret);
    }
    
    // it's not in the cache, so we need to calculate where to put it
    reg_t set = (addr & _index_mask);
    // ignore the offset entirely, we're just looking for a line in a set
    set >>= (_offset_bits + kIgnoredBits);
    // make set point to the beginning of this index's set
    set *= _ways;
    
    // see if set has empty lines
    for (int i = 0; i < _ways; i++)
    {
        if (_tag[set + i] == kWordMask)
        {
            // There was an empty line in this set, so assign it
            index = set + i;
            
            if (_debug)
            {
                printf("empty line found in set %u way %u ",
                        set ? set / _ways : 0, i);
            }
            
            // set tag
            _tag[index] = addr & _tag_mask;
            
            // This is where we would actually get the value requested
            if (!write)
            {
                if (_parent)
                {
                    if (_debug) printf("\n");
                    
                    // If there is more cache above us, ask it for the value
                    reg_t temp = addr;
                    for (int j = 0; j < _line_length; j++)
                    {
                        temp = addr + (j << 2);
                        ret += _parent->read(temp);
                    }
                    
                } else {
                    // Go to main memory
                    for (int j = 0; j < _line_length; j++)
                        // For now, just time everything
                        ret += _mmu->readTime();
                }
            } else {
                // set the value, if you're, you know, into that
            }
            
            // update LRU state
            use(index);
            
            // set the dirty bit
            _dirty[index] = write;
            
            // Set the value otherwise
            if (!write) index = 0;
            
            // return
            if (_debug) printf("\n");
            return (ret);
        }
    }
    
    // If control reaches here, we have to evict
    // find LRU line and use it as index
    index = lru(set);
    
    if (_debug)
    {
        reg_t comp_set = set ? set / _ways : 0;
        printf("evicting set %u way %u ",
                comp_set, index - set);
    }
    
    // if index is dirty, write the value to parent
    if (_dirty[index])
    {
        if (_debug) printf("dirty ");
        
        if (_parent)
        {
            if (_debug) printf("\n");
            
            // If there is more cache above us, write to them
            // For now, don't do anything
            reg_t temp = 5;
            for (int i = 0; i < _line_length; i++)
                ret += _parent->write(addr + (i << 2), valToWrite);
            
        } else {
            // go to main memory
            for (int i = 0; i < _line_length; i++)
                ret += _mmu->writeTime();
        }
        
    }
    
    // replace victim with addr
    _tag[index] = addr & _tag_mask;
    
    // This is where we would actually get the value requested
    
    // TODO: Optimize this so that we don't read from the word
    // we're going to write to.
    if (_parent)
    {
        if (_debug) printf("\n");
    
        // If there is more cache above us, ask it for the value
        reg_t temp;
        for (int i = 0; i < _line_length; i++)
        {
            temp = addr + (i << 2);
            ret += _parent->read(temp);
        }
    } else {
        // go to main memory
        for (int j = 0; j < _line_length; j++)
            ret += _mmu->readTime();
    }
    
    // update LRU state on index
    use(index);
    
    // if write, mark line as dirty
    _dirty[index] = write;
    
    if (write)
    {
        // write the value to line[index]
    } else {
        // return the value in index
        index = 0;
    }
    
    if (_debug) printf("\n");
    return (ret);
}

reg_t MemoryCache::lru(reg_t set)
{
    // Assume set points to the BEGINNING of the set
    if (set + (_ways - 1) > _size)
    {
        fprintf(stderr, "CACHE: lru set index out of bounds.\n");
        return (0);
    }
    
    char lru = 0;
    reg_t ret = set;
    for (int i = 0; i < _ways; i++)
    {
        if (_lru[set+i] > lru)
        {
            lru = _lru[set+i];
            ret = set + i;
        }
    }
    
    return (ret);
}

cycle_t MemoryCache::write(reg_t addr, reg_t val)
{
    // Check to see if the address is requesting a word that falls over the
    // boundry of a cache line
    cycle_t ret = 0;
    char start = addr & kIgnoredBitsMask;
    
    if (start)
    {
        //if (_debug)
            printf("CACHE: Writing over word boundry.\n");
        
        reg_t top, bottom;
        reg_t mask = kWordMask;
        start *= 8; // the amount of bits in the LOWER half of the word
        mask >>= start;
        
        // Mask will select the bytes to be written to the BOTTOM word
        bottom = val & mask;
        // NOT mask to get the TOP word
        top = val & ~mask;
        // have to shift it back into place of course
        top <<= kRegBits - start;
        
        // write TOP by ignoring bits
        reg_t lookup = addr >> kIgnoredBits;
        lookup <<= kIgnoredBits;
        ret = cache(lookup, top, true);
        
        // write BOTTOM by going one word forward
        ret += cache(lookup + 4, bottom, true);
        
        return (ret);
    }
    
    // If not, go right to cache
    // drop the bottom two bits of the address
    reg_t index = val;
    return (cache(addr, index, true));
} 

cycle_t MemoryCache::read(reg_t &addr)
{
    // Check to see if the address is requesting a word that falls over the
    // boundry of a cache line
    cycle_t ret = 0;
    char start = addr & kIgnoredBitsMask;
    
    if (start)
    {
        if (_debug) printf("CACHE: Reading over word boundry.");
        
        // read TOP by ignoring bits
        reg_t top, bottom;
        reg_t lookup = addr >> kIgnoredBits;
        lookup <<= kIgnoredBits;
        ret = cache(lookup, top);
        
        // write BOTTOM by going one word forward
        ret += cache(lookup + 4, bottom);
        
        // Mask out the returned values
        start *= 8; // the amount of bits in the LOWER half of the word
        
        addr = bottom >> start;
        addr |= top << (kRegBits - start);
        
        return (ret);
    }
    
    // If not, go right to cache
    // drop the bottom two bits of the address
    reg_t index = 0;
    ret = cache(addr, index);
    addr = index;
    return (ret);
}

