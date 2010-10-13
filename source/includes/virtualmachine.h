#ifndef _VIRTUALMACHINE_H_
#define _VIRTUALMACHINE_H_

#include "global.h"
#include "server.h"
#include "mmu.h"

#define kWriteCommand   "WRITE"
#define kReadCommand    "READ"
#define kRangeCommand   "RANGE"

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
    
    // The following is a hack, I think!
    bool terminate;
    
    pthread_mutex_t waiting;
    // The following are READ/WRITE LOCKED by 'waiting'
    char *operation, *response;
    size_t opsize, respsize;
private:
    MMU *mmu;
    MonitorServer *ms;
};

// Main VM context
extern VirtualMachine *vm;

#endif
