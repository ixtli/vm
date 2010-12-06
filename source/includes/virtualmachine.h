#ifndef _VIRTUALMACHINE_H_
#define _VIRTUALMACHINE_H_

#include <pthread.h>

#include "global.h"

#define kWriteCommand   "WRITE"
#define kReadCommand    "READ"
#define kRangeCommand   "RANGE"
#define kExecCommand    "EXEC"
#define kStatCommand    "STATUS"
#define kContCommand    "CONT"
#define kStepCommand    "STEP"

#define kDefaultBreakCount 5
#define kMinimumMemorySize 1024
#define kDefaultPipelineStages 5

enum VMRegisterCounts {
    kGeneralRegisters = 16,
    kPQRegisters = 2,
    kFPRegisters = 8
};

enum VMSizeMasks {
    kZero               = 0x0,
    kOne                = 0x1,
    kNybbleMask         = 0xF,
    kByteMask           = 0xFF,
    kHalfWordMask       = 0xFFFF,
    kWordMask           = 0xFFFFFFFF
};

enum VMMathMasks {
    kSignBitMask        = 0x80000000,
    kMSBMask            = 0x80000000,
    kLSBMask            = 0x00000001
};

enum VMPSRBits {
    kPSRZBit            = 0x00000001,
    kPSRCBit            = 0x00000002,
    kPSRNBit            = 0x00000004,
    kPSRVBit            = 0x00000008
};

#define NVCZ_MASK (kPSRNBit | kPSRVBit | kPSRCBit | kPSRZBit)

enum VMOpPrefixConditions {
    kCondEQ, kCondNE, kCondCS, kCondCC, kCondMI, kCondPL, kCondVS, kCondVC, 
    kCondHI, kCondLS, kCondGE, kCondLT, kCondGT, kCondLE, kCondAL, kCondNV
};

enum VMRegisterCodes {
    kR0Code, kR1Code, kR2Code, kR3Code, kR4Code, kR5Code, kR6Code, kR7Code,
    kR8Code, kR9Code, kR10Code, kR11Code, kR12Code, kR13Code, kR14Code, kR15Code,
    kPQ0Code, kPQ1Code, kPCCode, kPSRCode, kCSCode, kDSCode, kSSCode, kFPSRCode,
    kFPR0Code, kFPR1Code, kFPR2Code, kFPR3Code, kFPR4Code, kFPR5Code, kFPR6Code,
    kFPR7Code, kVMRegisterMax
};

enum VMComponantTimings {
    kMMUReadClocks      = 100,
    kMMUWriteClocks     = 100,
    kMMUAbortCycles     = 10
};

enum VMRegisterDefaults {
    kPSRDefault = 0x0000
};

enum InstructionOpCodeMasks {
    kConditionCodeMask  = 0xF0000000,
    kOpCodeMask         = 0x0F000000,
    kDataProcessingMask = 0x0C000000,
    kSingleTransferMask = 0x04000000,
    kBranchMask         = 0x04000000,
    kFloatingPointMask  = 0x01000000,
    kReservedSpaceMask  = 0x06000000,
    kSWInterruptMask    = 0x00000000
};

enum BranchMasks {
    kBranchLBitMask     = 0x01000000,
    kBranchOffsetMask   = 0x00FFFFFF
};

// Forward class and struct definitions
struct PipelineData;
struct ALUTimings;
struct MachineStatus;
struct MachineDescription;
struct CacheDescription;

class MonitorServer;
class InterruptController;
class ALU;
class MMU;
class FPU;
class InstructionPipeline;

class VirtualMachine
{
public:
    VirtualMachine();
    ~VirtualMachine();
    
    bool init(const char *config);
    void run();
    void step();
    void installJumpTable(reg_t *data, reg_t size);
    void installIntFunctions(reg_t *data, reg_t size);
    bool loadProgramImage(const char *path, reg_t addr);
    
    // These are exposed because they are readonly, and thus
    // threadsafe
    void statusStruct(MachineStatus &s);
    void descriptionStruct(MachineDescription &d);
    char *statusString(size_t &len);
    void readWord(reg_t addr, reg_t &val);
    void readRange(reg_t start, reg_t end, bool hex, char **ret);
    const char *readOnlyMemory(reg_t &size);
    
    // Helper methods that might be nice for other things...
    inline reg_t selectRegister(const char val)
    {
        reg_t *temp = demuxRegID(val);
        if (!temp) return ((reg_t) -1);
        return (*temp);
    }
    
    inline void incCycleCount(cycle_t val)
    {
        _cycle_count += val;
        
        // Set this up to break out of possible infinite loops
        if (_cycle_trap)
            if (_cycle_count > _cycle_trap)
                trap("Cycle count unlikely to be this large.");
    }
    
    inline void setProgramCounter(reg_t val)
    {
        _pc = val;
    }
    
    // Execution control
    void addBreakpoint(reg_t addr);
    reg_t deleteBreakpoint(reg_t index);
    
    // Every other part of the machine can play with this at will
    // thus it shouldn't ever be read from except at the very
    // beginning of the execution phase
    reg_t _psr;
    bool supervisor, fex;
    
    // The following should be READ/WRITE LOCKED by 'server_mutex'
    char *operation, *response;
    size_t opsize, respsize;
    //////////////////////////////////////////////////////////////
private:
    // Helper functions to keep code clean and relocatable
    bool configure(const char *c_path, ALUTimings &at);
    void resetSegmentRegisters();
    void resetGeneralRegisters();
    void setMachineDefaults();
    void relocateBreakpoints();
    bool configurePipeline();
    reg_t *demuxRegID(const char id);
    
    // Six stage pipe (conditional evalution)
    void evaluateConditional(PipelineData *d);
    
    // Five stage pipe (writeback)
    void writeBack(PipelineData *d);
    
    // Four stage pipe (forwarding)
    void fetchInstruction(PipelineData *d);
    void decodeInstruction(PipelineData *d);
    void executeInstruction(PipelineData *d);
    void memoryAccess(PipelineData *d);
    
    // Single stage pipe
    void doInstruction(PipelineData *d);
    
    // Server helper functions
    void waitForClientInput();
    cycle_t execute();
    void eval(char *op);
    
    void trap(const char *error);
    
    // Units
    MMU *mmu;
    ALU *alu;
    FPU *fpu;
    InstructionPipeline *pipe;
    InterruptController *icu;
    
    // Server
    MonitorServer *ms;
    
    // VM state
    char *_program_file, *_dump_file;
    bool _print_branch_offset, _print_instruction;
    reg_t _length_trap;
    cycle_t _cycle_trap;
    
    // execution control
    reg_t _breakpoint_count;
    reg_t *_breakpoints;
    
    // Machine info
    char _pipe_stages, _caches;
    bool _forwarding;
    reg_t _mem_size, _read_cycles, _write_cycles, _stack_size;
    CacheDescription *_cache_desc;
    
    // registers modifiable by client
    reg_t _r[kGeneralRegisters], _pq[kPQRegisters], _pc, _cs, _ds, _ss;
    reg_t _fpsr, _fpr[kFPRegisters];
    
    // storage registers
    reg_t _ir;
    
    // information about memory
    reg_t _int_table_size, _int_function_size, _image_size;
    
    // state info
    cycle_t _cycle_count, _swint_cycles, _branch_cycles;
};

// SIGINT flips this to tell everything to turn off
// Must have it declared extern and at file scope so that we can
// read it from anywhere, which is assumed safe because of it's type.
extern "C" {
    extern volatile sig_atomic_t terminate;
}

extern pthread_mutex_t server_mutex;

#endif
