#ifndef _SERVER_H_
#define _SERVER_H_

#include <pthread.h>

#include "global.h"
#include "virtualmachine.h"

#define PORT "3490"

class MonitorServer
{
public:
    MonitorServer();
    ~MonitorServer();
    
    bool init();
    int run();
private:
    pthread_t _listener;
};

#endif

