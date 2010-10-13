#ifndef _VIRTUALMACHINE_H_
#define _VIRTUALMACHINE_H_

#include "global.h"
#include "mmu.h"

class VirtualMachine
{
public:
    VirtualMachine();
    ~VirtualMachine();

    bool init(const char *mem_in, const char *mem_out, size_t mem_size);
};

#endif
