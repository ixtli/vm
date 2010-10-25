#include "includes/virtualmachine.h"
#include "includes/fpu.h"

FPU::FPU(VirtualMachine *vm) : _vm(vm)
{}

FPU::~FPU()
{
    _vm = NULL;
}

bool FPU::init()
{
    // Return true if instantiation failed
    if (!vm) return (true);
    return (false);
}

size_t FPU::execute(char op, reg_t &fps, reg_t &fpd, reg_t &fpn, reg_t &fpm)
{
    switch (op)
    {
        default:
        break;
    }
    
    return (0);
}