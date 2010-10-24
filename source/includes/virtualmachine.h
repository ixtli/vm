#ifndef _VIRTUALMACHINE_H_
#define _VIRTUALMACHINE_H_

#include <signal.h>

#include "global.h"
#include "server.h"
#include "mmu.h"

#define kIntSize        sizeof(unsigned int)

#define kWriteCommand   "WRITE"
#define kReadCommand    "READ"
#define kRangeCommand   "RANGE"

// Memory constants, in bytes
#define kDefaultStackSpace 256
#define kMinimumMemorySize 524288 

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
    kMMUReadClocks = 100,
    kMMUWriteClocks = 100
};

enum VMRegisterDefaults {
    kPSRDefault = 0x4000
};

enum InstructionOpCodeMasks {
    kOpCodeMask         = 0x0F000000,
    kDataProcessingMask = 0x0C000000,
    kSingleTransferMask = 0x08000000,
    kBranchMask         = 0x04000000,
    kFloatingPointMask  = 0x01000000,
    kReservedSpaceMask  = 0x06000000,
    kSWInterruptMask    = 0x00000000
};

enum DataProcessingMasks {
    kDPIFlagMask        = 0x02000000,
    kDPOpCodeMask       = 0x01E00000,
    kDPSFlagMask        = 0x00100000,
    kDPSourceMask       = 0x000F8000,
    kDPDestMask         = 0x00007C00,
    kDPOperandTwoMask   = 0x000003FF
};

enum ShifterMasks {
    kShiftImmediateMask = 0x000000FF,
    kShiftRotateMask    = 0x00000300,
    kShiftRmMask        = 0x0000001F,
    kShiftOpMask        = 0x00000060,
    kShiftRsMask        = 0x00000380
};

enum ShiftOperations {
    kShiftLSL, kShiftLSR, kShiftASR, kShiftROR
};

enum DataProcessingOpCodes {
    kADD, kSUB, kMOD, kMUL, kDIV, kAND, kORR, kNOT,
    kXOR, kCMP, kCMN, kTST, kTEQ, kMOV, kBIC, kNOP
};

enum DataProcessingTimings {
    kADDCycles          = 1,
    kANDCycles          = 1
};

// Forward class definitions
class MonitorServer;

class VirtualMachine
{
public:
    VirtualMachine();
    ~VirtualMachine();

    bool init(const char *mem_in, const char *mem_out, size_t mem_size);
    void run();
    
    // Helper methods that might be nice for other things...
    inline unsigned int *selectRegister(char val);
    
    // The following is a hack, I think!
    volatile sig_atomic_t terminate;
    
    pthread_mutex_t waiting;
    // The following are READ/WRITE LOCKED by 'waiting'
    char *operation, *response;
    size_t opsize, respsize;
private:
    bool evaluateConditional();
    size_t execute();
    void shiftOffset(unsigned int &offset, bool immediate);
    size_t dataProcessing(bool I, bool S, char op, char s, char d,
        unsigned int &op2);
    
    
    MMU *mmu;
    MonitorServer *ms;
    const char *dump_path;
    
    // registers modifiable by client
    unsigned int _r[16], _pq[2], _pc, _psr, _cs, _ds, _ss;
    unsigned int _fpsr, _fpr[8];
    
    // storage registers
    unsigned int _ir;
    
    // information about memory
    size_t _int_table_size, _int_function_size;
    
    // state info
    size_t _cycle_count;
};

// Main VM context
extern VirtualMachine *vm;

#endif
