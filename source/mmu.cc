#include "includes/mmu.h"

MMU::MMU(size_t size) : _memory_size(size)
{
    
}

MMU::~MMU()
{
    
}

bool MMU::init()
{
    printf("Initializing MMU: %lib RAM\n", _memory_size);
}
