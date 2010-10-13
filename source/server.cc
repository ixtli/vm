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

pthread_t _listener;
size_t _connection_count = 0;

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
    int ret = pthread_create( &_listener, NULL, listen_for_connections, NULL);
    return (ret);
}

void sigchld_handler(int s)
{
    while(waitpid(-1, NULL, WNOHANG) > 0);
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void *handle_connection( void *ptr )
{
    long fd_temp = (long)ptr;
    int fd = fd_temp; // We loose precision here but that's ok.
    
    if (send(fd, kConnectionAccepted, kCodeLength, 0) == -1)
    {
        perror("Sending to accepted client.");
        close(fd);
        _connection_count--;
        return (NULL);
    }
    
    while (!vm->terminate)
    {
        // Listen for a request
        if (send(fd, kWaitingForRequests, kCodeLength, 0) == -1)
        {
            perror("Sending to accepted client.");
            close(fd);
            _connection_count--;
            return (NULL);
        }
    }
    
    // clean up and disconnect
    send(fd, kServerHangingUp, kCodeLength, 0);
    close(fd);
    _connection_count--;
    return (NULL);
}

void *listen_for_connections( void *ptr )
{
    printf("Monitor connection listener thread started.\n");
    int sockfd, new_fd;
    struct addrinfo hints, *serverinfo, *p;
    // client info
    struct sockaddr_storage their_addr;
    socklen_t sin_size;
    struct sigaction sa;
    int yes = 1;
    char s[INET6_ADDRSTRLEN];
    int rv;
    
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
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                            p->ai_protocol)) == -1)
        {
            perror("server: socket");
            continue;
        }
        
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
                        sizeof(int)) == -1)
        {
            perror("setsockopt");
            exit(1);
        }
        
        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1)
        {
            close(sockfd);
            perror("server: bind");
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
    
    if (listen(sockfd, BACKLOG) == -1)
    {
        perror("listen");
        exit(1);
    }
    
    // reap dead processes
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1)
    {
        perror("sigaction");
        exit(1);
    }
    
    printf("Listening for server conncetions...\n");
    fflush(stdout);
    
    while(!vm->terminate)
    {
        sin_size = sizeof(their_addr);
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd == -1)
        {
            perror("accept");
            continue;
        }
        
        inet_ntop(their_addr.ss_family,
            get_in_addr((struct sockaddr *)&their_addr),
            s, sizeof(s));
        printf("Got a connection from %s\n", s);
        
        // Check to make sure we wouldn't be going over our max connections
        if (_connection_count >= MAX_CONNECTIONS)
        {
            if (send(new_fd, kTooManyConnections, kCodeLength, 0) == -1)
                perror("Sending to denied client.");
            close(new_fd);
        } else {
            // start a new process
            printf("Accepting connection.\n");
            pthread_t new_thread;
            int ret = pthread_create(
                &new_thread, NULL, handle_connection, (void *)new_fd);
        }
    }
    
    printf("Waiting for child processes to close...");
}

