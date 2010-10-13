#include "includes/virtualmachine.h"

VirtualMachine *vm = NULL;

VirtualMachine::VirtualMachine()
{
    
}

VirtualMachine::~VirtualMachine()
{
    // Do this first, just in case
    delete ms;
    
    printf("Destroying virtual machine...\n");
    delete mmu;
}

bool VirtualMachine::init(  const char *mem_in, const char *mem_out,
                            size_t mem_size)
{
    terminate = false;
    
    printf("Initializing server... \n");
    ms = new MonitorServer();
    if (ms->init())
        return (false);
    if (ms->run())
        return (false);
    
    printf("Initializing virtual machine...\n");
    
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
    
    while (!terminate)
    {
        // Wait for something to happen
    }
}

