// Maybe later?
// #include <OpenGL/gl.h>

#include "includes/windowmanager-sdl.h"

WindowManager *wm = NULL;

bool WindowManager::Create()
{
    if (wm == NULL)
    {
        wm = (WindowManager *) new SDLManager();
        return false;
    }
    
    return true;
}

void WindowManager::Destroy()
{
    if (wm != NULL)
    {
        delete wm;
        wm = NULL;
    }
}

SDLManager::SDLManager() :
    _window_w(0), _window_h(0),
    _videoFlags(SDL_HWSURFACE | SDL_RESIZABLE | SDL_DOUBLEBUF)
{
}

SDLManager::~SDLManager()
{
    SDL_Quit();
}

bool SDLManager::Init(size_t width, size_t height)
{
    if (SDL_Init( SDL_INIT_EVERYTHING ) < 0 )
    {
        printf("Unable to initialize SDL.\n");
        return true;
    }
    
    _window_w = width;
    _window_h = height;
    
    if (InitWindow())
        return true;
    
    return false;
}

bool SDLManager::InitWindow()
{
    //Create the window
    // You'll have to add SDL_WINDOW_OPENGL here later when adding
    // hardware acceleration.
    _surface = SDL_SetVideoMode(_window_w, _window_h, 32, _videoFlags);
    
    if (!_surface)
    {
        printf("Unable to create the window.\n");
        return true;
    }
    
    return false;
}

bool SDLManager::PollEventQueue()
{
    SDL_Event event;
    bool done = false;
    
    if (SDL_PollEvent(&event))
    {
            switch (event.type) {
            case SDL_QUIT:
                return true;
            case SDL_VIDEORESIZE:
                _surface = SDL_SetVideoMode(event.resize.w, event.resize.h, 32,
                    _videoFlags);
                return false;
            }
    }
}

