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
    kSUBCycles          = 1,
    kMODCycles          = 1,
    kMULCycles          = 1,
    kDIVCycles          = 1,
    kANDCycles          = 1,
    kORRCycles          = 1,
    kNOTCycles          = 1,
    kXORCycles          = 1,
    kNOPCycles          = 0,
    kMOVCycles          = 1
};

// Forward class definitions
class VirtualMachine;

class ALU
{
public:
    ALU(VirtualMachine *vm);
    ~ALU();
    
    bool init();
    
    // Operational: must return the timing
    size_t dataProcessing(bool I, bool S, char op, char s, char d, reg_t &op2);
    
    // Access to the barrel shifter.
    static bool shift(reg_t &offset, reg_t val, reg_t shift, reg_t op);
    
private:
    void shiftOffset(reg_t &offset, bool immediate);
    
    VirtualMachine *_vm;
    bool _carry_out;
};

#endif
