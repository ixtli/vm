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

// Forward struct definitions
struct FPFlags;

// Forward class definitions
class VirtualMachine;

class FPU
{
public:
    FPU(VirtualMachine *vm);
    ~FPU();
    
    bool init();
    
    inline reg_t output()
    {
        return (_output);
    }
    
    // Operational: must return the timing
    cycle_t execute(const FPFlags &flags);
private:
    reg_t _output;
    VirtualMachine *_vm;
};

#endif
