#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#include "includes/server.h"

void *serve( void *ptr );

pthread_t _listener;

MonitorServer::MonitorServer()
{}

MonitorServer::~MonitorServer()
{
    printf("Waiting for listener thread to terminate... ");
    // The following is a hack!
    vm->terminate = true;
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
    int ret = pthread_create( &_listener, NULL, serve, NULL);
    return (ret);
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void *serve( void *ptr )
{
    printf("Monitor connection listener thread started.\n");
    
    fd_set master;      // Master fd descr list
    fd_set read_fds;    // temp fd for select()
    int fdmax;          // max file desc number
    
    int listener;
    int newfd;
    struct sockaddr_storage remoteaddr;
    socklen_t addrlen;
    
    char buf[256];
    int nbytes;
    
    char remoteIP[INET6_ADDRSTRLEN];
    
    int yes=1;
    int rv;
    
    struct addrinfo hints, *serverinfo, *p;
    
    // timing for the select
    struct timeval tv;
    tv.tv_usec = 500000; // every .5 seconds check for termination call
    
    // Clear the master and temp sets
    FD_ZERO(&master);
    FD_ZERO(&read_fds);
    
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // Use current IP
    
    if ((rv = getaddrinfo(NULL, PORT, &hints, &serverinfo)) != 0)
    {
        fprintf(stderr, "Error getting addrinfo: %s\n", gai_strerror(rv));
        return (NULL);
    }
    
    // loop through all resuls and bind to the first one
    for (p = serverinfo; p != NULL; p = p->ai_next)
    {
        listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (listener < 0)
        {
            continue;
        }
        
        // get rid of "address already in use" messages
        setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
        
        if (bind(listener, p->ai_addr, p->ai_addrlen) < 0)
        {
            close(listener);
            continue;
        }
        
        break;
    }
    
    if (p == NULL)
    {
        printf("Server failed to bind.\n");
        return (NULL);
    }
    
    // clean up server info
    freeaddrinfo(serverinfo);
    
    if (listen(listener, BACKLOG) == -1)
    {
        perror("listen");
        exit(1);
    }
    
    printf("Listening for server conncetions...\n");
    fflush(stdout);
    
    // add listener to master set
    FD_SET(listener, &master);
    
    // keep track of the biggest file desriptor
    fdmax = listener;
    
    while(!vm->terminate)
    {
        // copy master fds
        read_fds = master;
        if (select(fdmax+1, &read_fds, NULL, NULL, &tv) == -1)
        {
            perror("select");
            return (NULL);
        }
        
        // poll existing connections looking for data to read
        for (int i = 0; i <= fdmax; i++)
        {
            if (FD_ISSET(i, &read_fds))
            {
                // Someone wants to talk to us
                if (i == listener)
                {
                    // This is a new connection ...
                    addrlen = sizeof(remoteaddr);
                    newfd = accept(listener, (struct sockaddr *)&remoteaddr,
                        &addrlen);
                    
                    if (newfd == -1)
                    {
                        perror("accept");
                    } else {
                        // add to master set
                        FD_SET(newfd, &master);
                        if (newfd > fdmax)
                        {
                            // keep track of max fd
                            fdmax = newfd;
                        }
                        printf("New connection from %s on socket %d\n",
                            inet_ntop(remoteaddr.ss_family,
                            get_in_addr((struct sockaddr *)&remoteaddr),
                            remoteIP, INET6_ADDRSTRLEN),
                            newfd);
                    }
                } else {
                    // handle data from client
                    if ((nbytes = recv(i, buf, sizeof(buf), 0)) <= 0)
                    {
                        //got error or connection closed by client
                        if (nbytes == 0)
                        {
                            // connection closed
                            printf("Socket %d hung up.\n", i);
                        } else {
                            perror("recv");
                        }
                        close(i);
                        // Remove from master set
                        FD_CLR(i, &master);
                    } else {
                        // Got some data from a client
                        buf[sizeof(buf)-1] = '\0';
                        printf("Client said %s", buf);
                    }
                }
            }
        }
    }
}

