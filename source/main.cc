#include <signal.h>
#include <limits.h>
#include <string.h>
#include <unistd.h>

#include "includes/virtualmachine.h"

// Probably won't be using SDL in the server, but if we are ...
#ifdef USE_SDL
    #include "includes/windowmanager.h"
    #include "SDL_main.h"
    // The SDL_main.h installed for OS X says that the following
    // needs to be present.  I wont argue.
    #ifdef __cplusplus
        extern "C"
    #endif
#endif

#define kDefaultConfigPath "config.lua"

void sigint_handler(int sig)
{
    printf("Caught signal.  Killing.\n");
    terminate = 1;
}

int main(int argc, char *argv[])
{
    char c;
    char config_path[PATH_MAX] = kDefaultConfigPath;
    
    // Register signal handler for SIGINT
    void sigint_handler(int sig);
    struct sigaction sa;
    sa.sa_handler = sigint_handler;
    sa.sa_flags = SA_RESTART;
    sigemptyset(&sa.sa_mask);
    
    // Initialize our terminate value
    terminate = 0;
    
    if (sigaction(SIGINT, &sa, NULL) == -1)
    {
        exit(1);
    }
    
    // Handle command line options
    while ((c = getopt(argc, argv, "vc:h")) != -1)
    {
        switch (c)
        {
            case 'v':
            printf("YAAA VM Version d0.0.1\n");
            exit(0);
            
            case 'c':
            strcpy(config_path, optarg);
            break;
            
            case 'h':
            printf("YAAA VM Help:\n");
            printf("v\t\t\tPrint version string.\n");
            printf("c <path>\t\tPath to the lua configuration file.\n");
            exit(0);
            
            default:
            break;
        }
    }
    
    // Now that we've intialized our environment, start the machine
    VirtualMachine *vm = new VirtualMachine() ;
    
    // Configure the VM using the config file
    if (vm->configure(config_path))
    {
        fprintf(stderr, "VM configuration failed, aborting.\n");
        exit(2);
    }
    
    // Init the VM
    if (vm->init())
    {
        // We found a problem.
        fprintf(stderr, "VM init failed, aborting.\n");
        exit(1);
    }
    
    // Run the VM and don't return after Fetch EXecute
    vm->run(false);
    
    // Clean up and exit
    delete vm;
    return (0);
}