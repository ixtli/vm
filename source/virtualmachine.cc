#include "includes/virtualmachine.h"

VirtualMachine::VirtualMachine()
{
    
}

VirtualMachine::~VirtualMachine()
{
    
}

bool VirtualMachine::init(  const char *mem_in, const char *mem_out,
                            size_t mem_size)
{
    printf("Initizing virtual machine...\n");
    mmu = new MMU(mem_size);
    mmu->init();
    
    // No errors
    return (false);
}

void VirtualMachine::run()
{
    delete mmu;
}

