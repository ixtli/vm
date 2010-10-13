#include <unistd.h>

#include "includes/windowmanager.h"
#include "includes/virtualmachine.h"

#include "SDL_main.h"

// The SDL_main.h installed for OS X says that the following
// needs to be present.  I wont argue.
#ifdef __cplusplus
extern "C"
#endif

int main(int argc, char *argv[])
{
    char c;
    char *outpath = NULL;
    char *inpath = NULL;
    size_t mem_size = 256;
    
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
            if (mem_size == 0)
            {
                printf("Memory size argument evaluated to zero.  Using 256kb.\n");
                mem_size = 256;
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
    WindowManager::Create();
    wm->Init(640, 480);
    
    // Now that we've intialized our environment, start the machine
    VirtualMachine *vm = new VirtualMachine();
    
    // Init the VM
    if (vm->init(inpath, outpath, mem_size))
    {
        // We found a problem.
        exit(1);
    }
    
    // Run the VM
    vm->run();
    
    // Clean up and exit
    delete vm;
    WindowManager::Destroy();
    return 0;
}