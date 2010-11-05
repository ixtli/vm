#include <signal.h>

// SIGINT flips this to tell everything to turn off
// Must have it declared extern and at file scope so that we can
// read it form anywhere.  Also, it must be outside of the extern "C" block
// otherwise it will not be mangled properly and link will fail.
volatile sig_atomic_t terminate;

// The SDL_main.h installed for OS X says that the following
// needs to be present.  I wont argue.
#ifdef USE_SDL
    #include "SDL_main.h"
    #ifdef __cplusplus
    extern "C"
    #endif
#endif

int main(int argc, char *argv[])
{
    return 0;
}