#include <string.h>

#include "includes/mmu.h"

MMU::MMU(size_t size, size_t rtime, size_t wtime) : 
    _memory_size(size), _read_time(rtime), _write_time(wtime)
{}

MMU::~MMU()
{
    printf("Destroying MMU... ");
    if (_memory)
        free(_memory);
    printf("Done.\n");
}

bool MMU::init()
{
    printf("Initializing MMU: %lib RAM\n", _memory_size);
    
    // Allocate real memory from system and zero its
    printf("Allocating and zeroing memory... ");
    _memory = (unsigned int *)calloc(_memory_size, sizeof(int));
    // Test to make sure memory was allocated
    if (!_memory)
    {
        printf("Error.\n");
        return (true);
    } else {
        printf("Done.\n");
    }
   
    size_t memBuff;
 
    return (false);
}

size_t MMU::write(size_t addr, unsigned int valueToSave)
{
    if (addr >= _memory_size)
        return 0;
    
    _memory[addr] = valueToSave;

    return (_write_time);
}

size_t MMU::read(size_t addr, unsigned int &valueToRet)
{
    if (addr >= _memory_size)
        return 0;
    
    valueToRet = _memory[addr];

    return (_read_time);
}

size_t MMU::readRange(size_t start, size_t end, bool hex, char **ret)
{ 
    // Whats the max value?
    int length = 25;
        
    size_t words = end - start;
    size_t max_size = words + (words*length) + 1;
   
    //char range[size];
    *ret = (char*)malloc(sizeof(char) * max_size);
    char *val = *ret;
    
    char single[length+2];
    size_t range_index = 0;

    for(int i = 0; start+i <= end; i++)
    {
        if (!hex)
            sprintf(single, "%#x\t- %d\n",  (unsigned int)start+i, _memory[start+i]);
        else
            sprintf(single, "%#x\t- %#x\n",  (unsigned int)start+i, _memory[start+i]);
        
        strncpy(&val[range_index], single, strlen(single));
        
        range_index += strlen(single);
    }
    val[range_index] = '\0';
    
    return (_read_time * words);
}
