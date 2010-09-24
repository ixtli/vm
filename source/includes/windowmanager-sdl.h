#ifndef _WINDOWMANAGER_SDL_H_
#define _WINDOWMANAGER_SDL_H_

#include "SDL.h"

#include "windowmanager.h"

class SDLManager : public WindowManager
{
public:
    SDLManager();
    virtual ~SDLManager();
    
    virtual bool Init(size_t width, size_t height);
    virtual bool PollEventQueue();
private:
    bool InitWindow();
    
    size_t _window_w, _window_h;
    SDL_Surface *_surface;
    
    Uint32 _videoFlags;
    
    DISALLOW_COPY_AND_ASSIGN(SDLManager);
};

#endif
