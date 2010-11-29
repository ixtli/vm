#include "includes/virtualmachine.h"

#include <errno.h>>
#include <signal.h>
#include <string.h>
#include <iostream>

#include "includes/server.h"
#include "includes/interrupt.h"
#include "includes/mmu.h"
#include "includes/alu.h"
#include "includes/fpu.h"
#include "includes/util.h"
#include "includes/luavm.h"
#include "includes/pipeline.h"

// Macros for checking the PSR
#define N_SET     (_psr & kPSRNBit)
#define N_CLEAR  !(_psr & kPSRNBit)
#define V_SET     (_psr & kPSRVBit)
#define V_CLEAR  !(_psr & kPSRVBit)
#define C_SET     (_psr & kPSRCBit)
#define C_CLEAR  !(_psr & kPSRCBit)
#define Z_SET     (_psr & kPSRZBit)
#define Z_CLEAR  !(_psr & kPSRZBit)

#define kDefaultBranchCycles    5

// SIGINT flips this to tell everything to turn off
// Must have it declared extern and at file scope so that we can
// read it form anywhere.  Also it needs to be extern C because it's
// going to be referenced from within a syscal?  Maybe?
extern "C" {
    volatile sig_atomic_t terminate;
}

// Init static mutex
pthread_mutex_t server_mutex;

reg_t *VirtualMachine::demuxRegID(const char id)
{
    if (id < kPQ0Code)
        // This is a general register
        return (&_r[id]);
    
    if (id > kFPSRCode)
        // This is an fpu register
        return (&_fpr[id - kFPR0Code]);
    
    switch (id)
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
        trap("Invalid register selection.\n");
        return (0);
    }
}

const reg_t *VirtualMachine::readOnlyMemory(reg_t &size)
{
    return (mmu->readOnlyMemory(size));
}

void VirtualMachine::evaluateConditional(PipelineData *d)
{
    d->executes = true;
    
    switch (d->condition_code)
    {
        case kCondAL:           // Always
        return;
        
        case kCondEQ:           // Equal
        if (Z_SET) return;      // Z bit set
        break;
        
        case kCondNE:           // Not equal
        if (Z_CLEAR) return;    // Z bit clear
        break;
        
        case kCondCS:           // unsigned higher or same
        if (C_SET) return;
        break;
        
        case kCondCC:           // Unsigned lower
        if (C_CLEAR) return;
        break;
        
        case kCondMI:           // Negative
        if (N_SET) return;
        break;
        
        case kCondPL:           // Positive or Zero
        if (N_CLEAR) return;
        break;
        
        case kCondVS:           // Overflow
        if (V_SET) return;
        break;
        
        case kCondVC:           // No Overflow
        if (V_CLEAR) return;
        break;
        
        case kCondHI:           // Unsigned Higher
        if (C_SET && V_CLEAR) return;
        break;
        
        case kCondLS:           // Unsigned lower or same
        if (C_CLEAR || Z_SET) return;
        break;
        
        case kCondGE:           // Greater or Equal
        if ((N_SET && V_CLEAR) || (N_CLEAR && V_CLEAR)) return;
        break;
        
        case kCondLT:           // Less Than
        if ((N_SET && V_CLEAR) || (N_CLEAR && V_SET)) return;
        break;
        
        case kCondGT:           // Greater Than
        // Z clear, AND "EITHER" N set AND V set, OR N clear AND V clear
        if (Z_CLEAR && (((N_SET && V_SET))||(N_CLEAR && V_CLEAR))) return;
        break;
        
        case kCondLE:           // Less than or equal
        // Z set, or N set and V clear, or N clear and V set
        if (Z_CLEAR || (N_SET && V_CLEAR) || (N_CLEAR && V_SET)) return;
        break;
        
        case kCondNV:           // Never
        default:                // Never
        break;
    }
    
    d->executes = false;
}

VirtualMachine::VirtualMachine()
{
    // Set dynamically allocated variables to NULL so that
    // we don't accidentally free them when destroying this class
    // if an error occurred during allocation or they were
    // intentionally not allocated.
    _program_file = NULL;
    _dump_file = NULL;
    _breakpoints = NULL;
}

