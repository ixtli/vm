#ifndef _SERVER_H_
#define _SERVER_H_

#include <pthread.h>

#include "global.h"
#include "virtualmachine.h"

// port to listen on
#define PORT "1337"

// How many pending connections do we tell to wait before we
// establish a TCP connection with them?
#define BACKLOG 10

#define MAX_CONNECTIONS 10

// Server protocol
#define kCodeLength             3
#define kTooManyConnections     "100"
#define kConnectionAccepted     "200"
#define kServerHangingUp        "300"
#define kWaitingForRequests     "400"

class MonitorServer
{
public:
    MonitorServer();
    ~MonitorServer();
    
    bool init();
    int run();
private:
};

#endif

