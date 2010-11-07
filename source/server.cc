#include "includes/virtualmachine.h"

#include <errno.h>
#include <unistd.h>
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

// The following should only ever be changed to true by the serve thread.
// It is used to tell the VM thread that all init has occured properly and
// the server has aquired the work lock.  This mechanism is here in case
// the VM finishes doing all of it's work before the thread even gets to lock.
// Remember that from starting the thread and doing all the socket work, the
// server thread is making a ton of really slow syscalls.
bool _ready = false;

// Define this here so we don't have to put #include <pthread.h> in the server
// header file
pthread_t _listener;

// Have to forward declare the function to be passed to pthread_create
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
    _ready = false;
    return (false);
}

bool MonitorServer::isRunning()
{
    if (pthread_kill(_listener, 0) == ESRCH)
        return (false);
    else
        return (true);
}

bool MonitorServer::ready()
{
    return (_ready);
}

int MonitorServer::run()
{
    // spawn the listener thread, explicitly joinable.
    pthread_attr_t attr;
    int ret = pthread_attr_init(&attr);
    ret = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    ret = pthread_create(&_listener, &attr, serve, (void *)_vm);
    
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
    tv.tv_usec = 100000; // every .1 seconds check for termination call
    
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
    
    // We must have the lock, otherwise it'll cause VM thread to deadlock
    // HOWEVER, nothing should have the lock at this point so if there is
    // contention, make sure to fail hard to avoid race conditions.
    int err = pthread_mutex_trylock(&server_mutex);
    if (err)
    {
        // Something has the mutex, die.
        fprintf(stderr, "Server could not get the mutex: %s\n", strerror(err));
        return (NULL);
    }
    
    // From now on, ANY return must free the mutex.  Do NOT assume that there
    // will be an exit() call when this thread dies.
    
    // Tell VM thread that we're ready.  This basically tells them we've
    // aquired the mutex and blocking on it wont cause a race condition.
    _ready = true;
    
    while(!terminate)
    {
        // copy master fds
        read_fds = master;
        if (select(fdmax+1, &read_fds, NULL, NULL, &tv) == -1)
        {
            perror("select");
            pthread_mutex_unlock(&server_mutex);
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
                        pthread_mutex_unlock(&server_mutex);
                        // try to get it back by waiting again
                        // (enter the queue)
                        pthread_mutex_lock(&server_mutex);
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
    pthread_mutex_unlock(&server_mutex);
    return (NULL);
}

