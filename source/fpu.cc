#include "includes/virtualmachine.h"
#include "includes/pipeline.h"
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
    if (!_vm) return (true);
    return (false);
}

cycle_t FPU::execute(const FPFlags &flags)
{
    cycle_t cycles = 0;
    bool shift_carry = false;
    bool arithmetic = true;
    reg_t dest;
    //reg_t fpd = _vm->selectRegister(flags.d);
    reg_t fpn = _vm->selectRegister(flags.n);
    reg_t fpm = _vm->selectRegister(flags.m);

    //  printf("%u %u %u %u", *_fps, *_fpd, *_fpn, *_fpm);
    
    
    //Add FPn + FPm and store result in FPs and FPd.
    switch (flags.op)
    {
        case kFAD:
            _output = fpn + fpm;
            break;
        case kFSB:
            _output = fpn - fpm;
            break;
        case kFML:
            _output = (fpn * fpm);
            break;
        case kFDV:
            _output = fpn / fpm;
            break;
        default:
            break;
    }
    //fpd = _output;   
    return (cycles);
}
