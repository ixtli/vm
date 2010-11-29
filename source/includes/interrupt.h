#ifndef _INTERRUPT_H_
#define _INTERRUPT_H_

#include "global.h"

enum SWIntInstructionMask {
    kSWIntCommentMask   = 0x00FFFFFF
};

// Forward struct definitions
struct IntFlags;

// Forward class definitions
class VirtualMachine;

class InterruptController
{
public:
    InterruptController(VirtualMachine *vm, cycle_t timing = 0);
    ~InterruptController();
    
    bool init();
    
    // Operational: must return the timing
    cycle_t swint(const IntFlags &flags);
private:
    cycle_t _swint_cycles;
    VirtualMachine *_vm;
};

#endif