VirtualMachine::~VirtualMachine()
{
    // Do this first, just in case
    delete ms;
    printf("Destroying virtual machine...\n");
    mmu->writeOut(_dump_file);
    
    delete mmu;
    delete alu;
    delete fpu;
    delete icu;
    delete pipe;
    
    if (_breakpoints)
        free(_breakpoints);
    
    if (_dump_file)
        free(_dump_file);
    
    if (_program_file)
        free(_program_file);
}

bool VirtualMachine::loadProgramImage(const char *path, reg_t addr)
{
    // Copy command line arguments onto the stack
    
    // Allocate stack space before code
    _ss = addr + (_stack_size << 2);
    printf("Program stack: %u words allocated at %#x\n", _stack_size, _ss);
    
    // Code segment starts right after it
    _cs = _ss + 1;
    printf("Code segment: %#x\n", _cs);
    
    // Load file into memory at _cs
    reg_t image_size = mmu->loadProgramImageFile(path, _cs, true);
    
    // Set data segment after code segment
    _ds = _cs + image_size;
    
    // Report
    printf("Data segment: %#x\n", _ds);
    
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

void VirtualMachine::relocateBreakpoints()
{
    printf("Relocating breakpoints as offsets into _cs... ");
    
    // This simply adds _cs to each breakpoint and
    // bounds checks them to make sure they're still within memory
    for (int i = 0; i < _breakpoint_count; i++)
    {
        if (_breakpoints[i] != 0x0)
            _breakpoints[i] += _cs;
        
        if (_breakpoints[i] > _mem_size)
        {
            fprintf(stderr, "Breakpoint %i relocated outside of memory.\n", i);
            deleteBreakpoint(i);
        }
    }
    printf("Done.\n");
}

bool VirtualMachine::configure(const char *c_path, ALUTimings &at)
{
    
    // Parse the config file
    LuaVM *lua = new LuaVM();
    lua->init();
    int ret = lua->exec(c_path, 0);
    
    if (ret)
    {
        // Something went wrong
        fprintf(stderr, "Execution of script failed.");
        delete lua;
        return (true);
    }
    
    // Count the errors for values that must be specified
    int err = 0;
    
    // string locations
    const char *prog_temp, *dump_temp;
    
    // Grab the config data from the global state of the VM post exec
    err += lua->getGlobalField("memory_size", kLUInt, &_mem_size);
    err += lua->getGlobalField("read_cycles", kLUInt, &_read_cycles);
    err += lua->getGlobalField("write_cycles", kLUInt, &_write_cycles);
    
    // If something required wasn't set, error out
    if (err)
    {
        fprintf(stderr, "A required value was not set in the config file.\n");
        delete lua;
        return (true);
    }
    
    // Optional arguments
    lua->getGlobalField("stack_size", kLUInt, &_stack_size);
    lua->getGlobalField("swint_cycles", kLUInt, &_swint_cycles);
    lua->getGlobalField("branch_cycles", kLUInt, &_branch_cycles);
    lua->getGlobalField("program", kLString, &prog_temp);
    lua->getGlobalField("memory_dump", kLString, &dump_temp);
    lua->getGlobalField("print_instruction", kLBool, &_print_instruction);
    lua->getGlobalField("print_branch_offset", kLBool, &_print_branch_offset);
    lua->getGlobalField("program_length_trap", kLUInt, &_length_trap);
    lua->getGlobalField("machine_cycle_trap", kLUInt, &_cycle_trap);
    lua->getGlobalField("stages", kLUInt, &_pipe_stages);
    
    // Error check pipe stages
    if (_pipe_stages != 1 && _pipe_stages != 4 && _pipe_stages != 5)
    {
        printf("Warning: Unsupported pipeline length %u.\n", _pipe_stages);
        _pipe_stages = kDefaultPipelineStages;
    }
    
    // Deal with ALU timings
    if (lua->openGlobalTable("alu_timings") != kLuaUnexpectedType)
    {
        // User defined the table, so pull all the ops out of it
        for (int i = 0; i < kDPOpcodeCount; i++)
            lua->getTableField(DPOpMnumonics[i], kLUInt, (void *) &at.op[i]);
        
        // Clean up
        lua->closeTable();
    }
    
    // Copy strings, because they wont exist after we free the lua VM
    _program_file = (char *)malloc(sizeof(char) * strlen(prog_temp) + 1);
    strcpy(_program_file, prog_temp);
    _dump_file = (char *)malloc(sizeof(char) * strlen(dump_temp) + 1);
    strcpy(_dump_file, dump_temp);
    
    // Get breakpoint count
    lua->getGlobalField("break_count", kLUInt, &_breakpoint_count);
    // Allocate memory to hold them all
    _breakpoints = (reg_t *)malloc(sizeof(reg_t) * _breakpoint_count);
    if (lua->openGlobalTable("breakpoints") != kLuaUnexpectedType)
    {
        // Pull all the values out of it
        reg_t c;
        size_t max = lua->lengthOfCurrentObject();
        
        // Get all the breakpoints, remember lua lists are ONE-INDEXED
        // so this for loop should look a little unnatural
        for (int i = 1; i <= max; i++)
        {
            lua->getTableField(i, kLUInt, &c);
            addBreakpoint(c << 2);
        }
    }
    
    // clean up 
    delete lua;
    return (false);
}

void VirtualMachine::setMachineDefaults()
{
    // Most stuff gets set to zero
    _print_instruction = false;
    _print_branch_offset = false;
    _cycle_count = 0;
    _pc = 0;
    _fpsr = 0;
    _length_trap = 0;
    _cycle_trap = 0;
    
    // Others have "hardcoded" defaults
    _breakpoint_count = kDefaultBreakCount;
    _branch_cycles = kDefaultBranchCycles;
    _psr = kPSRDefault;
}

bool VirtualMachine::init(const char *config)
{
    // Initialize server_mutex for MonitorServer
    pthread_mutex_init(&server_mutex, NULL);
    
    // Initialize command and status server
    ms = new MonitorServer(this);
    if (ms->init())
        return (true);
    if (ms->run())
        return (true);
    
    // Initialize actual machine
    printf("Initializing virtual machine: ");
    printf("%i %lu-byte registers.\n", kVMRegisterMax, kRegSize);
    
    // Set defaults
    setMachineDefaults();
    
    // Configure the VM using the config file
    ALUTimings _aluTiming;
    if (configure(config, _aluTiming))
    {
        fprintf(stderr, "VM configuration failed.\n");
        return (true);
    }
    
    // Start up ALU
    alu = new ALU(this);
    if (alu->init(_aluTiming)) return (true);
    
    // Start up FPU
    fpu = new FPU(this);
    if (fpu->init()) return (true);
    
    // Init memory
    mmu = new MMU(this, _mem_size, _read_cycles, _write_cycles);
    if (mmu->init()) return (true);
    
    // Init instruction pipeline
    pipe = new InstructionPipeline(_pipe_stages, this);
    if (pipe->init()) return (true);
    if (configurePipeline()) return (true);
    
    // Load interrupt controller
    icu = new InterruptController(this, _swint_cycles);
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
    
    // Relocate breakpoints now that we have our environment loaded
    relocateBreakpoints();
    
    // We can now start the Fetch EXecute cycle
    fex = true;
    
    // No errors
    return (false);
}

// Evaluate a destructive client operation, like write word
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
    } else if (strcmp(pch, kExecCommand) == 0) {
        /*reg_t instruction;
        pch = strtok(NULL, " ");
        
        if (pch)
            instruction = (reg_t) _hex_to_int(pch);
        else
            instruction = 0;
        
        reg_t ir = _ir;
        _ir = instruction;
        execute();
        _ir = ir;
        */
        fprintf(stderr, "EXEC operation not currently supported.\n");
        response = (char *)malloc(sizeof(char) * strlen(op));
        strcpy(response, op);
        respsize = strlen(response);
        return;
    }
    
    // If nothing worked
    respsize = 0;
}

