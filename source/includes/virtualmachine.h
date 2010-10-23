#ifndef _VIRTUALMACHINE_H_
#define _VIRTUALMACHINE_H_

#include <signal.h>

#include "global.h"
#include "server.h"
#include "mmu.h"

#define kWriteCommand   "WRITE"
#define kReadCommand    "READ"
#define kRangeCommand   "RANGE"

// Memory constants, in bytes
#define kDefaultStackSpace 256
#define kMinimumMemorySize 524288

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
    volatile sig_atomic_t terminate;
    
    pthread_mutex_t waiting;
    // The following are READ/WRITE LOCKED by 'waiting'
    char *operation, *response;
    size_t opsize, respsize;
private:
    
    
    MMU *mmu;
    MonitorServer *ms;
    const char *dump_path;
    
    // registers modifiable by client
    unsigned int _r[16], _pq[2], _pc, _psr, _cs, _ds, _ss;
    
    // storage registers
    unsigned int ir;
    
    // information about memory
    size_t _int_table_size, _int_function_size;
    
    // state info
    size_t _cycle_count;
};

// Main VM context
extern VirtualMachine *vm;

#endif
