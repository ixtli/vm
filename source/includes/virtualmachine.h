#ifndef _VIRTUALMACHINE_H_
#define _VIRTUALMACHINE_H_

#include <signal.h>

#include "global.h"
#include "server.h"

#define kWriteCommand   "WRITE"
#define kReadCommand    "READ"
#define kRangeCommand   "RANGE"
#define kExecCommand    "EXEC"

// Memory constants, in bytes
#define kMinimumMemorySize 524288 

// Stack space, in words
#define kDefaultStackSpace 5

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

#define NVCZ_MASK (kPSRNBit & kPSRVBit & kPSRCBit & kPSRZBit)

enum VMOpPrefixConditions {
    kCondEQ, kCondNE, kCondCS, kCondCC, kCondMI, kCondPL, kCondVS, kCondVC, 
    kCondHI, kCondLS, kCondGE, kCondLT, kCondGT, kCondLE, kCondAL, kCondNV
};

enum VMRegisterCodes {
    kR0Code, kR1Code, kR2Code, kR3Code, kR4Code, kR5Code, kR6Code, kR7Code,
    kR8Code, kR9Code, kR10Code, kR11Code, kR12Code, kR13Code, kR14Code, kR15Code,
    kPQ0Code, kPQ1Code, kPCCode, kPSRCode, kCSCode, kDSCode, kSSCode, kFPSRCode,
    kFPR0Code, kFPR1Code, kFPR2Code, kFPR3Code, kFPR4Code, kFPR5Code, kFPR6Code,
    kFPR7Code
};

enum VMComponantTimings {
    kMMUReadClocks      = 100,
    kMMUWriteClocks     = 100,
    kMMUAbortCycles     = 10
};

enum VMRegisterDefaults {
    kPSRDefault = 0x4000
};

enum InstructionOpCodeMasks {
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

// Forward class definitions
class MonitorServer;
class InterruptController;
class ALU;
class MMU;
class FPU;

class VirtualMachine
{
public:
    VirtualMachine();
    ~VirtualMachine();

    bool init(const char *mem_in, const char *mem_out, size_t mem_size);
    void run();
    void installJumpTable(reg_t *data, size_t size);
    void installIntFunctions(reg_t *data, size_t size);
    
    // Helper methods that might be nice for other things...
    inline reg_t *selectRegister(char val)
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
    
    // Every other part of the machine can play with this at will
    // thus it shouldn't ever be read from except at the very
    // beginning of the execution phase
    reg_t _psr;
    bool supervisor, fex;
    
    // SIGINT flips this to tell everything to turn off
    volatile sig_atomic_t terminate;
    
    pthread_mutex_t waiting;
    // The following are READ/WRITE LOCKED by 'waiting'
    char *operation, *response;
    size_t opsize, respsize;
private:
    bool evaluateConditional();
    size_t execute();
    void eval(char *op);
    
    MMU *mmu;
    ALU *alu;
    FPU *fpu;
    InterruptController *icu;
    MonitorServer *ms;
    const char *dump_path;
    
    // registers modifiable by client
    reg_t _r[16], _pq[2], _pc, _cs, _ds, _ss;
    reg_t _fpsr, _fpr[8];
    
    // storage registers
    reg_t _ir;
    
    // information about memory
    size_t _int_table_size, _int_function_size;
    
    // state info
    size_t _cycle_count;
};

// Main VM context
extern VirtualMachine *vm;

#endif
