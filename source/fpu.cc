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
    bool arithmetic = false;
    reg_t fpd = _vm->selectRegister(flags.d);
    reg_t fpn = _vm->selectRegister(flags.n);
    reg_t fpm = _vm->selectRegister(flags.m);

    //  printf("%u %u %u %u", *_fps, *_fpd, *_fpn, *_fpm);
    
    
    //Add FPn + FPm and store result in FPs and FPd.
    switch (flags.op)
    {
        case kFAD:
            printf("==============add\n");
            arithmetic = true;
            _output = fpn + fpm;
            cycles += kFADCycles;
            break;
        case kFSB:
            printf("==============sub\n");
            arithmetic = true;
            _output = fpn - fpm;
            cycles += kFSBCycles;
            break;
        case kFML:
            printf("==============mul\n");
            arithmetic = true;
            _output = (fpn * fpm);
            cycles += kFMLCycles;
            break;
        case kFDV:
            printf("==============div\n");
            arithmetic = true;
            _output = fpn / fpm;
            cycles += kFDVCycles;
            break;
        default:
            break;
    }
    fpd = _output;   
    return (cycles);
}
