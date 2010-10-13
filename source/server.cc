#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#include "includes/server.h"

void *listen_for_connections( void *ptr );

MonitorServer::MonitorServer()
{}

MonitorServer::~MonitorServer()
{
    printf("Waiting for listener thread to terminate... ");
    pthread_join(_listener, NULL);
    printf("Done.\n");
}

bool MonitorServer::init()
{
    // No errors
    return (false);
}

int MonitorServer::run()
{
    // spawn a listener thread
    int ret = pthread_create( &_listener, NULL, listen_for_connections, NULL);
    return (ret);
}

void *listen_for_connections( void *ptr )
{
    printf("Monitor connection listener thread started.\n");
    
}