void VirtualMachine::statusStruct(MachineStatus &s)
{
    // Populate status struct
    s.type = kStatusMessage;
    s.supervisor = supervisor ? 1 : 0;
    s.cycles = _cycle_count;
    s.psr = _psr;
    s.pc = _pc;
    s.ir = _ir;
    s.cs = _cs;
    s.ds = _ds;
    s.ss = _ss;
    s.pq[0] = _pq[0];
    s.pq[1] = _pq[1];
    for (int i = 0; i < kGeneralRegisters; i++)
        s.r[i] = _r[i];
    for (int i = 0; i < kFPRegisters; i++)
        s.f[i] = _fpr[i];
}

void VirtualMachine::descriptionStruct(MachineDescription &d)
{
    // Populate description struct
    d.type = kMachineDescription;
    d.int_table_length = _int_table_size;
    d.int_fxn_length = _int_function_size;
    d.mem_size = _mem_size;
}

char *VirtualMachine::statusString(size_t &len)
{
    // Format human readable
    char temp[1024];
    sprintf(temp, "Machine Status: ");
    if (supervisor)
        sprintf(temp+strlen(temp), "Supervisor mode\n");
    else
        sprintf(temp+strlen(temp), "User mode\n");
    sprintf(temp+strlen(temp),  "Cycle Count: %lu\n", _cycle_count);
    sprintf(temp+strlen(temp),  "Program Status Register: %#x\n", _psr);
    sprintf(temp+strlen(temp),  "N: %s V: %s C: %s Z: %s\n", N_SET ? "1" : "0",
        V_SET ? "1" : "0", C_SET ? "1" : "0", Z_SET ? "1" : "0");
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
    char *ret = (char *)malloc(sizeof(char) * strlen(temp) + 1);
    strcpy(ret, temp);
    len = strlen(ret);
    return (ret);
}

