#include "includes/windowmanager.h"

#include "SDL_main.h"

// The SDL_main.h installed for OS X says that the following
// needs to be present.  I wont argue.
#ifdef __cplusplus
extern "C"
#endif

int main(int argc, char *argv[])
{
    WindowManager::Create();
    wm->Init(640, 480);
    
    wm->PollEventQueue();
    
    WindowManager::Destroy();
    return 0;
}