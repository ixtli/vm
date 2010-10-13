#include <string.h>

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
    mmu->writeOut(dump_path);
    delete mmu;
}

bool VirtualMachine::init(  const char *mem_in, const char *mem_out,
                            size_t mem_size)
{
    if (pthread_mutex_init(&waiting, NULL))
    {
        printf("Can't init mutex.\n");
        return (true);
    }
    
    terminate = false;
    
    printf("Initializing server... \n");
    ms = new MonitorServer();
    if (ms->init())
        return (true);
    if (ms->run())
        return (true);
    
    printf("Initializing virtual machine...\n");
    
    // Init memory
    mmu = new MMU(mem_size, kMMUReadClocks, kMMUWriteClocks);
    if (mmu->init())
        return (true);
    
    dump_path = mem_out;
    
    mmu->loadFile(mem_in);
    
    // No errors
    return (false);
}

void VirtualMachine::run()
{
    printf("Running...\n");
    
    while (!terminate)
    {
        // Wait for something to happen
        pthread_mutex_lock(&waiting);
        if (terminate)
            return;
        // If we get here, we have a job to do.
        char *pch;
        pch = strtok(operation, " ");
        if (pch != NULL)
        {
            if (strcmp(pch, kWriteCommand) == 0)
            {
                int addr, val;
                pch = strtok(NULL, " ");
                if (pch)
                    addr = atoi(pch);
                else
                    addr = 0;
            
                pch = strtok(NULL, " ");
                if (pch)
                    val = atoi(pch);
                else
                    val = 0;
            
                mmu->write(addr, val);
            
                response = (char *)malloc(sizeof(char) * 512);
                sprintf(response, "Value '%i' written to '%#x'.\n",
                    val, addr);
                respsize = strlen(response);
            } else if (strcmp(pch, kReadCommand) == 0) {
                // read
                int addr;
                unsigned int val;
                pch = strtok(NULL, " ");
                if (pch)
                    addr = atoi(pch);
                else
                    addr = 0;
            
                mmu->read(addr, val);
            
                response = (char *)malloc(sizeof(char) * 512);
                sprintf(response, "%#x - %i\n", addr, val);
                respsize = strlen(response);
            } else if (strcmp(pch, kRangeCommand) == 0) {
                // range
                int addr, val;
                pch = strtok(NULL, " ");
                if (pch)
                    addr = atoi(pch);
                else
                    addr = 0;
            
                pch = strtok(NULL, " ");
                if (pch)
                    val = atoi(pch);
                else
                    val = 0;
            
                char *temp;
                mmu->readRange(addr, val, true, &temp);
                response = temp;
                respsize = strlen(response);
            } else {
                // unknown command
                respsize = 0;
            }
        }
        
        // we're done
        pthread_mutex_unlock(&waiting);
    }
}

