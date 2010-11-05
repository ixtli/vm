#ifndef _WINDOWMANAGER_H_
#define _WINDOWMANAGER_H_

#include "global.h"

class WindowManager
{
public:
    virtual ~WindowManager() {}
    
    static bool Create();
    static void Destroy();
    
    virtual bool Init(size_t width, size_t height) = 0;
    virtual bool PollEventQueue() = 0;
};

#endif
