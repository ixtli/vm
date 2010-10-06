#include <unistd.h>
#include "includes/windowmanager.h"
#include "SDL_main.h"

// The SDL_main.h installed for OS X says that the following
// needs to be present.  I wont argue.
#ifdef __cplusplus
extern "C"
#endif

int main(int argc, char *argv[])
{
    char c;
    
    // Handle command line options
    while ((c = getopt(argc, argv, "v")) != -1)
    {
        switch (c)
        {
            case 'v':
            
            break;
            default:
            break;
        }
    }
    
    // Create the wm object and init it
    WindowManager::Create();
    wm->Init(640, 480);
    
    // Wait on some sort of user action
    wm->PollEventQueue();
    
    // Clean up and exit
    WindowManager::Destroy();
    return 0;
}