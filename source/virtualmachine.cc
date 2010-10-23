#include <string.h>

#include "includes/virtualmachine.h"

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

inline unsigned int *VirtualMachine::selectRegister(char val)
{
    if (val < kPQ0Code)
        // This is a general register
        return (&_r[val]);
    
    if (val > kFPSRCode)
        // This is an fpu register
        return (&_fpr[val - kFPR0Code]);
    
    switch (val)
    {
        case kPSRCode:
        return (&_psr);
        case kPQ0Code:
        return (&_pq[0]);
        case kPQ1Code:
        return (&_pq[1]);
        case kPCCode:
        return (&_pc);
        case kFPSRCode:
        return (&_fpsr);
        default:
        return (NULL);
    }
}

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

size_t VirtualMachine::shiftOffset(unsigned int &offset)
{
    // Perform the somewhat complex operation of barrel shifting the offset
    // return the cycles required to perform this operation
}

size_t VirtualMachine::dataProcessing(bool I, bool S, char op,
    char s, char d, unsigned int &op2)
{
    size_t cycles = 0;
    
    if (!I)
    {
        // We need to shift the offset instead of treating it as an immediate
        cycles += shiftOffset(op2);
    }
    
    unsigned int *dest = selectRegister(d);
    unsigned int *source = selectRegister(s);
    
    bool arithmetic = false;
    
    switch (op)
    {
        case kADD:
        arithmetic = true;
        *dest = *source + op2;
        cycles += kADDCycles;
        break;
        
        case kNOP:
        default:
        break;
    }
    
    // We're done if we're not setting status bits
    // N.B.: Ignore the S bit if dest == PC, so that we dont get status bit
    //       confusion when you branch or something.
    if (!s || d == kPCCode) return (cycles);
    
    // Set status bits
    // There are two cases, logical and arithmetic
    if (arithmetic)
    {
        // Set V if there was an overflow into the 31st bit
        // My understanding of this is that if 31st bit was NOT
        // set in *source, but is in *dest, set V
        if (*dest & 0x80000000)
        {
            if (!(*source & 0x80000000))
                _psr |= kPSRVBit;
            
            // Also, set the negative bit
            _psr |= kPSRNBit;
        } else {
            
            // Z flag set if dest is zero
            if (*dest == 0x0) {
                _psr |= kPSRZBit;
            } else {
                // As long as we're not zero, we might be super large
                // The C flag is the carry out of bit 31
                if (*dest < *source)
                {
                    _psr |= kPSRCBit;
                } else if (C_SET) {
                    // Flip the c bit cause we didn't overflow
                    _psr ^= kPSRCBit;
                }
            }
            
            // Result is not 'negative'
            if (N_SET)
                // flip the NBit
                _psr ^= kPSRNBit;
        }
        
    } else {
        // We're a logical operation
        
        // C flag is the carry out of the shifter, as long as the shift
        // operation is not LSL #0
        
        // Z flag is set if result is all zeros
        
        // N flag is set to the logical value of bit 31 of the result
    }
    
    return (cycles);
}

size_t VirtualMachine::execute()
{
    // return this to tell how many cycles the op took
    size_t cycles = 0;
    
    // Mask the op code (least significant nybble in the most significant byte)
    unsigned int opcode = _ir && kOpCodeMask;
    
    // Parse the Operation Code
    // Test to see if it has a 0 in the first place of the opcode
    if (opcode & 0x08000000 == 0x0)
    {
        // We're either a data processing or single transfer operation
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
            unsigned int op2 = ( (_ir & kDPOperandTwoMask) );
            return (dataProcessing(I, S, op, source, dest, op2));
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

