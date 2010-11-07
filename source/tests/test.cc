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