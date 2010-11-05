#include <string.h>
#include <iostream>

#include "includes/virtualmachine.h"
#include "includes/interrupt.h"
#include "includes/mmu.h"
#include "includes/alu.h"
#include "includes/fpu.h"
#include "includes/util.h"
#include "includes/luavm.h"

// Macros for checking the PSR
#define N_SET     (_psr & kPSRNBit)
#define N_CLEAR  !(_psr & kPSRNBit)
#define V_SET     (_psr & kPSRVBit)
#define V_CLEAR  !(_psr & kPSRVBit)
#define C_SET     (_psr & kPSRCBit)
#define C_CLEAR  !(_psr & kPSRCBit)
#define Z_SET     (_psr & kPSRZBit)
#define Z_CLEAR  !(_psr & kPSRZBit)

bool VirtualMachine::evaluateConditional()
{
    // Evaluate the condition code in the IR
    // Get the most significant nybble of the instruction by masking
    reg_t cond = 0xF0000000;
    cond = _ir & cond;
    
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

cycle_t VirtualMachine::execute()
{
    // return this to tell how many cycles the op took
    cycle_t cycles = 0;
    // Parse the Operation Code
    // Test to see if it has a 0 in the first place of the opcode
    if ((_ir & 0x08000000) == 0x0)
    {
        // We're either a data processing or single transfer operation
        // Test to see if there is a 1 in the second place of the opcode
        if (_ir & kSingleTransferMask)
        {
            // We're a single transfer
            STFlags f;
            f.I = (_ir & kSTIFlagMask) ? true : false;
            f.L = (_ir & kSTLFlagMask) ? true : false;
            f.W = (_ir & kSTWFlagMask) ? true : false;
            f.B = (_ir & kSTBFlagMask) ? true : false;
            f.U = (_ir & kSTUFlagMask) ? true : false;
            f.P = (_ir & kSTPFlagMask) ? true : false;
            f.rs = (_ir & kSTSourceMask) >> 15;
            f.rd = (_ir & kSTDestMask) >> 10;
            f.offset = (_ir & kSTOffsetMask);
            return (mmu->singleTransfer(&f));
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
    } else if (_ir & kBranchMask == 0x0) {
        if (_ir & kReservedSpaceMask == 0x0)
        {
            fprintf(stderr, "Attempt to execute a reserved operation.\n");
            return (cycles);
        }
        // We're a branch
        if (_ir & kBranchLBitMask)
            // Store current pc in the link register (r15)
            _r[15] = _pc;
        
        // Left shift the address by two because instructions are word-aligned
        int temp = (_ir & kBranchOffsetMask) << 2;
        
        // Temp is now a 25-bit number, but needs to be sign extended to 32 bits
        // we do this with a little struct trick that will PROBABLY work
        // everywhere.
        struct {signed int x:25;} s;
        signed int addr = s.x = temp;
        
        // Add the computed address to the pc and make sure it's not negative
        addr += (signed int)_pc;
        if (addr < 0)
            _pc = 0;
        else
            _pc = addr;
        
        return (cycles);
    } else if (_ir & kFloatingPointMask == 0x0) {
        // We're a floating point operation
        char op = ((_ir & kFPOpcodeMask) >> 20);
        char fps = ((_ir & kFPsMask) >> 17);
        char fpd = ((_ir & kFPdMask) >> 14);
        char fpn = ((_ir & kFPnMask) >> 11);
        char fpm = ((_ir & kFPmMask) >> 8);
        return (fpu->execute(op, _fpr[fps], _fpr[fpd], _fpr[fpn], _fpr[fpm]));
    } else {
        // We're a SW interrupt
        // pc is always saved in r15 before branching
        _r[15] = _pc;
        return (icu->swint(_ir & kSWIntCommentMask));
    }
}

VirtualMachine::VirtualMachine()
{
    _program_file = NULL;
}

VirtualMachine::~VirtualMachine()
{
    // Do this first, just in case
    delete ms;
    
    printf("Destroying virtual machine...\n");
    mmu->writeOut(_dump_file);
    delete mmu;
    
    if (_program_file)
        free(_program_file);
}

bool VirtualMachine::loadProgramImage(const char *path, reg_t addr)
{
    // Copy command line arguments onto the stack
    
    // Allocate stack space before code
    _ss = addr + (_stack_size << 2);
    printf("%u words of stack allocated at %#x\n", _stack_size, _ss);
    
    // Code segment starts right after it
    _cs = _ss + 1;
    printf("Code segment begins at %#x\n", _cs);
    
    // Load file into memory at _cs
    reg_t image_size = mmu->loadProgramImageFile(path, _cs, true);
    
    // Set data segment after code segment
    _ds = _cs + image_size;
    
    // Report
    printf("Data segment begins at %#x\n", _ds);
    
    // Initialize program state
    resetGeneralRegisters();
    _psr = kPSRDefault;
    _fpsr = 0;
    
    // Jump to _main
    // TODO: Make this jump to the main label, not the top
    _pc = _cs;
}

void VirtualMachine::resetSegmentRegisters()
{
    _cs = 0; _ds = 0; _ss = 0;
}

void VirtualMachine::resetGeneralRegisters()
{
    for (int i = 0; i < kGeneralRegisters; i++)
        _r[i] = 0;
    for (int i = 0; i < kFPRegisters; i++)
        _fpr[i] = 0;
    for (int i = 0; i < kPQRegisters; i++)
        _pq[i] = 0;
}

bool VirtualMachine::configure(const char *c_path)
{
    
    // Parse the config file
    LuaVM *lua = new LuaVM();
    lua->init();
    int ret = lua->exec(c_path, 0);
    
    if (ret)
    {
        // Something went wrong
        fprintf(stderr, "Execution of script failed,");
        delete lua;
        return (true);
    }
    
    // Count the errors for values that must be specified
    int err = 0;
    
    // string locations
    const char *prog_temp, *dump_temp;
    
    // Grab the config data from the global state of the VM post exec
    err += lua->getGlobalUInt("memory_size", _mem_size);
    err += lua->getGlobalUInt("read_cycles", _read_cycles);
    err += lua->getGlobalUInt("write_cycles", _write_cycles);
    
    // Optional arguments
    lua->getGlobalUInt("stack_size", _stack_size);
    lua->getGlobalString("program", &prog_temp);
    lua->getGlobalString("memory_dump", &dump_temp);
    
    // If 
    if (err)
    {
        fprintf(stderr, "A required value was not set in the config file.\n");
        delete lua;
        return (true);
    }
    
    // Copy strings, because they wont exist after we free the lua VM
    _program_file = (char *)malloc(sizeof(char) * strlen(prog_temp) + 1);
    strcpy(_program_file, prog_temp);
    _dump_file = (char *)malloc(sizeof(char) * strlen(dump_temp) + 1);
    strcpy(_dump_file, dump_temp);
    
    // clean up 
    delete lua;
    return (false);
}

bool VirtualMachine::init()
{
    if (pthread_mutex_init(&waiting, NULL))
    {
        printf("Can't init mutex.\n");
        return (true);
    }
    
    // Initialize virtual machine constructs here
    terminate = 0;
    
    // Initialize command and status server
    printf("Initializing server... \n");
    ms = new MonitorServer(this);
    if (ms->init())
        return (true);
    if (ms->run())
        return (true);
    
    // Initialize actual machine
    printf("Initializing virtual machine...\n");
    printf("Registers size: %lu bytes.\n", kRegSize);
    
    // Start up ALU
    alu = new ALU(this);
    if (alu->init()) return (true);
    
    // Start up FPU
    fpu = new FPU(this);
    if (fpu->init()) return (true);
    
    // Init memory
    mmu = new MMU(this, _mem_size, _read_cycles, _write_cycles);
    if (mmu->init()) return (true);
    
    // Init cycle count
    _cycle_count = 0;
    
    // Initialize required default machine state
    _pc = 0;
    _psr = kPSRDefault;
    _fpsr = 0;
    
    // Load interrupt controller
    printf("Loading interrupt controller.\n");
    icu = new InterruptController(this);
    icu->init();
    
    // Create initial sane environment
    resetGeneralRegisters();
    resetSegmentRegisters();
    
    // Initialize the operating system here by running an interrupt that breaks
    // at the end, returning control to us
    
    // TODO: Make the OS load the program image through an interrupt
    // run(true);
    // Load program right after function table
    loadProgramImage(_program_file, _int_table_size + _int_function_size);
    
    // We can now start the Fetch EXecute cycle
    fex = true;
    
    // No errors
    return (false);
}

void VirtualMachine::eval(char *op)
{
    // We need to parse arguments
    char *pch = strtok(op, " ");
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
            val = _hex_to_int(pch);
        else
            val = 0;
    
        mmu->writeWord(addr, val);
    
        response = (char *)malloc(sizeof(char) * 512);
        sprintf(response, "Value '%i' written to '%#x'.\n",
            val, addr);
        respsize = strlen(response);
        return;
    } else if (strcmp(pch, kReadCommand) == 0) {
        // read
        int addr;
        reg_t val;
        pch = strtok(NULL, " ");
        if (pch)
            addr = atoi(pch);
        else
            addr = 0;
    
        mmu->readWord(addr, val);
    
        response = (char *)malloc(sizeof(char) * 512);
        sprintf(response, "%#x - %#x\n", addr, val);
        respsize = strlen(response);
        return;
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
        return;
    } else if (strcmp(pch, kExecCommand) == 0) {
        reg_t instruction;
        pch = strtok(NULL, " ");
        
        if (pch)
            instruction = (reg_t) _hex_to_int(pch);
        else
            instruction = 0;
        
        reg_t ir = _ir;
        _ir = instruction;
        execute();
        _ir = ir;
        
        response = (char *)malloc(sizeof(char) * strlen(op));
        strcpy(response, op);
        respsize = strlen(response);
        
        return;
    }
    
    // If nothing worked just send machine status
    char temp[1024];
    sprintf(temp, "Machine Status: ");
    if (supervisor)
        sprintf(temp+strlen(temp), "Supervisor mode\n");
    else
        sprintf(temp+strlen(temp), "User mode\n");
    sprintf(temp+strlen(temp),  "Cycle Count: %lu\n", _cycle_count);
    sprintf(temp+strlen(temp),  "Program Status Register: %#x\n", _psr);
    sprintf(temp+strlen(temp),  "Program Counter: %#x\n", _pc);
    sprintf(temp+strlen(temp),  "Instruction Register: %#x\n", _ir);
    sprintf(temp+strlen(temp),  "Code segment: %#x\n", _cs);
    sprintf(temp+strlen(temp),  "Data segment: %#x\n", _ds);
    sprintf(temp+strlen(temp),  "Stack segment: %#x\n", _ss);
    sprintf(temp+strlen(temp), "General Purpose Registers:\n");
    sprintf(temp+strlen(temp),  
        "r0 - %u r1 - %u r2 - %u r3 - %u\n", _r[0], _r[1], _r[2], _r[3]);
    sprintf(temp+strlen(temp),  
        "r4 - %u r5 - %u r6 - %u r7 - %u\n", _r[4], _r[5], _r[6], _r[7]);
    sprintf(temp+strlen(temp),  
        "r8 - %u r9 - %u r10 - %u r11 - %u\n", _r[8], _r[9], _r[10], _r[11]);
    sprintf(temp+strlen(temp),  
        "r12 - %u r13 - %u r14 - %u r15 - %u\n", _r[12], _r[13], _r[14], _r[15]);
    
    response = (char *)malloc(sizeof(char) * strlen(temp) + 1);
    strcpy(response, temp);
    respsize = strlen(response);
}

void VirtualMachine::run(bool break_after_fex)
{
    printf("Starting execution at %#x\n", _pc);
    while (fex)
    {
        // Fetch PC instruction into IR and increment the pc
        _cycle_count += mmu->readWord(_pc, _ir);
        printf("PC: %#X\t\t\t%#X\n", _pc, _ir);
        _pc += kRegSize;
        
        // FOR NOW:
        if (_pc > (0x50 + _cs))
        {
            fprintf(stderr, "CPU TRAP: Program unlikely to be this long.\n");
            _ir = 0xEF000000;
            execute();
            continue;
        }
        
        // If the cond code precludes execution of the op, don't bother
        if (!evaluateConditional())
            continue;
        
        // Do the op
        _cycle_count += execute();
    }
    
    if (break_after_fex) return;
    
    while (!terminate)
    {
        // Wait for something to happen
        pthread_mutex_lock(&waiting);
        if (terminate)
            continue;
        
        // If we get here, we have a job to do
        if (!operation) continue;
        eval(operation);
        
        // we're done
        pthread_mutex_unlock(&waiting);
    }
    
    printf("Exiting...\n");
}

void VirtualMachine::installJumpTable(reg_t *data, reg_t size)
{
    mmu->writeBlock(0x0, data, size);
    _int_table_size = size;
    printf("%ub interrupt jump table at 0x0.\n", size);
}

void VirtualMachine::installIntFunctions(reg_t *data, reg_t size)
{
    mmu->writeBlock(_int_table_size, data, size);
    _int_function_size = size;
    printf("%ub interrupt functions at %#x.\n", size, (reg_t)_int_table_size);
}

void VirtualMachine::shiftOffset(reg_t &offset, reg_t *val)
{
    alu->shiftOffset(offset, val);
}