void VirtualMachine::trap(const char *error)
{
    if (error) fprintf(stderr, "CPU TRAP: %s\n", error);
    _ir = 0xEF000000;
    pipe->printState();
    pipe->invalidate();
    fex = false;
}

void VirtualMachine::waitForClientInput()
{
    // Is the server even running?
    if (!ms->isRunning()) return;
    
    // Busy wait for the other thread to initialize and aquire the lock before
    // we go tosleep on it.  If we get this lock before the other thread starts
    // and begins listening, it will cause a deadlock.
    while (!ms->ready())
    { }
    
    while (!terminate && !fex)
    {
        // Wait for something to happen
        pthread_mutex_lock(&server_mutex);
        // SIGINT may have been sent while we were asleep,
        // or the server may have been told to step or cont
        if (terminate || fex)
        {
            pthread_mutex_unlock(&server_mutex);
            return;
        }
        
        // If we get here, we have a job to do
        if (!operation) continue;
        eval(operation);
        
        // we're done
        pthread_mutex_unlock(&server_mutex);
    }
}

void VirtualMachine::step()
{
    // Can't step if we're not broken.
    if (fex) return;
    
    pipe->step();
}

bool VirtualMachine::configurePipeline()
{
    printf("Configuring ");
    switch (_pipe_stages)
    {
        case 1:
        printf("single stage pipeline... ");
        if (pipe->registerStage(&VirtualMachine::doInstruction))
            return (true);
        _forwarding = true;
        break;
        
        case 5:
        printf("five stage pipeline... ");
        if (pipe->registerStage(&VirtualMachine::fetchInstruction))
            return (true);
        if (pipe->registerStage(&VirtualMachine::decodeInstruction))
            return (true);
        if (pipe->registerStage(&VirtualMachine::executeInstruction))
            return (true);
        if (pipe->registerStage(&VirtualMachine::memoryAccess))
            return (true);
        if (pipe->registerStage(&VirtualMachine::writeBack))
            return (true);
        _forwarding = false;
        break;
        
        case 4:
        default:
        printf("four stage pipeline (forwarding)... ");
        if (pipe->registerStage(&VirtualMachine::fetchInstruction))
            return (true);
        if (pipe->registerStage(&VirtualMachine::decodeInstruction))
            return (true);
        if (pipe->registerStage(&VirtualMachine::executeInstruction))
            return (true);
        if (pipe->registerStage(&VirtualMachine::memoryAccess))
            return (true);
        _forwarding = true;
        break;
    }
    printf("Done.\n");
    return (false);
}

