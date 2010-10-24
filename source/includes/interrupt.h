#ifndef _INTERRUPT_H_
#define _INTERRUPT_H_

#include "global.h"

enum SWIntInstructionMask {
    kSWIntCommentMask   = 0x00FFFFFF
};

enum SWIntTiming {
    kSWIntUserMode      = 10
};

// Forward class definitions
class VirtualMachine;

class InterruptController
{
public:
    InterruptController(VirtualMachine *vm);
    ~InterruptController();
    
    bool init();
    
    // Operational: must return the timing
    size_t swint(reg_t comment);
private:
    VirtualMachine *_vm;
};

#endif
