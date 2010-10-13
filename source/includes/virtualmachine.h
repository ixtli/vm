#ifndef _VIRTUALMACHINE_H_
#define _VIRTUALMACHINE_H_

#include "global.h"
#include "server.h"
#include "mmu.h"

enum VMComponantTimings {
    kMMUReadClocks = 100,
    kMMUWriteClocks = 100
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
    
    // The following is a hack!
    bool terminate;
    
private:
    MMU *mmu;
    MonitorServer *ms;
};

// Main VM context
extern VirtualMachine *vm;

#endif
