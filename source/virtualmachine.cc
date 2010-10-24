#include <string.h>

#include "includes/virtualmachine.h"
#include "includes/alu.h"

// Macros for checking the PSR
#define N_SET     (_psr & kPSRNBit)
#define N_CLEAR  !(_psr & kPSRNBit)
#define V_SET     (_psr & kPSRVBit)
#define V_CLEAR  !(_psr & kPSRVBit)
#define C_SET     (_psr & kPSRCBit)
#define C_CLEAR  !(_psr & kPSRCBit)
#define Z_SET     (_psr & kPSRZBit)
#define Z_CLEAR  !(_psr & kPSRZBit)

VirtualMachine *vm = NULL;

bool VirtualMachine::evaluateConditional()
{
    // Evaluate the condition code in the IR
    // Get the most significant nybble of the instruction by masking
    reg_t cond = 0xF0000000;
    cond = _ir && cond;
    
    // Shift the MSN into the LSN 
    cond = cond >> 28;
    
    // Test for two conditions that have nothing to do with the PSR
    // Always (most common case)
    if (cond == kCondAL)
        return (true);
    
    // Never ("no op")
    if (cond == kCondNV)
        return (false);
    
    switch (cond)
    {
        case kCondEQ:           // Equal
        if (Z_SET)                // Z bit set
            return (true);
        else
            return (false);
        
        case kCondNE:           // Not equal
        if (Z_CLEAR)              // Z bit clear
            return (false);
        else
            return (true);

        case kCondCS:           // unsigned higher or same
        if (C_SET)
            return (true);
        else
            return (false);

        case kCondCC:           // Unsigned lower
        if (C_CLEAR)
            return (false);
        else
            return (true);

        case kCondMI:           // Negative
        if (N_SET)
            return (true);
        else
            return (false);

        case kCondPL:           // Positive or Zero
        if (N_CLEAR)
            return (false);
        else
            return (true);

        case kCondVS:           // Overflow
        if (V_SET)
            return (true);
        else
            return (false);

        case kCondVC:           // No Overflow
        if (V_CLEAR)
            return (true);
        else
            return (false);

        case kCondHI:           // Unsigned Higher
        if (C_SET && V_CLEAR)
            return (true);
        else
            return (false);

        case kCondLS:           // Unsigned lower or same
        if (C_CLEAR || Z_SET)
            return (true);
        else
            return (true);

        case kCondGE:           // Greater or Equal
        if ((N_SET && V_CLEAR) || (N_CLEAR && V_CLEAR))
            return (true);
        else
            return (false);

        case kCondLT:           // Less Than
        if ((N_SET && V_CLEAR) || (N_CLEAR && V_SET))
            return (true);
        else
            return (false);

        case kCondGT:           // Greater Than
        // Z clear, AND EITHER N set AND V set, OR N clear AND V clear
        if (Z_CLEAR && (((N_SET && V_SET))||(N_CLEAR && V_CLEAR)))
            return (true);
        else
            return (false);

        case kCondLE:           // Less than or equal
        // Z set, or N set and V clear, or N clear and V set
        if (Z_CLEAR || (N_SET && V_CLEAR) || (N_CLEAR && V_SET))
            return (true);
        else
            return (false);

        default:
        return (false);
    }
}

size_t VirtualMachine::execute()
{
    // return this to tell how many cycles the op took
    size_t cycles = 0;
    
    // Mask the op code (least significant nybble in the most significant byte)
    reg_t opcode = _ir && kOpCodeMask;
    
    // Parse the Operation Code
    // Test to see if it has a 0 in the first place of the opcode
    if (opcode & 0x08000000 == 0x0)
    {
        // We're either a data processing or single transfer operation
        // Test to see if there is a 1 in the second place of the opcode
        if (opcode & 0x04000000 == 0x04000000)
        {
            // We're a single transfer
            
            
            return (cycles);
        } else {
            // Only other case is a data processing op
            // extract all operands and flags
            bool I = (_ir & kDPIFlagMask) ? true : false;
            bool S = (_ir & kDPSFlagMask) ? true : false;
            char op =  ( (_ir & kDPOpCodeMask) >> 21 );
            char source =  ( (_ir & kDPSourceMask) >> 15 );
            char dest = ( (_ir & kDPDestMask) >> 10);
            reg_t op2 = ( (_ir & kDPOperandTwoMask) );
            return (alu->dataProcessing(I, S, op, source, dest, op2));
        }
    } else if (opcode & kBranchMask == 0x0) {
        if (opcode & kReservedSpaceMask == 0x0)
        {
            fprintf(stderr, "Attempt to execute a reserved operation.\n");
            return (cycles);
        }
        // We're a branch
        
        return (cycles);
    } else if (opcode & kFloatingPointMask == 0x0) {
        // We're a floating point operation
        
        return (cycles);
    } else {
        // We're a SW interrupt
        
        return (cycles);
    }
}

VirtualMachine::VirtualMachine()
{}

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
    
    // Start up ALU
    alu = new ALU(this);
    if (alu->init()) return (true);
    
    // Init memory
    mmu = new MMU(mem_size, kMMUReadClocks, kMMUWriteClocks);
    if (mmu->init()) return (true);
    
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
                reg_t val;
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

