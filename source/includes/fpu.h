#ifndef _FPU_H_
#define _FPU_H_

#include "global.h"

enum FPUInstructionMasks {
    kFPOpcodeMask       = 0x00F00000,
    kFPsMask            = 0x000E0000,
    kFPdMask            = 0x0001C000,
    kFPnMask            = 0x00003800,
    kFPmMask            = 0x00000700
};

enum FPUProcessingOpCodes {
    kFAD, kFSB, kFML, kFDV
};

enum FPUProcessingTimings {
    kFADCycles          = 1,
    kFSBCycles          = 1,
    kFMLCycles          = 1,
    kFDVCycles          = 1
}; 

// Forward class definitions
class VirtualMachine;

class FPU
{
public:
    FPU(VirtualMachine *vm);
    ~FPU();
    
    bool init();
    
    // Operational: must return the timing
    cycle_t execute(char op, reg_t &fps, reg_t &fpd, reg_t &fpn, reg_t &fpm);
private:
    VirtualMachine *_vm;
};

#endif
