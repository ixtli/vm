#include <string.h>

#include "includes/virtualmachine.h"

VirtualMachine *vm = NULL;

bool VirtualMachine::evaluateConditional()
{
    // Evaluate the condition code in the IR
    // Get the most significant nybble of the instruction by masking
    unsigned int cond = 0xF0000000;
    cond = _ir && cond;
    
    // Shift the MSN into the LSN 
    cond = cond >> 28;
    
    // Test for two conditions that have nothing to do with the PSR
    // Always (most common case)
    if (cond == 0xE)
        return (true);
    
    // Never ("no op")
    if (cond == 0xF)
        return (false);
    
    // Cond is now our index into the PSR.
    unsigned int one = 1;
    one = one << cond;
    // If the bit at that index is flipped, execute the instruction
    if (_psr && one)
        return (true);
    
    // Otherwise, don't do it
    return (false);
}

size_t VirtualMachine::execute()
{
    // return this to tell how many cycles the op took
    size_t cycles = 0;
    
    // Mask the op code (least significant nybble in the most significant byte)
    unsigned int opcode = _ir && 0x0F000000;
    
    // Move the op code down so we can talk about it in decimal
    opcode = opcode >> 24;
    
    if (opcode < 4)
    {
        // its a data processing operation
        
        return (cycles);
    }
    
    if (opcode < 8)
    {
        // it's single transfer
        
        return (cycles);
    }
    
    if (opcode < 14)
    {
        if (opcode < 10)
        {
            // This is reserved space
            fprintf(stderr, "Attempt to execute a reserved operation.\n");
            return (cycles);
        }
        // it's a branch
        
        return (cycles);
    }
    
    if (opcode == 14)
    {
        // it's a floating point op
        
        return (cycles);
    }
    
    if (opcode == 15)
    {
        // it's a sw interrupt
        
        return (cycles);
    }
}

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
    
    terminate = 0;
    
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
    
    // Init cycle count
    _cycle_count = 0;
    
    printf("Loading interrupt table.\n");
    // Load interupts and corresponding functions
    _int_table_size = 0;
    printf("Loading interrupt functions.\n");
    _int_function_size = 0;
    
    // TODO:  Have the machine do this at start up
    // Set up a sane operating environment
    
    // Start the program at the byte after the end of the interrupt functions
    _pc = 0x0;
    
    // Advance pc to allocate stack space
    _ss = _pc;
    _pc += kDefaultStackSpace;
    
    // Load memory image at PC.
    size_t code_seg_size = mmu->loadFile(mem_in, _pc);
    _cs = _pc;
    _ds = _pc + code_seg_size;
    
    // Set up PSR
    _psr = kPSRDefault;
    
    // No errors
    return (false);
}

void VirtualMachine::run()
{
    printf("Running...\n");
    
    while (true)
    {
        // Fetch PC instruction into IR and increment the pc
        _cycle_count += mmu->read(++_pc, _ir);

        // If the cond code precludes execution of the op, don't bother
        if (!evaluateConditional())
            continue;
        
        // Do the op
        _cycle_count += execute();
    }
    
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

