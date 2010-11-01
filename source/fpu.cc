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
    printf("=================here\n");
    size_t cycles = 0;
    bool shift_carry = false;
    bool arithmetic = false;
    reg_t *_fps = vm->selectRegister(fps);
    reg_t *_fpd = vm->selectRegister(fpd);
    reg_t *_fpn = vm->selectRegister(fpn);
    reg_t *_fpm = vm->selectRegister(fpm);
    
  //  printf("%u %u %u %u", *_fps, *_fpd, *_fpn, *_fpm);


//Add FPn + FPm and store result in FPs and FPd.
    switch (op)
    {
        case kFAD:
            printf("==============add\n");
            *_fps = *_fpn + *_fpm;
            cycles += kFADCycles;
            break;
        case kFSB:
            printf("==============sub\n");
            *_fps = *_fpn - *_fpm;
            cycles += kFSBCycles;
            break;
        case kFML:
            printf("==============mul\n");
            cycles += kFMLCycles;
            break;
        case kFDV:
            printf("==============div\n");
            *_fps = *_fpn / *_fpm;
            cycles += kFDVCycles;
            break;
           

        default:
            break;
    }
    
    return (0);
}