void VirtualMachine::run()
{
    printf("Starting execution at %#x\n", _pc);
    
    // Logic for the fetch -> execute cycle
    while (fex)
    {
        // Check breakpoints on the CURRENT instruction, that is, before
        // advancing the pipeline
        for (int i = 0; i < _breakpoint_count; i++)
        {
            if (_breakpoints[i] == _pc && _pc != 0x0)
            {
                printf("Breakpoint %i at instruction %u (%#x).\n",
                    i, _breakpoints[i] - _cs, _breakpoints[i]);
                
                // stop execution
                fex = false;
                // sit around
                waitForClientInput();
                
                // SIGINT could have happened during this time, so test for it
                if (terminate)
                {
                    printf("Exiting...\n");
                    return;
                }
            }
        }
        
        if(pipe->cycle())
            trap("Pipeline exception.\n");
    }
    
    // Idle and only close server after SIGINT
    while (!terminate)
        waitForClientInput();
    
    printf("Exiting...\n");
}

void VirtualMachine::installJumpTable(reg_t *data, reg_t size)
{
    incCycleCount(mmu->writeBlock(0x0, data, size));
    _int_table_size = size;
    printf("(%ub table @ 0x0) ", size);
}

void VirtualMachine::installIntFunctions(reg_t *data, reg_t size)
{
    incCycleCount(mmu->writeBlock(_int_table_size, data, size));
    _int_function_size = size;
    printf("(%ub functions @ %#x) ", size, (reg_t)_int_table_size);
}

void VirtualMachine::readWord(reg_t addr, reg_t &val)
{
    incCycleCount(mmu->readWord(addr, val));
}

void VirtualMachine::readRange(reg_t start, reg_t end, bool hex, char **ret)
{
    incCycleCount(mmu->readRange(start, end, hex, ret));
}

void VirtualMachine::addBreakpoint(reg_t addr)
{
    // Error check
    if (!_breakpoints || !_breakpoint_count) return;
    
    if (addr == 0x0)
    {
        fprintf(stderr, "Cannot set breakpoint at 0x0. Ignoring.\n");
        return;
    }
    
    if (addr >= _mem_size)
    {
        fprintf(stderr, "Attempt to set breakpoint outside memory: %#x.\n", addr);
        return;
    }
    
    for (int i = 0; i < _breakpoint_count; i++)
    {
        if (_breakpoints[i] == 0x0)
        {
            _breakpoints[i] = addr;
            printf("Breakpoint %i set: %#x\n", i, addr);
            return;
        }
    }
    
    fprintf(stderr, "Could not set breakpoint: Array full.");
}

reg_t VirtualMachine::deleteBreakpoint(reg_t index)
{
    if (!_breakpoints || !_breakpoint_count) return (0x0);
    
    if (index >= _breakpoint_count)
    {
        fprintf(stderr, "Delete breakpoint: Bounds exception.\n");
        return (0x0);
    }
    
    if (_breakpoints[index] == 0x0)
    {
        fprintf(stderr, "No breakpoint at index %u\n.", index);
        return (0x0);
    }
    
    reg_t ret = _breakpoints[index];
    _breakpoints[index] = 0x0;
    printf("Breakpoints %u deleted.\n", index);
    return (ret);
}

