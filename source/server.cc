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
#include "includes/virtualmachine.h"

pthread_t _listener;
void *serve( void *ptr );

MonitorServer::MonitorServer(VirtualMachine *vm) : _vm(vm)
{ }

MonitorServer::~MonitorServer()
{
    printf("Waiting for listener thread to terminate... ");
    terminate = 1;
    pthread_join(_listener, NULL);
    printf("Done.\n");
}

bool MonitorServer::init()
{
    if (!_vm) return (true);
    return (false);
}

int MonitorServer::run()
{
    // spawn a listener thread
    int ret = pthread_create(&_listener, NULL, serve, (void *)_vm);
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
    
    // Find out what machine we're dealing with
    VirtualMachine *vm = (VirtualMachine *)ptr;
    if (!vm)
    {
        fprintf(stderr, "No virtual machine instance.  Aborting.\n");
        return (NULL);
    }
    
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
    
    // Take the lock so the vm goes to sleep while we wait for
    // something to do
    pthread_mutex_lock(&vm->waiting);
    
    while(!terminate)
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
                        buf[sizeof(buf)-1] = '\0';
                        vm->operation = buf;
                        vm->opsize = sizeof(buf);
                        // give the vm a chance to work
                        pthread_mutex_unlock(&vm->waiting);
                        // try to get it back by waiting again
                        // (enter the queue)
                        pthread_mutex_lock(&vm->waiting);
                        if (vm->respsize > 0)
                        {
                            send(i, vm->response, vm->respsize, 0);
                            free(vm->response);
                            vm->response = NULL;
                        } else {
                            // unknown command
                            char temp[512];
                            sprintf(temp, "Unknown command: %s", buf);
                            send(i, temp, strlen(temp), 0);
                        }
                    }
                }
            }
        }
    }
    
    printf("Shutting down listener.\n");
    pthread_mutex_unlock(&vm->waiting);
}

