#include "includes/virtualmachine.h"
#include "includes/interrupt.h"

#define kDefaultSWIntCycles     10

#define kBREAK_INSTRUCTION      0xEF00000F
#define kRETURN_INSTRUCTION     0xEF0000FF

reg_t jump_table[] = {      0x0, sizeof(reg_t)          };
reg_t functions[] = {       kBREAK_INSTRUCTION          ,
                            kRETURN_INSTRUCTION
                                                        };

InterruptController::InterruptController(VirtualMachine *vm, cycle_t timing) :
    _vm(vm), _swint_cycles(timing)
{}

InterruptController::~InterruptController()
{
    _vm = NULL;
}

bool InterruptController::init()
{
    printf("Loading interrupt controller... ");
    
    // Return true if instantiation failed
    if (!_vm)
    {
        printf("invalid parameters.\n");
        return (true);
    }
    
    if (!_swint_cycles) _swint_cycles = kDefaultSWIntCycles;
    
    _vm->installJumpTable(jump_table, sizeof(jump_table));
    _vm->installIntFunctions(functions, sizeof(functions));
    
    printf("Done.\n");
    return (false);
}

cycle_t InterruptController::swint(reg_t comment)
{
    // There are two possible interpretations of the comments field
    if (!_vm->supervisor)
    {
        // If user-mode code called this then we treat the comment
        // as an offset into the interrupt table.
        
        // Error check
        if (comment > sizeof(jump_table) -1)
        {
            fprintf(stderr, "Instruction Unit TRAP: Invalid interrupt loc\n");
            return (0);
        }
        
        // Move the program counter to the start of the jump function
        *(_vm->selectRegister(kPCCode)) = sizeof(jump_table)+jump_table[comment];
        
        // We're entering supervisor mode
        _vm->supervisor = true;
        return(_swint_cycles);
    }
    
    // otherwise we're in supervisor mode and the vm is trying to talk to
    // the host machine's hardware.  We use the 'ret' value to simulate
    // the time it takes to do one of these hardware interrupts
    size_t ret = 1;
    
    switch (comment)
    {
        case 0xF:
        // Treat this as BREAK, which should stop the FEX and allow the server
        // to query the state of the machine
        printf("Hardware Interrupt: BREAK\n");
        _vm->fex = false;
        break;
        
        case 0xFF:
        // This is the return function, which sets PC to r15
        // and turns off supervisor mode
        *(_vm->selectRegister(kPCCode)) = *(_vm->selectRegister(kR15Code));
        _vm->supervisor = false;
        break;
        
        default:
        break;
    }
    
    return (ret);
}