// Five stage pipe (writeback)
void VirtualMachine::writeBack(PipelineData *d)
{
    if (!d)
    {
        trap("Invalid writeback parameter.\n");
        return;
    }
    
    if (!pipe->isSquashed())
    {
        pipe->unlock();
        return;
    }
    
    switch (d->instruction_class)
    {
        case kDataProcessing:
        if (!alu->result()) break;
        // if MUL
        if (d->flags.dp.op == kMUL)
        {
            _pq[0] = alu->result();
            _pq[1] = alu->top();
        } else {
            *(demuxRegID(d->flags.dp.rd)) = alu->result();
        }
        break;
        
        case kSingleTransfer:
        // wait on dest register if it's a load
        if (d->flags.st.l)
            *(demuxRegID(d->flags.st.rd)) = mmu->readOut();
        
        // write address back into rs if w == 1
        if (d->flags.st.w)
            *(demuxRegID(d->flags.st.rs)) = d->flags.st.addr;
        break;
        
        case kFloatingPoint:
            *(demuxRegID(d->flags.fp.s + kFPR0Code)) = fpu->output();
            // TODO: add another output register
        default:
        break;
    }
    
    pipe->unlock();
}

// Four stage pipe (forwarding)
void VirtualMachine::fetchInstruction(PipelineData *d)
{
    if (!d)
    {
        trap("Invalid fetch parameter.\n");
        return;
    }
    
    // Fetch PC instruction into IR and increment the pc
    incCycleCount(mmu->readWord(_pc, _ir));
    
    // Set metadata
    d->instruction = _ir;
    d->location = _pc;
    
    // Print instruction if requested
    if (_print_instruction)
        printf("PC: %#X\t\t\t%#X\n", _pc, _ir);
    
    // Increment the program counter.
    // NOTE: The effect of this action being taken here is that the PC
    // always points to the NEXT instruction to be executed, that is
    // the location of the instruction ONE AHEAD of the ir register
    _pc += kRegSize;
    
    // Set this up to break out of possibly invalid jumps
    if (_length_trap)
        if (_pc > (_length_trap + _cs))
            trap("Program unlikely to be this long.");
}

void VirtualMachine::decodeInstruction(PipelineData *d)
{
    if (!d)
    {
        trap("Invalid decode parameter.\n");
        return;
    }
    
    // Parse the condition code
    // Get the most significant nybble of the instruction by masking
    // then move it from the MSN into the LSN 
    d->condition_code = (_ir & kConditionCodeMask) >> 28;
    
    // Parse the Operation Code
    // Test to see if it has a 0 in the first place of the opcode
    if ((_ir & 0x08000000) == 0x0)
    {
        // We're either a data processing or single transfer operation
        // Test to see if there is a 1 in the second place of the opcode
        if (_ir & kSingleTransferMask)
        {
            // We're a single transfer
            d->instruction_class = kSingleTransfer;
            d->flags.st.i = (_ir & kSTIFlagMask) ? 1 : 0;
            d->flags.st.l = (_ir & kSTLFlagMask) ? 1 : 0;
            d->flags.st.w = (_ir & kSTWFlagMask) ? 1 : 0;
            d->flags.st.b = (_ir & kSTBFlagMask) ? 1 : 0;
            d->flags.st.u = (_ir & kSTUFlagMask) ? 1 : 0;
            d->flags.st.p = (_ir & kSTPFlagMask) ? 1 : 0;
            d->flags.st.rs = (_ir & kSTSourceMask) >> 15;
            d->flags.st.rd = (_ir & kSTDestMask) >> 10;
            d->flags.st.offset = (_ir & kSTOffsetMask);
            
            // wait on dest register if it's a load
            if (d->flags.st.l)
                pipe->waitOnRegister(d->flags.st.rd);
            
            // wait on base if there is writeback
            if (d->flags.st.w)
                pipe->waitOnRegister(d->flags.st.rs);
            
        } else {
            // Only other case is a data processing op
            // extract all operands and flags
            d->instruction_class = kDataProcessing;
            d->flags.dp.i = (_ir & kDPIFlagMask) ? 1 : 0;
            d->flags.dp.s = (_ir & kDPSFlagMask) ? 1 : 0;
            d->flags.dp.op =  ( (_ir & kDPOpCodeMask) >> 21 );
            d->flags.dp.rs =  ( (_ir & kDPSourceMask) >> 15 );
            d->flags.dp.rd = ( (_ir & kDPDestMask) >> 10);
            d->flags.dp.offset = ( (_ir & kDPOperandTwoMask) );
            
            if (d->flags.dp.op == kMUL)
            {
                // wait on PQ registers if it's a mul
                pipe->waitOnRegister(kPQ0Code);
                pipe->waitOnRegister(kPQ1Code);
            } else {
                // wait on dest register
                pipe->waitOnRegister(d->flags.st.rd);
            }
        
            
        }
        return;
    }
    
    // Are we a branch?
    if ((_ir & kBranchMask) == 0x0) {
        // We could be trying to execute something in reserved space
        if (_ir & kReservedSpaceMask == 0x0)
        {
            d->instruction_class = kReserved;
        } else {
            d->instruction_class = kBranch;
            // We're a branch
            
            if ( _ir & kBranchLBitMask)
                d->flags.b.link = true;
            
            // Left shift the address by two because instructions
            // are word-aligned
            int temp = (_ir & kBranchOffsetMask) << 2;
            
            // Temp is now a 25-bit number, but needs to be sign extended to
            // 32 bits, so we do this with a little struct trick that will
            // PROBABLY work everywhere.
            struct {signed int x:25;} s;
            s.x = temp;
            d->flags.b.offset = s.x;
        }
        
        return;
    }
    
    if ((_ir & kFloatingPointMask) == 0x0) {
        // We're a floating point operation
        d->instruction_class = kFloatingPoint;
        d->flags.fp.op = ((_ir & kFPOpcodeMask) >> 20);
        d->flags.fp.s = ((_ir & kFPsMask) >> 17);
        d->flags.fp.d = ((_ir & kFPdMask) >> 14);
        d->flags.fp.n = ((_ir & kFPnMask) >> 11);
        d->flags.fp.m = ((_ir & kFPmMask) >> 8);
        
        // TODO: Optimize this
        pipe->waitOnRegister(d->flags.fp.s + kFPR0Code);
        pipe->waitOnRegister(d->flags.fp.d + kFPR0Code);
        
        return;
    }
    
    // We're a SW interrupt
    // pc is always saved in r15 before branching
    d->instruction_class = kInterrupt;
    d->flags.i.comment = (_ir & kSWIntCommentMask);
}

