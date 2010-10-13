#include "includes/virtualmachine.h"

VirtualMachine::VirtualMachine()
{
    
}

VirtualMachine::~VirtualMachine()
{
    printf("Destroying virtual machine...\n");
    delete mmu;
}

bool VirtualMachine::init(  const char *mem_in, const char *mem_out,
                            size_t mem_size)
{
    printf("Initizing virtual machine...\n");
    
    // Init memory
    mmu = new MMU(mem_size, kMMUReadClocks, kMMUWriteClocks);
    if (mmu->init())
        return (true);
    
    // No errors
    return (false);
}

void VirtualMachine::run()
{
    printf("Running...\n");
}

