#ifndef _FPU_H_
#define _FPU_H_

#include "global.h"

// Forward class definitions
class VirtualMachine;

class FPU
{
public:
    FPU(VirtualMachine *vm);
    ~FPU();
    
    bool init();
    
    // Operational: must return the timing
    
private:
    VirtualMachine *_vm;
};

#endif
