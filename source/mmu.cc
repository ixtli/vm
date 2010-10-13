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

size_t MMU::write(size_t addr, size_t valueToSave)
{
    _memory[addr] = valueToSave;
    printf("%li", valueToSave, " has been placed at %li", addr, " %li", _memory[addr]);   

    return (_write_time);
}

size_t MMU::read(size_t addr, size_t &valueToRet)
{
    valueToRet = _memory[addr];
    printf("the address %li", addr, " contains %li", valueToRet);

    return (_read_time);
}

char* MMU::readRange(size_t start, size_t end, bool hex)
{ 
    int size = int(end - start);
   
    //char range[size];
    char* range = (char*)malloc(sizeof(char) * size);
    char single[1];    

    for(int i = 0; start+i <= end; i++)
    {
        if (!hex)
            sprintf(single, "%d",  _memory[start+i]);
        else
            sprintf(single, "%x",  _memory[start+i]);

        range[i] = single[0];
    }
    return (range);
}
