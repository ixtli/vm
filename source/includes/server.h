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
    kMemoryRange,
    kMachineDescription
};

// Handshake format
typedef struct StreamHandshake
{
    char type;
};

// Structure that describes the settings to the virtual machine
// these can not change while the vm is active and are described
// by the configuration file
typedef struct MachineDescription
{
    char type;
    reg_t int_table_length, int_fxn_length;
    reg_t mem_size;
};

// Structure to sent to describe the current state of the machine
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

// Format of a memory request message
typedef struct MemoryRequest
{
    char type;
    reg_t start, end;
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

