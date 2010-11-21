#ifndef _SERVER_H_
#define _SERVER_H_

#include "global.h"

// port to listen on
#define PORT "1337"

// How many pending connections do we tell to wait before we
// establish a TCP connection with them?
#define BACKLOG 10

#define MAX_CONNECTIONS 10

enum ServerMessageTypes
{
    kStreamHandshake,
    kStatusMessage,
    kMemoryRange
};

typedef struct StreamHandshake
{
    char type;
};

typedef struct MachineStatus
{
    char type;
    reg_t supervisor;
    cycle_t cycles;
    reg_t psr, pc, ir, cs, ds, ss;
    reg_t pq[2];
    reg_t r[kGeneralRegisters];
    reg_t f[kFPRegisters];
};

// Forward class definitions
class VirtualMachine;

class MonitorServer
{
public:
    MonitorServer(VirtualMachine *vm);
    ~MonitorServer();
    
    bool init();
    int run();
    
    bool isRunning();
    bool ready();
    
private:
    VirtualMachine *_vm;
};

#endif

