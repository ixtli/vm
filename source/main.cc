#include <signal.h>

#include "includes/windowmanager.h"
#include "includes/virtualmachine.h"

#include "SDL_main.h"

// The SDL_main.h installed for OS X says that the following
// needs to be present.  I wont argue.
#ifdef __cplusplus
extern "C"
#endif

void sigint_handler(int sig)
{
    printf("Caught signal.  Killing.\n");
    vm->terminate = 1;
}

int main(int argc, char *argv[])
{
    char c;
    char *outpath = NULL;
    char *inpath = NULL;
    reg_t mem_size = kMinimumMemorySize;
    
    // Register signal
    void sigint_handler(int sig);
    struct sigaction sa;
    sa.sa_handler = sigint_handler;
    sa.sa_flags = SA_RESTART;
    sigemptyset(&sa.sa_mask);
    
    if (sigaction(SIGINT, &sa, NULL) == -1)
    {
        exit(1);
    }
    
    // Handle command line options
    while ((c = getopt(argc, argv, "vo:i:m:h")) != -1)
    {
        switch (c)
        {
            case 'v':
            printf("YAAA VM Version d0.0.1\n");
            exit(0);
            
            case 'o':
            outpath = (char *)malloc(sizeof(char)* strlen(optarg) + 1);
            strcpy(outpath, optarg);
            break;
            
            case 'i':
            inpath = (char *)malloc(sizeof(char)* strlen(optarg) + 1);
            strcpy(inpath, optarg);
            break;
            
            case 'm':
            mem_size = atoi(optarg);
            if (mem_size < kMinimumMemorySize)
            {
                printf("Memory size argument less than minimum.  Defaults.\n");
                mem_size = kMinimumMemorySize;
            }
            break;
            
            case 'h':
            printf("YAAA VM Help:\n");
            printf("v\t\t\tPrint version string.\n");
            printf("o [path]\t\tPrint memory to file.\n");
            printf("i [path]\t\tRead memory in from file.\n");
            printf("m [Uint32]\t\tMachine memory size in kilobytes.\n");
            exit(0);
            
            default:
            break;
        }
    }
    // Create the wm object and init it
    //WindowManager::Create();
    //wm->Init(640, 480);
    
    // Now that we've intialized our environment, start the machine
    vm = new VirtualMachine();
    
    // Init the VM
    if (vm->init(inpath, outpath, mem_size))
    {
        // We found a problem.
        exit(1);
    }
    
    // Run the VM
    vm->run(false);
    
    // Clean up and exit
    delete vm;
    //WindowManager::Destroy();
    if (inpath)
        free(inpath);
    if (outpath)
        free(outpath);
    return 0;
}