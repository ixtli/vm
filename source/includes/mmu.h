#ifndef _MMU_H_
#define _MMU_H_

#include "global.h"

class MMU
{
public:
    MMU(size_t size, size_t rtime, size_t wtime);
    ~MMU();
    
    bool init();
    
    bool loadFile(const char *path);
    bool writeOut(const char *path);
    
    // Operational: must return the timing
    size_t write(size_t addr, unsigned int valueToSave);
    size_t read(size_t addr, unsigned int &valueToRet);
    size_t readRange(size_t start, size_t end, bool hex, char **ret);

private:
    size_t _memory_size, _read_time, _write_time;
    unsigned int *_memory;
};

#endif
