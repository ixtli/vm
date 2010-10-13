#ifndef _MMU_H_
#define _MMU_H_

#include "global.h"

class MMU
{
public:
    MMU(size_t size, size_t rtime, size_t wtime);
    ~MMU();
    
    bool init();
    
    // Operational: must return the timing
    size_t write(size_t addr, size_t valueToSave);
    size_t read(size_t addr, size_t &valueToRet);
<<<<<<< HEAD
    char* readRange(size_t start, size_t end, size_t type);   
=======
    char* readRange(size_t start, size_t end);
>>>>>>> 1c1501f04438a354ab3db334d0439848ad2057c5

private:
    size_t _memory_size, _read_time, _write_time;
    unsigned int *_memory;
};

#endif
