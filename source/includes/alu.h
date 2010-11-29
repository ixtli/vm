#ifndef _ALU_H_
#define _ALU_H_

#include "global.h"

enum DataProcessingMasks {
    kDPIFlagMask        = 0x02000000,
    kDPOpCodeMask       = 0x01E00000,
    kDPSFlagMask        = 0x00100000,
    kDPSourceMask       = 0x000F8000,
    kDPDestMask         = 0x00007C00,
    kDPOperandTwoMask   = 0x000003FF
};

enum ShifterMasks {
    kShiftType          = 0x00000003,
    kShiftOp            = 0x00000004,
    kShiftRmMask        = 0x00000078,
    kShiftRsMask        = 0x00000380,
    kShiftLiteral       = 0x00000380
};

enum MOVOpShiftMasks {
    kMOVShiftRs         = 0x000000F8,
    kMOVLiteral         = 0x000003F8
};

enum ShiftOperations {
    kShiftLSL, kShiftLSR, kShiftASR, kShiftROR
};

enum DataProcessingOpCodes {
    kADD, kSUB, kMOD, kMUL, kDIV, kAND, kORR, kNOT,
    kXOR, kCMP, kCMN, kTST, kTEQ, kMOV, kBIC, kNOP,
    kDPOpcodeCount
};

static const char *DPOpMnumonics[kDPOpcodeCount] =
{   "ADD", "SUB", "MOD", "MUL", "DIV", "AND", "ORR", "NOT",
    "XOR", "CMP", "CMN", "TST", "TEQ", "MOV", "BIC", "NOP"
};

// Default timing for ANY instruction not specified in config file
#define kDefaultALUTiming   0

struct ALUTimings {
    ALUTimings()
    {
        for (int i = 0; i < kDPOpcodeCount; i++)
            op[i] = kDefaultALUTiming;
    }
    
    inline void copy(ALUTimings &src)
    {
        for (int i = 0; i < kDPOpcodeCount; i++)
            op[i] = src.op[i];
    }
    
    cycle_t op[kDPOpcodeCount];
};

struct DPFlags;
struct STFlags;

// Forward class definitions
class VirtualMachine;

class ALU
{
public:
    ALU(VirtualMachine *vm);
    ~ALU();
    
    bool init(ALUTimings &timings);
    
    // Operational: must return the timing
    cycle_t dataProcessing(DPFlags &instruction);
    cycle_t singleTransfer(STFlags &f);
    
    // Access to the barrel shifter.
    void shiftOffset(reg_t &offset);
    void shiftOffset(reg_t &offset, reg_t &val);
    
    static bool shift(reg_t &offset, reg_t val, reg_t shift, reg_t op);
    
    inline bool result()
    {
        return (_result);
    }
    
    inline reg_t output()
    {
        return (_output);
    }
    
    inline reg_t auxOut()
    {
        return (_aux_out);
    }
    
private:
    VirtualMachine *_vm;
    ALUTimings _timing;
    bool _carry_out, _result;
    reg_t _output, _aux_out;
};

#endif
