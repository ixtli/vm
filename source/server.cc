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

void _send_stream_status(VirtualMachine *vm, int fd)
{
    size_t len;
    MachineStatus s;
    vm->statusStruct(s);
    
    send(fd, &s, sizeof(MachineStatus), 0);
}

void _send_stream_description(VirtualMachine *vm, int fd)
{
    size_t len;
    MachineDescription d;
    vm->descriptionStruct(d);
    
    send(fd, &d, sizeof(MachineDescription), 0);
}

void _send_stream_memory(VirtualMachine *vm, int fd, MemoryRequest *m)
{
    reg_t mem_size;
    size_t range = m->end - m->start;
    size_t len = sizeof(MemoryRequest) + range;
    const char *memory = (const char *)vm->readOnlyMemory(mem_size);
    
    // Bounds testing
    if (mem_size < (m->start + range))
    {
        fprintf(stderr, "Memory request range out of bounds.\n");
        return;
    }
    
    // Allocate memory
    char *final = (char *)malloc(len);
    
    // We might actually not be able to serve the request
    if (!final)
    {
        fprintf(stderr, "Could not allocate memory request buffer.\n");
        return;
    }
    
    // Copy memory request into the response
    for (int i = 0; i < sizeof(MemoryRequest); i++)
        final[i] = ((char *)m)[i];
    
    // Copy the memory
    for (int i = 0; i < range; i++)
        final[i + sizeof(MemoryRequest)] = memory[i+m->start];
    
    // Send the request
    send(fd, final, len, 0);
    
    // Clean up
    free(final);
}

void _demux_stream_op(char *buf, size_t size, VirtualMachine *vm, int fd)
{
    // Deal with commands sent in the stream format
    char code = *(char *)buf;
    switch (code)
    {
        case kStatusMessage:
        _send_stream_status(vm, fd);
        break;
        
        case kMachineDescription:
        _send_stream_description(vm, fd);
        break;
        
        case kMemoryRange:
        if (size < sizeof(MemoryRequest))
        {
            fprintf(stderr, "Memory request format error.\n");
            return;
        }
        _send_stream_memory(vm, fd, (MemoryRequest *)buf);
        break;
        
        case kStreamHandshake:
        default:
        fprintf(stderr, "Stream request format error.\n");
        break;
    }
}

bool _demux_op(char *op, size_t size, VirtualMachine *vm, int fd)
{
    // Deal with commands sent as human readable strings
    
    // Return true if we handled the request, but only handle read only ops
    
    // check for commands that require no arguments
    if (strncmp(op, kStatCommand, 6) == 0) {
        // Send status
        size_t len;
        char *temp = vm->statusString(len);
        if (temp)
        {
            send(fd, temp, len, 0);
            free(temp);
        }
        return (true);
    }
    
    // Tokenize commands that require args
    char *pch = strtok(op, " ");
    if (strcmp(pch, kReadCommand) == 0) {
        // read
        reg_t addr;
        reg_t val;
        pch = strtok(NULL, " ");
        if (pch)
            addr = (reg_t)atoi(pch);
        else
            addr = 0;
        
        // Do the read
        vm->readWord(addr, val);
        
        // Format a string to be sent
        char buf[32];
        sprintf(buf, "%#x - %#x\n", addr, val);
        buf[31] = '\0';
        size_t respsize = strlen(buf);
        send(fd, buf, respsize, 0);
        
        return (true);
    } else if (strcmp(pch, kRangeCommand) == 0) {
        // range
        int addr, val;
        pch = strtok(NULL, " ");
        if (pch)
            addr = atoi(pch);
        else
            addr = 0;
        
        pch = strtok(NULL, " ");
        if (pch)
            val = atoi(pch);
        else
            val = 0;
        
        char *temp;
        vm->readRange(addr, val, true, &temp);
        send(fd, temp, strlen(temp), 0);
        
        // Clean up
        if (temp) free(temp);
        return (true);
    }
    
    return (false);
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
    fd_set stream;      // Clients that get stream data and present it using GUI
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
    FD_ZERO(&stream);
    
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
                        printf("New connection from %s on socket %d.\n",
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
                        FD_CLR(i, &stream);
                    } else {
                        if (nbytes == sizeof(StreamHandshake))
                        {
                            // Check to see if we're talking to a stream client
                            if (buf[0] == kStreamHandshake)
                            {
                                FD_SET(i, &stream);
                                printf("Successful handshake with socket %d.\n", i);
                                // Push status
                                _send_stream_description(vm, i);
                                _send_stream_status(vm, i);
                                
                                // Send the initial image of memory
                                MemoryRequest m;
                                m.type = kMemoryRange;
                                m.start = 0;
                                m.end = 256;
                                _send_stream_memory(vm, i, &m);
                                continue;
                            }
                        }
                        
                        if (nbytes == sizeof(buf))
                        {
                            fprintf(stderr, "Client request way too large.");
                            buf[sizeof(buf)-1] = '\0';
                            continue;
                        }
                        
                        // Avoid buffer overflows like the plague.
                        buf[nbytes] = '\0';
                        
                        // Are we a stream client?
                        if (FD_ISSET(i, &stream))
                        {
                            _demux_stream_op(buf, nbytes, vm, i);
                            continue;
                        }
                        
                        // If it's a status check or read operation we don't
                        // need to fool with mutex operations, so check those
                        // first.
                        
                        if (_demux_op(buf, nbytes, vm, i))
                            continue;
                        
                        // If we get here we need the VM to do something
                        // like process an opcode, so we need to lock and unlock
                        vm->operation = buf;
                        vm->opsize = sizeof(buf);
                        // give the vm a chance to work
                        pthread_mutex_unlock(&server_mutex);
                        // try to get it back by waiting again
                        // (enter the queue)
                        pthread_mutex_lock(&server_mutex);
                        // Now we expect response and respsize to be allocated
                        // and set, respectively.  We also remember to clean up
                        if (vm->respsize > 0)
                        {
                            send(i, vm->response, vm->respsize, 0);
                            free(vm->response);
                            vm->response = NULL;
                        } else {
                            // unknown command
                            char temp[512];
                            sprintf(temp, "Unknown command: %s", buf);
                            temp[511] = '\0';
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