void VirtualMachine::executeInstruction(PipelineData *d)
{
    if (!d)
    {
        trap("Invalid execute parameter.\n");
        return;
    }
    
    // If the cond code precludes execution of the op, don't bother
    evaluateConditional(d);
    
    if (!d->executes)
    {
        pipe->squash();
        return;
    }
    
    pipe->lock();
    
    switch (d->instruction_class)
    {
        case kDataProcessing:
        alu->dataProcessing(d->flags.dp);
        // release the register if there's no writeback stage
        if (!_forwarding) writeBack(d);
        break;
        
        case kSingleTransfer:
        alu->singleTransfer(d->flags.st);
        break;
        
        case kBranch:
        // Store current pc in the link register (r15)
        if (d->flags.b.link) _r[15] = _pc;
        
        // If we need help debugging
        if (_print_branch_offset) printf("BRANCH: %i\n", d->flags.b.offset);
        
        // Add the computed address to the pc and make sure it's not < 0
        d->flags.b.offset += (signed int)_pc;
        
        // Don't jump to a negative offset
        if (d->flags.b.offset < 0)
            _pc = 0;
        else
            _pc = d->flags.b.offset;
        
        pipe->invalidate();
        break;
        
        case kInterrupt:
        _r[15] = _pc;
        icu->swint(d->flags.i);
        pipe->invalidate();
        break;
        
        case kFloatingPoint:
        fpu->execute(d->flags.fp);
        if (!_forwarding) writeBack(d);
        break;
        
        case kReserved:
        default:
        trap("Unknown or reserved opcode.\n");
        break;
    }
}

void VirtualMachine::memoryAccess(PipelineData *d)
{
    if (!d)
    {
        trap("Invalid memory access parameter.\n");
        return;
    }
    
    switch (d->instruction_class)
    {
        case kSingleTransfer:
        mmu->singleTransfer(d->flags.st);
        if (!_forwarding) writeBack(d);
        break;
        
        default:
        break;
    }
}

// Single stage pipe
void VirtualMachine::doInstruction(PipelineData *d)
{
    
}
